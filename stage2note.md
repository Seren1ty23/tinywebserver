# 阶段 2 笔记：进程/线程与同步原语（lock/）

> 阶段 2 前置知识 + 实践要点。核心：掌握并发基础，实现 `lock/` 的三个同步封装类。
> 配套：`PLAN.md`（计划）、`MEMORY.md`（进度）、`Development Log.md`（日志）。

## 1. 进程 vs 线程

| | 进程 process | 线程 thread |
|--|------|------|
| 创建 | `fork()` | `pthread_create()` |
| 内存 | **独立**内存空间 | **共享**进程内存（堆/全局/文件） |
| 开销 | 大（隔离/复制资源） | 小（共享资源） |
| 通信 | 麻烦，需 IPC（管道/共享内存/socket） | 简单，直接读写共享变量（**但要加锁**） |
| 隔离性 | 强（互不影响） | 弱（一个崩，整进程可能崩） |
| 切换速度 | 慢 | 快 |

- **进程** = 一栋独立房子（各家水电独立，互不干扰）
- **线程** = 同一栋房子里的多个房间/人（共享客厅厨房，通信方便但易抢东西）
- 服务器用**线程**：共享内存、开销小；代价是要自己处理同步。

## 2. 竞态条件 / 临界区 / 死锁

- **竞态条件（race condition）**：多线程并发访问同一份共享数据、且至少一个在写时，最终结果**取决于执行顺序**。
  - 根因：很多操作「看似一步，实则多步」（非原子）。如 `count++` 实为「读 → 加 1 → 写回」三步。
  - 演示：两线程各加 100 万次，可能读到同一旧值、各自 +1 写回，丢更新 → 结果 < 200 万。
- **临界区**：访问共享资源的代码段，要加锁保护，同一时刻只能一个线程进。
- **死锁**：两线程各持一把锁、又等对方那把锁，都卡死。
- 发生竞态的 3 条件：① 多线程 ② 同一份共享数据 ③ 至少一个在写（只读不用锁）。

## 3. 互斥锁 mutex（保证「互斥」）

- 一把「同一时刻只允许一个线程持有」的锁，保护临界区。
- 二值（锁定/未锁定）；抢不到就**阻塞等待**；**只保证互斥、不保证顺序**。

```c
pthread_mutex_t m;
pthread_mutex_init(&m, NULL);   // 初始化
pthread_mutex_lock(&m);         // 加锁：抢到才往下走，抢不到阻塞等
// ── 临界区 ──
pthread_mutex_unlock(&m);       // 解锁
pthread_mutex_destroy(&m);      // 销毁
```

## 4. 条件变量 cond（保证「同步」）

- 让线程「**等某个条件成立**」再继续；解决忙等（`while(空){}` 烧 CPU）。
- **必须配合互斥锁使用**。

```c
pthread_cond_t c;
pthread_cond_init(&c, NULL);
pthread_cond_wait(&c, &m);      // 等：①释放锁 ②挂起 ③被唤醒后重新获得锁（三步原子）
pthread_cond_signal(&c);        // 唤醒一个等待者
pthread_cond_broadcast(&c);     // 唤醒所有等待者
pthread_cond_destroy(&c);
```

- `wait` 为什么要配锁：让「检查条件 + 睡下」不可分割，防止「错过唤醒」。
- **标准写法用 `while` 而非 `if`**（防假唤醒）：
```cpp
m_mutex.lock();
while (!条件成立) m_cond.wait(m_mutex.get());
// 条件成立，干活...
m_mutex.unlock();
```
- `signal` 只叫醒一个（一个资源够一个消费者）；`broadcast` 叫醒所有（条件一变所有等待者都要重检）。

## 5. 信号量 sem（计数版同步）

- 本质：一个**非负整数计数器 + 两个原子操作**。

```c
sem_t s;
sem_init(&s, 0, N);   // 计数初始 = N
sem_wait(&s);         // 计数 > 0 则减 1；否则阻塞等
sem_post(&s);         // 计数 +1，唤醒一个等待者
sem_destroy(&s);
```

- 用途：控制并发/资源数量（如「最多 N 个连接」）。
- 计数设为 1 → 等价一把互斥锁（二值信号量）。

## 6. 三者区别与关系

| | 互斥锁 mutex | 条件变量 cond | 信号量 sem |
|--|-------------|--------------|-----------|
| 解决 | 互斥（别一起改） | 同步（等条件） | 同步（控并发数） |
| 计数 | 二值 0/1 | 无 | 有（可 N） |
| 配锁 | 自己够用 | **必须配锁** | 独立 |
| 记忆 | — | **无**（signal 没人等就丢） | **有**（post 会记住） |
| 回答 | 「能进临界区吗」 | 「该谁动」 | 「还有名额吗」 |

