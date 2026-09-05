# 阶段 1 笔记：文件 I/O 系统调用

> 阶段 1 前置知识 + 实践要点。核心：理解「一切皆文件」与文件描述符，能用系统调用读写文件。
> 配套：`PLAN.md`（计划）、`MEMORY.md`（进度）、`Development Log.md`（日志）。

## 1. 文件描述符 fd

Linux 把普通文件、终端、socket、设备都抽象成「文件」。每打开一个文件，内核发一个整数「号码牌」= 文件描述符（fd）。

| fd | 含义 |
|----|------|
| 0 | stdin 标准输入（键盘） |
| 1 | stdout 标准输出（屏幕） |
| 2 | stderr 标准错误（屏幕） |

`open` 返回的新 fd 从 **3** 开始（0/1/2 被占用了）。这三个是独立的 I/O 通道，可以分别重定向：

```bash
./io_demo PLAN.md > out.txt 2> err.log   # fd1→out.txt，fd2→err.log，互不干扰
```

## 2. 五个系统调用

| 调用 | 作用 | 成功返回 | 失败 |
|------|------|---------|------|
| `open(path, flags)` | 打开文件 | fd（≥3） | -1 |
| `read(fd, buf, n)` | 读最多 n 字节到 buf | 实际读到的字节数 | -1 |
| `write(fd, buf, n)` | 写最多 n 字节 | 实际写入字节数 | -1 |
| `close(fd)` | 关闭 | 0 | -1 |
| `lseek(fd, off, whence)` | 移动读写位置 | 新位置 | -1 |

flags 常用：`O_RDONLY` 只读 / `O_WRONLY` 只写 / `O_RDWR` 读写。

**lseek 常用场景**（后面 HTTP 阶段算 `Content-Length` 会用到）：

```cpp
off_t size = lseek(fd, 0, SEEK_END);  // 跳到末尾，返回值 = 文件大小
lseek(fd, 0, SEEK_SET);               // 跳回开头
// whence：SEEK_SET(0) 开头 / SEEK_CUR(1) 当前位置 / SEEK_END(2) 末尾
```

## 3. read 三种返回值（重点）

- **`> 0`**：读到了这么多字节
- **`== 0`**：读到文件末尾（EOF），没数据了 → 退出循环
- **`< 0`（-1）**：出错，具体原因看 `errno`

**为什么循环**：`read(fd, buf, 4096)` 每次最多读 4096 字节，文件更大时一次读不完，要循环读到返回 0。

## 4. 用户态 vs 内核态缓冲区

数据从磁盘到程序，走两次拷贝：

```
磁盘文件 ──▶ 内核态缓冲区 ──read()──▶ 用户态缓冲区(char buf[4096])
```

- **用户态缓冲区**：程序里自己声明的 `char buf[4096]`，在进程内存里
- **内核态缓冲区**：内核内部的内存（磁盘有 page cache），看不到
- `read` 把数据从内核缓冲区拷进 `buf`；`write` 相反

两层是因为程序不能直接碰内核内存/硬件（安全隔离），靠系统调用做拷贝。

## 5. 错误处理（errno / perror / strerror）

系统调用失败通常返回 -1，但「为什么失败」看 `errno`：

- `errno`：全局整数，记录最近一次系统调用失败的**错误码**（如 `ENOENT`=文件不存在）
- `perror("前缀")`：打印 `前缀: 错误描述`，最省事——`perror("open")` 输出 `open: No such file or directory`
- `strerror(errno)`：只**返回**错误描述字符串（不打印）

⚠️ `errno` 只在「系统调用返回 -1 失败」时才有意义。

## 6. 查手册（man 分节）

`man` 手册分「节」，数字代表类别：

| 节 | 内容 | 例子 |
|----|------|------|
| 1 | 普通命令 | `man 1 ls` |
| 2 | 系统调用 | `man 2 open`、`man 2 read` |
| 3 | 库函数 | `man 3 printf` |
| 7 | 杂项/协议 | `man 7 epoll` |

名字会重复（`open` 既是命令又是系统调用），所以要加数字指定。

## 7. 参考实现：简化版 cat

```cpp
#include <fcntl.h>      // open、O_RDONLY
#include <unistd.h>     // read、write、close、STDOUT_FILENO
#include <sys/types.h>  // ssize_t
#include <cstdio>       // perror
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <文件名>" << std::endl;
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);           // 只读打开命令行指定的文件
    if (fd == -1) {                             // open 失败
        perror("open");
        return 1;
    }

    char buf[4096];                             // 用户态缓冲区
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {   // 循环读到 EOF
        ssize_t off = 0;
        while (off < n) {                       // 处理「部分写入」
            ssize_t w = write(STDOUT_FILENO, buf + off, n - off);
            if (w == -1) { perror("write"); close(fd); return 1; }
            off += w;
        }
    }
    if (n == -1) {                              // read 出错
        perror("read");
        close(fd);
        return 1;
    }

    close(fd);                                  // 用完关掉
    return 0;
}
```

编译运行：

```bash
g++ -std=c++11 -Wall -o io_demo test/io_demo.cpp
./io_demo PLAN.md        # 原样打印 PLAN.md 全文
./io_demo 不存在的文件     # 打印 "open: No such file or directory"
```

## 8. 踩坑

- **CMake 一个 target 只能有一个 `main()`**：两个含 main 的 `.cpp`（如 `main.cpp` 和 `io_demo.cpp`）塞进同一个 `add_executable` 会链接报 `multiple definition of 'main'` → 一个可执行文件 = 一个 `add_executable`。
- **编译产物 ≠ 源码**：`g++ -o io_demo test/io_demo.cpp` 生成的 `io_demo` 是 ELF 可执行文件（构建产物），不是源码；已加入 `.gitignore`。
