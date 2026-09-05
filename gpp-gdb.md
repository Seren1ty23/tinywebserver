# g++ 编译与 gdb 调试速查

> 阶段 0 前置知识。配套：`PLAN.md`（总体计划）、`linux-commands.md`（Linux 命令）。

## 1. g++ 编译四步

`g++ -o main main.cpp` 一步到位，背后是一条 4 段流水线：

```
main.cpp ──预处理──▶ main.i ──编译──▶ main.s ──汇编──▶ main.o ──链接──▶ main(可执行)
```

| 步骤 | 命令 | 输出 | 干什么 |
|------|------|------|--------|
| 1 预处理 | `g++ -E main.cpp -o main.i` | `main.i` | 展开 `#include`、`#define`、去注释、条件编译 |
| 2 编译 | `g++ -S main.cpp -o main.s` | `main.s` | C++ 源码翻译成汇编语言 |
| 3 汇编 | `g++ -c main.cpp -o main.o` | `main.o` | 汇编变机器码，但 `cout` 等符号还没地址 |
| 4 链接 | `g++ main.o -o main` | `main` | 把 `.o` + 标准库拼起来，解析符号，生成可执行文件 |

动手观察中间产物：

```bash
g++ -E main.cpp -o main.i     # 预处理（main.i 会几万行，因为 <iostream> 被展开）
g++ -S main.cpp -o main.s     # 编译成汇编
g++ -c main.cpp -o main.o     # 汇编成目标文件
g++ main.o -o main            # 链接
wc -l main.i                  # 看预处理后有多大
head -30 main.s               # 看 Hello World 变成了什么汇编
```

---

## 2. 常用编译选项

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

---

## 3. 调试：g++ 侧的准备

调试前必须这样编译：

```bash
g++ -g -O0 -o main main.cpp
```

- **`-g`**：把「源码行号、变量名」写进可执行文件。没有它，gdb 里只剩汇编和地址，看不到源码和变量名。
- **`-O0`**：不优化。开了 `-O2` 后编译器会重排代码、优化掉变量，断点会乱跳、变量显示 `<optimized out>`，没法调。

---

## 4. gdb 基础命令

启动：

```bash
gdb ./main                    # 调试 main
gdb --args ./server 9006      # 带命令行参数调试（后面阶段有用）
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
| `print 表达式` | `p sum+i` | 打印表达式结果 |
| `list` | `l` | 查看当前源码附近 |
| `info locals` | | 列出当前函数所有局部变量 |
| `backtrace` | `bt` | 查看函数调用栈 |
| `info breakpoints` | `i b` | 查看所有断点 |
| `delete 1` | `d 1` | 删除 1 号断点 |
| `watch 变量` | | 变量被修改时自动停下 |
| `finish` | | 运行到当前函数返回 |
| `quit` | `q` | 退出 gdb |

---

## 5. 完整示例

以一个求和的程序为例：

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

gdb 交互过程：

```
(gdb) break 6          # 在 sum += i 那行设断点
(gdb) run              # 运行，停在断点
Breakpoint 1, main () at sum.cpp:6
(gdb) print i          # 看 i 的值
(gdb) print sum        # 看 sum 的值
(gdb) next             # 单步执行一行
(gdb) print sum        # sum 变了
(gdb) continue         # 继续到循环下一轮（又停在断点）
(gdb) info locals      # 一次性看所有局部变量
(gdb) quit
```

---

## 6. 与 CLion 的关系

CLion 的「绿色小虫子 Debug」按钮，底层就是 gdb。你在 CLion 里：

- 点行号左边打红点 = `break`
- 按 Step Over = `next`
- 按 Step Into = `step`
- 调试面板看变量 = `print` / `info locals`

所以学会 gdb 命令，你就知道 CLion 调试器在背后干什么，命令行环境（比如服务器上）也能徒手调试。
