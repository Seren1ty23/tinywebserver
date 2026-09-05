# TinyWebServer 开发日志

> 按日期记录项目进展。配套：`PLAN.md`（12 阶段计划）、`MEMORY.md`（进度与踩坑笔记）。

## 2026-08-31 · 规划阶段

- 确定目标：对照 GitHub [qinguoyi/TinyWebServer](https://github.com/qinguoyi/TinyWebServer)，从零重写一个 Linux 下 C++ 轻量级 Web 服务器。
- 三条核心决策：
  1. **环境**：WSL2 + Ubuntu（不做 Windows 原生移植）
  2. **范围**：核心版 —— 线程池 + epoll + HTTP GET/POST + 定时器 + 日志（不含 MySQL）
  3. **方式**：对照重写（只看不抄，每阶段编译跑通再进下一步）
- 产出：`PLAN.md`（12 阶段迭代计划 + 每阶段前置知识点清单）、`MEMORY.md`（项目 memory）。

## 2026-09-01 · 环境搭建与项目初始化

- 安装 WSL2 + Ubuntu，发行版装到 `D:\Ubuntu`（用户 lu_yunhao）。
- **踩坑修复**：`wsl --install` 报 `Error 14098`（组件存储损坏）→ `DISM /Online /Cleanup-Image /RestoreHealth` + `sfc /scannow` + 重启 + 重新启用 `VirtualMachinePlatform`。
- 装工具链：`build-essential` + `git`（g++ 15.2 / make 4.4.1 / git 2.53）。
- 写 Hello World（`main.cpp`）；构建系统由 Makefile 切换为 **CMake**（CLion 原生支持，替代原版 Makefile）。
- 配置 CLion + WSL 工具链；补装 `cmake`（4.2.3）+ `gdb`（17.1）。
- **踩坑修复**：CLion「cmake 错误 127 / 调试器没找到」= WSL 内缺 cmake/gdb（`build-essential` 不含它们）。

## 2026-09-04 · 学习与阶段 0 收尾

- 整理速查文档：`linux-commands.md`（Linux 命令）、`gpp-gdb.md`（g++ 编译 + gdb 调试）。
- 学习 g++ 编译四步（预处理 → 编译 → 汇编 → 链接）与常用编译选项。
- gdb 实操（断点/单步/看变量）；**踩坑修复**：debuginfod 联网卡死 → `~/.gdbinit` 里 `set debuginfod enabled off` 永久关闭。
- git 实操：`init` → `add` → `commit`，root-commit `1656cf6`（7 files / 822 insertions）。
- **踩坑修复**：git 身份被自动推导为 `lu_yunhao@LAPTOP-PNBUG161.localdomain` → 显式设置 `user.name` / `user.email`（Gmail）。
- 生成 SSH 密钥（ed25519），公钥添加到 GitHub，测试认证通过（账号 `Seren1ty23`）。
- 推送本地仓库到 GitHub 远程（分支 `main`）。

**阶段 0（环境搭建）正式完成 ✅**

## 2026-09-05 · 阶段 1：文件 I/O 系统调用

- 学习文件描述符 fd（0/1/2 + open 返回 ≥3）、五个系统调用（open/read/write/close/lseek）、read 三种返回值、errno/perror。
- 写 `test/io_demo.cpp`（简化版 cat）：循环 `read` 到 EOF + 处理 `write` 部分写入。
- 命令行编译运行通过：`./io_demo PLAN.md` 正确打印全文。
- **踩坑修复**：把 `io_demo.cpp` 和 `main.cpp` 塞进同一个 CMake target → 链接报 `multiple definition of 'main'`；拆成两个 `add_executable` 解决。

**阶段 1（文件 I/O）正式完成 ✅**
