# TinyWebServer 实现计划（核心版 · 对照重写）

> 学习型项目计划：以 GitHub [qinguoyi/TinyWebServer](https://github.com/qinguoyi/TinyWebServer) 为蓝本，从零按阶段重写，掌握 Linux 下的 C++ 网络编程。

**Goal:** 从「基本语法 + 数据结构」基础起步，最终实现一个 Linux 下可运行、可压测的轻量级 Web 服务器（核心版）。

**Architecture:** 线程池 + 非阻塞 socket + epoll（LT/ET）+ Reactor 事件模型 + HTTP 状态机解析 + 升序链表定时器处理非活动连接 + 同步/异步日志。并发模型为「半同步/半反应堆」：主线程负责 I/O 监听与事件分发，工作线程负责 HTTP 解析与响应。

**Tech Stack:** C++11 · Linux（WSL2 + Ubuntu）· g++ · CMake · git · epoll · pthread · HTTP/1.1

## 全局约束

- 环境：WSL2 + Ubuntu（推荐 Ubuntu 22.04/24.04），不做 Windows 原生移植（原版依赖 epoll）
- 编译标准：`g++ -std=c++11 -Wall -g`，用 CMake 构建（CLion 原生支持）
- 范围：核心版 —— 线程池 + epoll + HTTP GET/POST + 定时器 + 日志；**不含 MySQL / 注册登录 / 数据库连接池**（作为后续扩展，见文末「扩展方向」）
- 方式：对照重写 —— 原版代码只作参考，自己按阶段从零实现，每阶段编译跑通再进入下一阶段；禁止整段拷贝
- 所有文件（含本计划与项目 memory）都放在 `tinywebserver/` 目录下
- 每完成一个阶段：`git commit` 一次，并在 `MEMORY.md` 记录本阶段知识点与踩坑

## 目录结构（最终形态）

```
tinywebserver/
├── PLAN.md              # 本计划
├── MEMORY.md            # 项目 memory（学习记录/决策/踩坑）
├── CMakeLists.txt       # 构建脚本（CMake）
├── main.cpp             # 入口：解析参数、组装模块、启动
├── config.h             # 配置常量（端口/线程数/超时/日志开关等）
├── lock/
│   └── locker.h         # 互斥锁/条件变量/信号量的 RAII 封装
├── threadpool/
│   └── threadpool.h     # 半同步半反应堆线程池
├── http/
│   ├── http_conn.h      # HTTP 连接类：状态机解析 GET/POST
│   └── http_conn.cpp
├── timer/
│   ├── lst_timer.h      # 升序链表定时器，处理非活动连接
│   └── lst_timer.cpp
├── log/
│   ├── log.h            # 同步/异步日志（单例）
│   ├── log.cpp
│   └── block_queue.h    # 阻塞队列（异步日志用）
├── root/                # 静态资源：index.html、图片、视频
└── test/                # 自写测试与压测脚本
```

阶段 1~10 按顺序创建这些文件，**不必一开始就建好目录**。

## 迭代路线图总览

| 阶段 | 交付物 | 关键前置 |
|-----|--------|---------|
| 0 | 环境 + Hello World + git 仓库 | 无 |
| 1 | 文件 I/O 系统调用 demo | 文件描述符 |
| 2 | `lock/` 线程同步封装 | 进程/线程/锁 |
| 3 | TCP echo server/client | socket API |
| 4 | 单线程阻塞 HTTP 静态服务器 | HTTP 报文 |
| 5 | epoll 非阻塞多连接 | I/O 多路复用 |
| 6 | Reactor + Epoller/连接类封装 | 回调/封装 |
| 7 | HTTP 解析状态机（GET/POST） | 状态机 |
| 8 | 线程池 | 生产者消费者 |
| 9 | 定时器（非活动连接） | 链表/时间 |
| 10 | 日志系统（同步/异步） | 阻塞队列/单例 |
| 11 | 整合 + 压测 + 优化 | 全部 |

> 说明：阶段顺序按「学习曲线」重排，与原版源码目录顺序（lock→threadpool→http→timer→log）不同，但最终模块一一对应。建议先把原版仓库 clone 到 `tinywebserver/` 之外（如 `~/reference/TinyWebServer`）作为对照，只看不抄。

---

## 阶段 0：环境搭建

**目标：** 装好 WSL2 + Ubuntu 工具链，能编译运行 C++ 程序，建好 git 仓库。

**前置知识点学习清单：**
- [x] WSL2 安装：`wsl --install -d Ubuntu`（PowerShell 管理员运行），重启后设置用户名密码
- [x] Linux 常用命令：`cd / ls / mkdir / touch / cp / mv / rm / cat / apt`
- [x] g++ 编译四步：预处理 → 编译 → 汇编 → 链接；`g++ -o main main.cpp`
- [x] 常用编译选项：`-std=c++11`（标准）、`-Wall`（警告）、`-g`（调试符号）、`-O2`（优化）
- [x] CMake 基础：`CMakeLists.txt` 最小结构（`cmake_minimum_required / project / add_executable`）
- [x] git 基础：`init / add / commit / log / status / diff`
- [x] gdb 基础：`break` 断点、`run`、`next` 单步、`print` 看变量（后续调试常用）

**要写的代码：**
- `main.cpp`：输出 `Hello TinyWebServer`
- `CMakeLists.txt`：5 行最小配置，声明可执行目标

**验证：**
```bash
sudo apt install -y cmake gdb      # cmake 构建 + gdb 调试（CLion 的 WSL 工具链也需要）
cmake -B build && cmake --build build && ./build/main   # 输出 Hello TinyWebServer（CLion 里直接点运行即可）
git init && git add . && git commit -m "stage0: env setup"
```

**完成标准：** 能编译、能运行、git 仓库可用。编辑器用 CLion（配置 WSL 工具链）。

---

## 阶段 1：Linux 系统调用与文件 I/O

**目标：** 理解「一切皆文件」与文件描述符，能用系统调用读写文件。

**前置知识点学习清单：**
- [x] 文件描述符 fd（0/1/2 = stdin/stdout/stderr，`open` 返回新 fd）
- [x] `open / read / write / close / lseek` 各参数与返回值
- [x] `errno` 与错误处理：`perror()` / `strerror(errno)`
- [x] 用户态缓冲区 vs 内核态缓冲区；`read` 返回值 < 0 / = 0（EOF） / > 0 三种情况
- [x] `man 2 open` 查看手册的方法

**要写的代码：**
- `test/io_demo.cpp`：实现一个简化版 `cat`——`open` 文件，循环 `read` 到缓冲区，`write` 到 stdout

**关键实现要点：**
- `read` 循环读到返回 0 为止，不能一次 `read` 就假设读完了
- 每次 `write` 检查返回值，处理部分写入（`write` 返回值可能小于请求写入的字节数）

**验证：**
```bash
g++ -std=c++11 -o io_demo test/io_demo.cpp
./io_demo PLAN.md   # 应原样输出本文件内容
```

**完成标准：** demo 能正确输出任意文件内容，理解了 fd 和 `read/write` 返回值的三种含义。

---

## 阶段 2：进程、线程与同步原语（lock/）

**目标：** 掌握并发基础，实现原版 `lock/` 目录的线程同步封装类。

**前置知识点学习清单：**
- [ ] 进程 vs 线程；`fork / wait`；`pthread_create / pthread_join`
- [ ] 竞态条件、临界区、死锁
- [ ] 互斥锁：`pthread_mutex_init / lock / unlock`
- [ ] 条件变量：`pthread_cond_init / wait / signal / broadcast`（`wait` 必须配合锁）
- [ ] 信号量：`sem_init / sem_wait / sem_post`
- [ ] RAII 思想：构造时加锁、析构时解锁

**要写的代码：**
- `lock/locker.h`：三个 RAII 封装类
  - `sem`：封装信号量，`wait()` / `post()`
  - `locker`：封装互斥锁，`lock()` / `unlock()`
  - `cond`：封装条件变量，`wait()` / `signal()` / `broadcast()`

**关键实现要点：**
- 条件变量 `wait` 前必须先持有锁，唤醒后重新获得锁
- `cond::wait()` 内部实现：`pthread_cond_wait(&m_cond, m_mutex)`，注意与「先解锁再等待再上锁」的语义对应
- 析构函数中调用 `pthread_*_destroy` 释放资源

**验证：**
- `test/lock_test.cpp`：多线程各加 100 万次计数器，最终结果 = 200 万（无竞态）
- 生产者-消费者测试：一个线程生产、一个线程消费，用条件变量/信号量协调，程序不死锁、不丢数据

**完成标准：** 两个测试都通过；能讲清楚「互斥锁保证互斥，条件变量/信号量保证同步」的区别。

---

## 阶段 3：Socket 网络编程基础（TCP echo）

**目标：** 写出第一个网络程序：阻塞版 TCP 回显服务器 + 客户端。

**前置知识点学习清单：**
- [ ] TCP/IP 分层；三次握手（SYN/SYN-ACK/ACK）、四次挥手
- [ ] 服务端流程：`socket → bind → listen → accept → read/write → close`
- [ ] 客户端流程：`socket → connect → read/write → close`
- [ ] `sockaddr_in` 结构：`sin_family / sin_port / sin_addr`；`htons / htonl / ntohs`（网络字节序）
- [ ] `listen` 的 backlog 参数；`accept` 返回新连接的 fd
- [ ] 阻塞 I/O 的含义：`accept/read` 没数据时挂起

**要写的代码：**
- `test/echo_server.cpp`：监听端口，`accept` 后循环 `read`，把收到的数据 `write` 回去
- `test/echo_client.cpp`：`connect` 后从 stdin 读一行发出去，打印回显

**关键实现要点：**
- `bind` 前设置 `SO_REUSEADDR`（`setsockopt`），避免重启时「Address already in use」
- 客户端关闭后服务端 `read` 返回 0，要关闭连接并继续 `accept`

**验证：**
```bash
./echo_server 9000 &
./echo_client 127.0.0.1 9000    # 输入 hello，收到 hello
# 再开一个 echo_client，观察第二个连接是否被处理
```

**完成标准：** 回显正常；能说出一次 TCP 连接的完整生命周期。此阶段只支持「一次一个连接」，属正常现象，阶段 5 解决。

---

## 阶段 4：单线程阻塞 HTTP 静态服务器

**目标：** 让浏览器/curl 能访问到服务器返回的静态页面。

**前置知识点学习清单：**
- [ ] HTTP/1.1 请求报文格式：请求行（`GET /index.html HTTP/1.1`）+ 请求头 + 空行 + 请求体
- [ ] HTTP 响应报文格式：状态行（`HTTP/1.1 200 OK`）+ 响应头 + 空行 + 响应体
- [ ] 常见状态码：200 / 400（Bad Request）/ 404（Not Found）/ 500
- [ ] 关键响应头：`Content-Type`（MIME 类型）、`Content-Length`
- [ ] 常见 MIME：`text/html`、`image/jpeg`、`image/png`、`video/mp4`

**要写的代码：**
- `root/index.html`：一个简单首页
- `test/http_static.cpp`：`read` 请求 → 提取 URL → `open` 对应文件 → 返回「状态行 + 头 + 文件内容」

**关键实现要点：**
- 此阶段不要求完整解析，先处理「收到 `GET /xxx` 就返回对应文件」的最简情况
- 响应头必须以 `\r\n` 结尾，头与体之间是空行 `\r\n\r\n`
- `Content-Length` 用文件大小（`stat` 或 `lseek` 到末尾获取）
- 文件不存在返回 404 页面

**验证：**
```bash
./http_static 9006
# 浏览器访问 http://localhost:9006/ 看到 index.html
curl -i http://localhost:9006/            # 观察完整响应头
curl -i http://localhost:9006/nonexist    # 应返回 404
```

**完成标准：** 浏览器能看到页面、curl 能看到正确的状态码与头部。依然一次只处理一个连接。

---

## 阶段 5：非阻塞 I/O + epoll 多路复用

**目标：** 让服务器在单线程下同时服务多个连接，这是本项目的核心分水岭。

**前置知识点学习清单：**
- [ ] 阻塞 vs 非阻塞：非阻塞 `read` 无数据时返回 -1 且 `errno == EAGAIN/EWOULDBLOCK`
- [ ] I/O 多路复用思想：select / poll / epoll 的对比（为什么 epoll 快）
- [ ] epoll 三个接口：`epoll_create1 / epoll_ctl(ADD/MOD/DEL) / epoll_wait`
- [ ] 事件类型：`EPOLLIN / EPOLLOUT / EPOLLET / EPOLLONESHOT`
- [ ] LT（水平触发）vs ET（边缘触发）的区别与使用要点
- [ ] `fcntl(fd, F_SETFL, O_NONBLOCK)` 设置非阻塞

**要写的代码：**
- 改造阶段 4：`test/epoll_server.cpp`——`listenfd` 设为非阻塞加入 epoll，`epoll_wait` 循环处理多个连接

**关键实现要点：**
- ET 模式必须在一次事件里循环 `read` 到 `EAGAIN` 为止，否则会丢数据
- LT 模式可只读一次，作为起步先用 LT 跑通，再切换 ET 理解差异
- 每连接需要自己的读缓冲（用 `std::map<int, std::string>` 或结构体存 fd→缓冲区）

**验证：**
```bash
./epoll_server 9006
# 同时开 3 个终端 curl 或多次请求，应都能响应，不互相阻塞
```

**完成标准：** 单线程能并发处理多连接；能口头讲清 LT 与 ET 的区别、为什么 ET 要循环读到 EAGAIN。

---

## 阶段 6：Reactor 模型 + 事件与连接封装

**目标：** 把阶段 5 的裸 epoll 代码封装成可扩展的事件驱动框架。

**前置知识点学习清单：**
- [ ] Reactor 模型：主线程只监听事件并分发（I/O 与业务分离）
- [ ] Proactor 模型：由内核/主线程完成读写，工作线程只处理数据（本阶段先理解概念，原版两种都实现）
- [ ] 回调函数 / 函数对象 / `std::function` / `std::bind`
- [ ] C++ 类设计：封装 epoll 操作、封装单个连接的状态

**要写的代码：**
- `http/http_conn.h`（初版）：连接类，含 `sockaddr_in` 地址、读缓冲、写缓冲、`read_once / write / process`
- 新增 `Epoller` 封装（可先放 `http/` 或单独 `web/`，最终随 main 整合）：`add_fd / del_fd / mod_fd / wait`

**关键实现要点：**
- 用 `epoll_event.data.fd` 或 `data.ptr` 关联 fd 与连接对象（`data.ptr` 指向对象是更通用的做法）
- 连接对象持有自己的读/写缓冲区，避免全局 map
- 事件到达 → 回调 `read_once` 读数据 → 交给后续阶段的解析逻辑

**验证：** 功能与阶段 5 等价（多连接可响应），但代码结构清晰、可继续扩展。用 `clang-format` 或手动保持格式统一。

**完成标准：** 能解释「事件循环 + 回调」如何工作；连接对象自包含缓冲。

---

## 阶段 7：HTTP 解析状态机（GET + POST）

**目标：** 完整解析 HTTP 请求报文，正确区分 GET 与 POST，返回正确资源。

**前置知识点学习清单：**
- [ ] 有限状态机（FSM）思想：状态 + 输入 → 迁移
- [ ] 主状态机：解析请求行 → 解析请求头 → 解析请求体（POST）
- [ ] 从状态机：`LINE_OK / LINE_BAD / LINE_OPEN` 逐行读取
- [ ] HTTP 方法 GET / POST；POST 的 `Content-Length` 决定请求体长度
- [ ] 连接管理：`Connection: keep-alive`（长连接）vs `close`
- [ ] C++ 字符串处理：`string::find / substr / compare`、`strpbrk`

**要写的代码：**
- `http/http_conn.h` + `http/http_conn.cpp`：完整实现
  - 状态枚举：`CHECK_STATE_REQUESTLINE / CHECK_STATE_HEADER / CHECK_STATE_CONTENT`
  - `parse_request_line / parse_headers / parse_content`（提取方法、URL、版本、Content-Length、Host）
  - `do_request`：生成响应（GET 返回文件，POST 暂回 200 或解析后回显，不含数据库）

**关键实现要点：**
- 一行结束标志是 `\r\n`；`read_once` 后可能只读到半行，状态机要能「停下等下次数据」（`LINE_OPEN`）
- 从状态机负责「读完整一行」，主状态机负责「这一行是什么」
- 请求体长度必须依据 `Content-Length`，不能靠缓冲区大小猜
- 响应头部：`Content-Length` 必须与实际响应体字节数一致，否则浏览器会挂起

**验证：**
```bash
cmake --build build && ./build/server 9006
curl -i http://localhost:9006/              # GET 200
curl -i -X POST -d "a=1&b=2" http://localhost:9006/login   # POST 解析正常
# 上传一个 1MB 图片到 root/，用浏览器/curl 访问验证 Content-Length 正确
```

**完成标准：** GET/POST 都能正确解析；大文件传输 `Content-Length` 正确；长连接下连续多个请求不串数据。

---

## 阶段 8：线程池（半同步/半反应堆）

**目标：** 把耗时的业务处理（HTTP 解析 + 响应）交给工作线程，主线程只做 I/O 分发。

**前置知识点学习清单：**
- [ ] 生产者-消费者模型：主线程生产「就绪连接任务」，工作线程消费
- [ ] 线程池结构：N 个常驻线程 + 一个任务队列
- [ ] 用条件变量 + 互斥锁实现「队列空则等待、有任务则唤醒」
- [ ] C++ 模板、函数对象作为任务载体（`std::queue<std::function<void()>>`）

**要写的代码：**
- `threadpool/threadpool.h`：模板类
  - 构造函数创建 N 个 `pthread`（或 `std::thread`），`worker()` 循环从队列取任务执行
  - `append(T* request)`：主线程把「连接处理函数」加入队列并唤醒一个线程
  - `run()`：工作线程执行 `request->process()`

**关键实现要点：**
- 线程池析构时需通知所有线程退出并 `join`，避免泄漏
- 半同步半反应堆：主线程（epoll）是异步的「反应堆」，业务处理是同步的「工作线程」，两者靠任务队列衔接
- 先跑通「正确性」，再对比阶段 7 单线程的性能提升

**验证：**
```bash
./server 9006 -t 8          # 8 个工作线程
# 用 webbench 或 curl 并发压测，对比单线程版吞吐
```

**完成标准：** 多线程下无死锁、无数据竞争；并发吞吐明显高于单线程版。

---

## 阶段 9：定时器（升序链表处理非活动连接）

**目标：** 自动关闭长时间空闲的连接，回收资源。

**前置知识点学习清单：**
- [ ] 升序链表：按「超时时间」排序，每次只需检查头部
- [ ] 时间获取：`time(NULL)` / `gettimeofday` / `clock_gettime`
- [ ] 定时触发：`alarm()` + `SIGALRM` 信号（原版做法），或利用 `epoll_wait` 的超时参数
- [ ] 非活动连接：一段时间没读写就关闭，防止资源被占满

**要写的代码：**
- `timer/lst_timer.h` + `timer/lst_timer.cpp`：
  - 定时器节点：`expire`（绝对超时时间）+ 回调函数 + 前后指针
  - `add_timer / adjust_timer / del_timer / tick`
- 在连接类中：每次收到数据就 `adjust_timer` 刷新超时；`tick()` 遍历删除并关闭超时连接

**关键实现要点：**
- 链表按 expire 升序，`tick()` 从头部开始，遇到未超时即可停止（优化）
- SIGALRM 信号处理函数要短小（只设置标志位或直接 tick），避免在信号里做重操作
- 关闭连接要同时 `del_timer` + `epoll_ctl DEL` + `close(fd)`

**验证：**
```bash
./server 9006
# curl 连上后挂起 60 秒不请求，观察日志/连接被自动关闭
```

**完成标准：** 空闲连接在设定超时后被关闭；活跃连接不被误杀。

---

## 阶段 10：日志系统（同步/异步）

**目标：** 记录服务器运行状态，支持同步写入与异步写入（阻塞队列 + 后台线程）。

**前置知识点学习清单：**
- [ ] 阻塞队列：队列满时生产者等待、队列空时消费者等待
- [ ] 单例模式：懒汉式 + 线程安全（`static` 局部变量）
- [ ] 可变参数格式化：`va_list / va_start / va_end` 或 C++ 可变参数模板
- [ ] 异步日志：一个后台线程从队列取日志写文件，主线程只 `push`

**要写的代码：**
- `log/block_queue.h`：线程安全阻塞队列
- `log/log.h` + `log/log.cpp`：单例日志类
  - `init(path, mode, max_queue_size)`：同步（直接写）或异步（队列 + 线程）
  - `write_log(level, format, ...)`：带级别（DEBUG/INFO/WARN/ERROR）与时间戳
  - 用宏 `LOG_INFO(format, ...)` 简化调用

**关键实现要点：**
- 同步模式：加锁直接写文件；异步模式：写队列，后台线程落盘
- 异步队列满时的策略：原版是阻塞等待；可先阻塞，跑通后再优化
- 文件写入用 `fputs` 或 `write`，注意换行与 flush

**验证：**
```bash
./server 9006 -l 0    # 同步日志，观察 log 文件
./server 9006 -l 1    # 异步日志，观察 log 文件且性能更好
cat log 文件           # 能看到带时间戳和级别的日志行
```

**完成标准：** 同步/异步两种模式都正确落盘；异步模式压测时吞吐更高（因为主线程不阻塞写盘）。

---

## 阶段 11：整合、压测与优化

**目标：** 把全部模块组装成完整服务器，能压测，能做基础调优。

**前置知识点学习清单：**
- [ ] 命令行参数解析：`getopt`（原版 `-p/-t/-m/-l/-c/-o/-a`）
- [ ] 压测工具：webbench（原版自带）或 `wrk` / `ab`
- [ ] 性能观测：`top` 看 CPU、`strace -c` 看系统调用耗时、`perf`（进阶）
- [ ] `-O2` 优化、关闭日志对性能的影响

**要写的代码：**
- `config.h`：端口、线程数、触发模式、超时时间、日志开关等常量
- `main.cpp`：解析参数 → 初始化日志/线程池/监听 → 事件循环
- `CMakeLists.txt`：一键编译全项目（列出所有源文件）
- `test/`：压测脚本与说明

**关键实现要点：**
- 组装顺序：解析参数 → 初始化日志 → 创建线程池 → 建立监听 fd → epoll 主循环
- 触发模式（listenfd/connfd 的 LT/ET 组合）用配置项控制，便于对比
- 优雅关闭连接（`SO_LINGER`，`-o` 参数）作为可选加分项

**验证：**
```bash
cmake --build build && ./build/server -p 9006 -t 8 -m 0 -l 0
# 浏览器 + curl + webbench 压测：
webbench -c 1000 -t 5 http://127.0.0.1:9006/
```

**完成标准：** 完整服务器稳定运行，压测无崩溃；能看懂 QPS/并发数，能说清关闭日志、`-O2`、ET 模式分别带来什么收益。

---

## Memory 使用说明

- 每完成一个阶段，在 `MEMORY.md` 的进度表打勾，并在「知识点笔记」里用一两句话记下：新概念、踩过的坑、解决办法
- 「决策记录」记架构选择（如：为什么先 Reactor 再 Proactor、为什么核心版砍掉 MySQL）
- 「面试/复盘问题清单」持续补充，作为最后复习提纲

## 学习资源

- 游双《Linux 高性能服务器编程》—— 本项目蓝本，**核心必读**（对应 epoll/线程池/定时器/日志各章）
- 《UNIX 环境高级编程》(APUE) —— 阶段 1、2 系统编程参考
- 《UNIX 网络编程》卷 1 (UNP) —— 阶段 3 socket 权威参考
- 《TCP/IP 详解》卷 1 —— 阶段 3、4 协议细节
- 陈硕《Linux 多线程服务端编程》(muduo) —— 阶段 6、8 进阶（Reactor/线程池）
- 原版「庖丁解牛」系列文章（README 中链接）—— 每阶段完成后对照阅读
- 小林 coding「图解网络」—— HTTP/TCP 入门图解，适合零基础
- man 手册：`man 2 socket`、`man 7 epoll`、`man 2 fcntl`、`man 3 pthread`

## 扩展方向（完成核心版之后）

1. MySQL 注册/登录 + 数据库连接池（原版 `CGImysql/`，需先学 SQL 基础 + MySQL C API）
2. Proactor 模型（主线程完成读写）与 Reactor 并跑对比
3. 支持更多 HTTP 特性：断点续传（Range）、分块传输编码、gzip
4. 用 C++11 特性（`std::thread`、智能指针）重写 pthread 部分
