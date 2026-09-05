#include <fcntl.h> // 提供open()
#include <unistd.h> // 提供read(),wirte(),close()
#include <sys/types.h> // 提供read的返回类型
#include <cstdio> // 提供perror()
#include <iostream>

int main(int argc,char* argv[]) {
    // argc是命令行参数个数 argv[]是参数数组，[0]是程序名，[1]是第一个参数
    // 敲 ./io_demo PLAN.md → argc=2，argv[0]="./io_demo"，argv[1]="PLAN.md"

    // 参数检查，argc<2没传文件名，return 1 表示返回出错，cerr提示用法
    if (argc < 2) {
        std::cerr << "用法：" << argv[0] << "<文件名>" << std::endl;
        return 1;
    }

    int fd = open(argv[1],O_RDONLY); // O_RDONLY表示只读
    // -1 表示导入失败，perror返回原因
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 准备缓冲区
    char buf[4096];
    ssize_t n; // 有符号整数类型
    // 每次 read 最多读 4096 字节到 buf，返回值 n = 实际读到的字节数
    // n > 0：读到了数据 → 进循环体
    // n == 0：文件读到末尾（EOF）→ 退出循环
    // n == -1：出错 → 退出循环，交给后面处理
    // 为什么循环：文件可能比 4096 大，一次读不完，得反复读。
    while ((n = read(fd,buf,sizeof(buf))) > 0) {
        ssize_t off = 0; // off接收偏移量
        while (off < n) {
            ssize_t w =write(STDOUT_FILENO,buf + off,n - off); // 宏表示fd = 1，输出
            // write函数根据fd来写，此处fd = 1，就是输出到屏幕
            if (w == -1) {
                perror("write");
                close(fd);
                return 1;
            }
            off += w;
        }
    }
    // read出错
    if (n == -1) {
        perror("read");
        close(fd);
        return 1;
    }

    // 正常退出
    close(fd);
    return 0;
}
// Created by Lu Yunhao on 2026/9/5.
//
