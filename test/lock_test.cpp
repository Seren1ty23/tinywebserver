#include "../lock/locker.h"
#include <pthread.h>
#include <iostream>
#include <ostream>

locker g_lock; // 全局互斥锁
int g_count = 0; // 全局计数器

void* worker(void*) {
    for (int i = 0; i < 1000000; ++i) {
        g_lock.lock();
        g_count++;
        g_lock.unlock();
    }
    return nullptr;
}

// 进程：正在运行的程序实例
// 线程：进程内的一条执行流
int main() {
    pthread_t t1,t2; // 线程句柄
    pthread_create(&t1,nullptr,worker,nullptr); // 创建线程1，跑worker
    pthread_create(&t2,nullptr,worker,nullptr); // 创建线程2，跑worker
    pthread_join(t1,nullptr); // 等线程1结束
    pthread_join(t2,nullptr); // 等线程2结束
    std::cout<<"count = "<<g_count<<std::endl;
}
// Created by Lu Yunhao on 2026/9/12.
//
