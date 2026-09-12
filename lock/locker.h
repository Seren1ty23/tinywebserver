#ifndef LOCKER_H
#define LOCKER_H
// 头文件保护，防止重复定义

#include <pthread.h> // 线程库
#include <semaphore.h> // 信号量
#include <exception>

// 信号量封装
class sem {
    public:
    sem() {
        if (sem_init(&m_sem,0,0) != 0) {
            throw std::exception();
        }
    }
    sem(int num) {
        if (sem_init(&m_sem,0,num) != 0) {
            throw std::exception();
        }
    }
    ~sem() {
        sem_destroy(&m_sem);
    }

    // sem_wait()与sem_post()控制资源并发数量
    bool wait() {
        return sem_wait(&m_sem) == 0;
        // sem_wait()：计数 > 0 就减一；否则阻塞等待
    }
    bool post() {
        return sem_post(&m_sem) == 0;
        // sem_post()：计数 +1，并唤醒一个等待者
    }
    private:
    sem_t m_sem; // sem_t 信号量类型
};

// 互斥锁封装
// 互斥锁保证同一个时刻只有一个线程进入临界区
class locker {
    public:
    locker() {
        if (pthread_mutex_init(&m_mutex,nullptr) != 0) {
            // 函数用来初始化互斥锁，非零表示出错
            throw std::exception();
        }
    }
    ~locker() {
        pthread_mutex_destroy(&m_mutex);
        // 销毁锁
    }

    // lock与unlock保证同一时刻只有一个线程在临界区
    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
        // 加锁 被占就阻塞，直到对方解锁
    }
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
        // 解锁
    }
    pthread_mutex_t* get() {
        return &m_mutex;
    }
    private:
    pthread_mutex_t m_mutex;
};

// 条件变量封装
class cond {
    public:
    cond() {
        if (pthread_cond_init(&m_cond,nullptr) != 0) {
            // 函数用来初始化条件变量
            throw std::exception();
        }
    }
    ~cond() {
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t* m) {
        return pthread_cond_wait(&m_cond,m) == 0;
        // pthread_cond_wait():
        // 1.释放锁m
        // 2.挂起当前进程，睡，等别人signal/broadcast
        // 3.被唤醒后。重新获得锁m
    }
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
        // 唤醒一个等待的线程
    }
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
        // 唤醒所有等待的线程
    }
    private:
    pthread_cond_t m_cond;
};

#endif
// Created by Lu Yunhao on 2026/9/12.
//