**关系**：`cond` 依赖 `locker`（`pthread_cond_wait` 需要锁指针，靠 `locker::get()` 传）；`sem` 独立。

- 类比：mutex = 单人房门锁；cond = 房间里的「等通知」；sem = 停车场空位计数牌。
- 口诀：**mutex 保证互斥，cond/sem 保证同步**。

## 7. RAII 封装（本阶段重点）

RAII = 资源获取即初始化：把「初始化/销毁」绑到对象的「构造/析构」。

- 构造时 `init`，析构时 `destroy`
- 好处：即使提前 `return` 或抛异常，析构也**一定**执行，不会忘清理

`lock/locker.h` 就是把裸 pthread 函数封装成三个 RAII 类。

## 8. 参考实现 `lock/locker.h`

```cpp
#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

// 信号量封装
class sem {
public:
    sem()          { if (sem_init(&m_sem, 0, 0)   != 0) throw std::exception(); }
    sem(int num)   { if (sem_init(&m_sem, 0, num) != 0) throw std::exception(); }
    ~sem()         { sem_destroy(&m_sem); }
    bool wait()    { return sem_wait(&m_sem) == 0; }
    bool post()    { return sem_post(&m_sem) == 0; }
private:
    sem_t m_sem;
};

// 互斥锁封装
class locker {
public:
    locker()       { if (pthread_mutex_init(&m_mutex, nullptr) != 0) throw std::exception(); }
    ~locker()      { pthread_mutex_destroy(&m_mutex); }
    bool lock()    { return pthread_mutex_lock(&m_mutex)   == 0; }
    bool unlock()  { return pthread_mutex_unlock(&m_mutex) == 0; }
    pthread_mutex_t* get() { return &m_mutex; }   // 给条件变量用
private:
    pthread_mutex_t m_mutex;
};

// 条件变量封装
class cond {
public:
    cond()         { if (pthread_cond_init(&m_cond, nullptr) != 0) throw std::exception(); }
    ~cond()        { pthread_cond_destroy(&m_cond); }
    bool wait(pthread_mutex_t* m) { return pthread_cond_wait(&m_cond, m) == 0; }
    bool signal()    { return pthread_cond_signal(&m_cond)    == 0; }
    bool broadcast() { return pthread_cond_broadcast(&m_cond) == 0; }
private:
    pthread_cond_t m_cond;
};

#endif
```

**设计要点**：
- 三个类都是 RAII；参数统一 `bool` 返回（`== 0` = 成功）
- `locker::get()` 把内部锁指针暴露给 `cond::wait`（条件变量必须配锁）
- `NULL` 可统一写 `nullptr`（C++11，类型更安全）

## 9. 验证 `test/lock_test.cpp`

两线程各加 100 万次，最终应为 200 万：

```cpp
#include "../lock/locker.h"
#include <pthread.h>
#include <iostream>

locker g_lock;
int g_count = 0;

void* worker(void*) {
    for (int i = 0; i < 1000000; ++i) {
        g_lock.lock();
        g_count++;
        g_lock.unlock();
    }
    return nullptr;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, worker, nullptr);
    pthread_create(&t2, nullptr, worker, nullptr);
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    std::cout << "count = " << g_count << std::endl;   // 应为 2000000
}
```

编译（**必须加 `-pthread` / `-lpthread`**）：
```bash
g++ -std=c++11 -Wall -pthread -o lock_test test/lock_test.cpp
./lock_test        # count = 2000000
```

**去掉 `lock`/`unlock`** → 结果会 < 200 万（竞态丢更新），这就是锁的必要性。

## 10. 注意：pthread 要链接库

`pthread_*` 在 `libpthread` 里，编译**必须加 `-lpthread` 或 `-pthread`**。CMake 里用：
```cmake
find_package(Threads REQUIRED)
target_link_libraries(lock_test Threads::Threads)
```
不加会报 `undefined reference to pthread_create`。

## 11. 踩坑

- **同一 target 只能有一个 `main()`**（延续阶段 1）：`lock_test.cpp` 要单独一个 `add_executable(lock_test test/lock_test.cpp)` target。
- **忘记 `-pthread`**：链接报 `undefined reference to pthread_*`。
- **条件变量忘配锁 / 用 `if` 而非 `while`**：假唤醒或丢唤醒，逻辑出错。
