# 阶段 0 笔记：环境与工具链

> 整合自 `linux-commands.md` 与 `gpp-gdb.md`（已合并进本文件）。阶段 0 前置知识速查。
> 配套：`PLAN.md`（计划）、`MEMORY.md`（进度）、`Development Log.md`（日志）。

---

## 一、Linux 常用命令

### 0. 三个核心概念

1. **一切皆文件**：普通文件、目录、socket、设备，都是「文件」，都有路径。
2. **路径**：
   - 绝对路径：`/home/lu_yunhao/xxx`（从根 `/` 开始）
   - 相对路径：`.` 当前目录、`..` 上一级、`~` 家目录
3. **命令格式**：`命令 [选项] [参数]`；短选项 `-l`、长选项 `--list`。

### 1. 导航

| 命令 | 作用 | 例子 |
|------|------|------|
| `pwd` | 显示当前目录 | `pwd` → `/home/lu_yunhao` |
| `ls` | 列出文件 | `ls -l` 详情；`ls -a` 含隐藏；`ls -lh` 人类可读大小 |
| `cd` | 切换目录 | `cd ~` 回家；`cd ..` 上一级；`cd -` 回上一个目录 |

### 2. 文件 / 目录操作

```bash
mkdir tinywebserver       # 建目录；-p 一次建多层：mkdir -p a/b/c
touch main.cpp            # 新建空文件（或刷新时间戳）
cp PLAN.md backup.md      # 复制文件；复制目录加 -r：cp -r src dst
mv old.cpp new.cpp        # 重命名；mv file dir/ 是移动
rm old.cpp                # 删除文件；删目录 rm -r dir；强制 rm -f
```

> ⚠️ Linux 没有回收站，`rm -rf` 删了就没了，动手前先 `ls` 确认路径。

查看文件内容：

```bash
cat main.cpp              # 一次性打印全文（小文件）
less PLAN.md              # 翻页；q 退出；/关键字 搜索
head -n 20 file           # 看前 20 行
tail -f server.log        # 实时追踪日志尾部；Ctrl+C 退出
```

### 3. 权限与 sudo

`ls -l` 第一列：`drwxrwxrwx`

```
d        rwx       rwx       rwx
类型     所有者    同组      其他人
d=目录   r读 w写 x执行
```

```bash
chmod u+x build.sh        # 给所有者加执行权限（u=user，+x=加执行）
chmod 755 build.sh        # 数字法：7=rwx 5=r-x 5=r-x
sudo apt install xxx      # 管理员身份执行；要密码且不回显（正常现象）
```

### 4. 软件包 apt

```bash
sudo apt update                      # 刷新软件源索引（装新包前先跑）
sudo apt install -y build-essential  # 安装；-y 自动确认
sudo apt remove xxx                  # 卸载
```

### 5. 进程管理

```bash
ps aux | grep server       # 查进程；| 管道：左边输出喂给 grep 过滤
top                        # 实时看 CPU/内存；q 退出
kill 12345                 # 正常结束进程；kill -9 12345 强杀（最后手段）
Ctrl+C                     # 终止前台程序
```

### 6. 网络（项目高频）

```bash
curl -i http://localhost:9006/                       # 看完整响应头（测服务器必用）
curl -X POST -d "a=1&b=2" http://localhost:9006/     # 发 POST
ping 127.0.0.1                                       # 测连通性
ss -tlnp                                             # 看监听端口
ip addr                                              # 看本机 IP（hostname -I 更简短）
```

### 7. 搜索

```bash
grep -rn "main" .          # 当前目录递归搜"main"；-r 递归 -n 行号 -i 忽略大小写
find . -name "*.cpp"       # 按文件名找所有 .cpp 文件
```

### 8. 查帮助

```bash
g++ --help                 # 多数命令支持 --help
man 2 socket               # 手册：2=系统调用 3=库函数 7=杂项
man 7 epoll                # 本项目会用到
```

### 9. 提速小技巧

- **Tab 补全**：输入前几个字母按 Tab 自动补全
- **通配符**：`*.cpp` 匹配所有 .cpp 文件
- `Ctrl+A` 行首、`Ctrl+E` 行尾、`Ctrl+R` 搜索历史命令、`history` 看历史
- `clear` 清屏

---

## 二、g++ 编译

### 1. 编译四步

`g++ -o main main.cpp` 一步到位，背后是 4 段流水线：

```
main.cpp ──预处理──▶ main.i ──编译──▶ main.s ──汇编──▶ main.o ──链接──▶ main(可执行)
```

