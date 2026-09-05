# TinyWebServer 项目 Memory

> 本文件是本项目的持久记忆：进度、知识点笔记、决策、踩坑。每完成一个阶段就更新一次。计划见 `PLAN.md`，开发日志见 `Development Log.md`。

## 项目信息

- 目标：Linux 下 C++ 轻量级 Web 服务器（核心版，对照重写）
- 范围：线程池 + epoll + HTTP GET/POST + 定时器 + 日志（**不含 MySQL**）
- 环境：WSL2 + Ubuntu · `g++ -std=c++11` · CMake · CLion（WSL 工具链）
- GitHub：github.com/Seren1ty23/tinywebserver（SSH 已配通，分支 main）
- 蓝本：https://github.com/qinguoyi/TinyWebServer
- 方式：对照重写（只看不抄，每阶段编译跑通再进下一步）

## 进度

| 阶段 | 内容 | 状态 | 日期 | 备注 |
|-----|------|:---:|------|------|
| 0 | 环境搭建 | ☑ | 2026-09-04 | WSL2+CMake+CLion+git 首次提交 |
| 1 | 文件 I/O 系统调用 | ☑ | 2026-09-05 | io_demo（简化版 cat）跑通 |
| 2 | 进程/线程/同步封装（lock/） | ☐ | | |
| 3 | TCP echo server/client | ☐ | | |
| 4 | 单线程阻塞 HTTP 静态服务器 | ☐ | | |
| 5 | 非阻塞 + epoll 多路复用 | ☐ | | |
| 6 | Reactor + Epoller/连接类封装 | ☐ | | |
| 7 | HTTP 解析状态机（GET/POST） | ☐ | | |
| 8 | 线程池 | ☐ | | |
| 9 | 定时器（非活动连接） | ☐ | | |
| 10 | 日志系统（同步/异步） | ☐ | | |
| 11 | 整合 + 压测 + 优化 | ☐ | | |

## 知识点笔记（按阶段）

### 阶段 0：环境搭建
- WSL2 + Ubuntu（装在 D:\Ubuntu），默认用户 lu_yunhao；g++ 15.2 / cmake 4.2.3 / git 2.53 / gdb 17.1
- 踩坑1：`wsl --install` 报 `Error 14098`（组件存储损坏）→ `DISM /Online /Cleanup-Image /RestoreHealth` → `sfc /scannow` → 重启 → 重新 `Enable-Feature VirtualMachinePlatform`
- 踩坑2：CLion WSL 工具链报「cmake 错误127 / 调试器没找到」→ 是 WSL 里没装 cmake 和 gdb，`sudo apt install cmake gdb`
- 踩坑3：gdb 卡在 debuginfod 联网 → `set debuginfod enabled off`（写入 `~/.gdbinit`）
- 构建：CLion + CMake + WSL 工具链；CMakeLists 加了 `-Wall`；编译产物是 Linux ELF
- git：init → add → commit 跑通（root-commit）；身份用注册 GitHub 的 Gmail 邮箱显式设置（GitHub 按邮箱关联账号）；SSH 密钥配通并已推送 GitHub

### 阶段 1：文件 I/O 系统调用
- 核心：文件描述符 fd（0=stdin 1=stdout 2=stderr，open 返回 ≥3）；「一切皆文件」
- open/read/write/close/lseek 五个系统调用；read 三返回值（>0 读到 / =0 EOF / <0 出错看 errno）
- 踩坑：CMake 一个 target 只能有一个 main()，两个含 main 的 .cpp 塞同一 add_executable 会「multiple definition of main」→ 一个可执行文件 = 一个 add_executable
- 产出：test/io_demo.cpp（简化版 cat，循环 read 到 EOF + 处理部分写入）

## 决策记录

- 2026-09：范围定为「核心版」，砍掉 MySQL/注册登录，先跑通网络与并发主线
- 2026-09：阶段顺序按学习曲线重排（先 socket 再并发），而非原版源码目录顺序
- 2026-09-01：构建工具用 CMake（CLion 原生支持），替代原版的 Makefile
- 2026-09-04：git 提交身份用注册 GitHub 的 Gmail 邮箱（GitHub 按邮箱归属提交），配 SSH 推送

## 关键命令备忘

```bash
# 编译（CLion 里直接点运行即可）
cmake -B build && cmake --build build
# 运行
./build/server -p 9006 -t 8 -m 0 -l 0
# 压测
webbench -c 1000 -t 5 http://127.0.0.1:9006/
# 调试
gdb ./build/server    # 或 CLion 断点调试
```

## 面试 / 复盘问题清单

- [ ] epoll 为什么比 select/poll 高效？
- [ ] LT 和 ET 的区别？ET 为什么必须循环读到 EAGAIN？
- [ ] Reactor 和 Proactor 的区别？
- [ ] 半同步/半反应堆模型是怎么工作的？
- [ ] 线程池为什么能提升吞吐？任务队列如何避免竞态？
- [ ] HTTP 状态机为什么需要「停下等下次数据」的能力？
- [ ] 定时器升序链表为什么只检查头部即可？
- [ ] 同步日志和异步日志的区别？阻塞队列满时怎么办？
