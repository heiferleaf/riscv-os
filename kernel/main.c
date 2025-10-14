#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void
main()
{
    printf("main\n");
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    kinit();         // 启用页式管理
    kvminit();       // 创建内核页表并映像内核部分
    kvminithart();   // 把页表设置为内核页表
    trapinit();      // 注册中断处理函数
    trapinithart();  // 注册中断向量表

    procinit();      // 初始化进程表

    // 创建根测试进程
    struct proc *p = allocproc();
    initproc = p;
    safestrcpy(p->name, "initproc", sizeof(p->name));
    // 可以设置 trapframe 的入口点为 test_entry
    // p->trapframe->epc = (uint64)test_entry; // 如果支持epc
    // 或直接把 test_entry 放在 forkret 或进程调度后调用
    acquire(&p->lock);
    p->state = RUNNABLE;
    release(&p->lock);

    // 启动调度器
    scheduler();

    // 不再需要裸 while(1){}
}