| 步骤 | 命令 | 输出 | 干什么 |
|------|------|------|--------|
| 1 预处理 | `g++ -E main.cpp -o main.i` | `main.i` | 展开 `#include`、`#define`、去注释、条件编译 |
| 2 编译 | `g++ -S main.cpp -o main.s` | `main.s` | C++ 源码翻译成汇编语言 |
| 3 汇编 | `g++ -c main.cpp -o main.o` | `main.o` | 汇编变机器码，但 `cout` 等符号还没地址 |
| 4 链接 | `g++ main.o -o main` | `main` | 把 `.o` + 标准库拼起来，解析符号，生成可执行文件 |

### 2. 常用编译选项

| 选项 | 作用 | 本项目 |
|------|------|:---:|
| `-o 文件名` | 指定输出文件名（默认 `a.out`） | ✅ |
| `-std=c++11` | 用 C++11 标准 | ✅ 核心 |
| `-Wall` | 打开常用警告（必开，帮你抓 bug） | ✅ |
| `-Wextra` | 更多警告 | 可选 |
| `-g` | 加调试信息，gdb/CLion 断点调试必须有 | ✅ |
| `-O0` | 不优化（默认，调试友好） | ✅ 开发期 |
| `-O2` | 常规优化（压测/发布） | ✅ 阶段11 |
| `-lpthread` | 链接 pthread 线程库 | ✅ 阶段8线程池 |
| `-I 目录` | 额外头文件搜索路径 | 以后可能 |
| `-L 目录` / `-l库名` | 库搜索路径 / 链接指定库 | 以后可能 |

> CMake 底层就是调 g++ 带这些参数：`set(CMAKE_CXX_STANDARD 14)` → `-std=c++14`，CLion Debug 配置自动加 `-g -O0`。

### 3. 调试：g++ 侧的准备

```bash
g++ -g -O0 -o main main.cpp
```

- **`-g`**：把「源码行号、变量名」写进可执行文件。没有它，gdb 里只剩汇编和地址。
- **`-O0`**：不优化。开了 `-O2` 后编译器会重排代码、优化掉变量，断点乱跳、变量显示 `<optimized out>`。

---

## 三、gdb 调试

### 1. 基础命令

启动：

```bash
gdb ./main                    # 调试 main
gdb --args ./server 9006      # 带命令行参数调试
```

| 命令 | 缩写 | 作用 |
|------|------|------|
| `break 5` | `b 5` | 在第 5 行设断点 |
| `break main` | `b main` | 在 main 函数入口设断点 |
| `run` | `r` | 开始运行，到断点停下 |
| `next` | `n` | 单步执行（不进入函数内部） |
| `step` | `s` | 单步执行（进入函数内部） |
| `continue` | `c` | 继续运行到下一个断点 |
| `print 变量` | `p x` | 打印变量值 |
| `list` | `l` | 查看当前源码附近 |
| `info locals` | | 列出当前函数所有局部变量 |
| `backtrace` | `bt` | 查看函数调用栈 |
| `info breakpoints` | `i b` | 查看所有断点 |
| `delete 1` | `d 1` | 删除 1 号断点 |
| `watch 变量` | | 变量被修改时自动停下 |
| `finish` | | 运行到当前函数返回 |
| `quit` | `q` | 退出 gdb |

> 踩坑：国内网络下 gdb `run` 会卡在 debuginfod 联网下载符号 → 在 `~/.gdbinit` 里加 `set debuginfod enabled off` 永久关闭。

### 2. 完整示例

```cpp
#include <iostream>
int main() {
    int sum = 0;
    for (int i = 1; i <= 10; i++) {
        sum += i;
    }
    std::cout << sum << std::endl;   // 55
    return 0;
}
```

```bash
g++ -g -O0 -o sum sum.cpp
gdb ./sum
```

```
(gdb) break 6          # 在 sum += i 那行设断点
(gdb) run              # 运行，停在断点
(gdb) print i          # 看 i
(gdb) print sum        # 看 sum
(gdb) next             # 单步执行一行
(gdb) print sum        # sum 变了
(gdb) continue         # 到下一轮循环
(gdb) info locals      # 一次性看所有局部变量
(gdb) quit
```

### 3. 与 CLion 的关系

CLion 的「绿色小虫子 Debug」按钮，底层就是 gdb：

- 点行号左边打红点 = `break`
- Step Over = `next`；Step Into = `step`
- 调试面板看变量 = `print` / `info locals`

学会 gdb 命令，就知道 CLion 调试器在背后干什么，命令行环境也能徒手调试。
