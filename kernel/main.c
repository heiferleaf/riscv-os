#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void
main()
{
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    kinit();         // 启用页式管理
    kvminit();       // 创建内核页表并映像内核部分
    kvminithart();   // 把页表设置为内核页表
    trapinit();      // 注册中断处理函数
    trapinithart();  // 注册中断向量表
    procinit();      // 初始化进程表

    // 创建一个用户进程
    userinit();      // 第一个用户进程

    printf("main\n");

    // 启动调度器
    scheduler();

    // 不再需要裸 while(1){}
}