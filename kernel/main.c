#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void
main()
{
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    kinit();         // 启用页式管理
    kvminit();       // 创建内核页表并映像内核部分
    kvminithart();   // 把页表设置为内核页表
    trapinit();      // 注册中断处理函数
    trapinithart();  // 注册中断向量表

    test_kernel_timer_interrupt();
    while(1){}
}