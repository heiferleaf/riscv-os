#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

extern struct superblock sb;

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

    virtio_disk_init(); // 必须在前！

    binit();    // 初始化缓冲区缓存
    printf("binit done\n");
    iinit();       // 初始化 inode 表
    printf("iinit done\n");
    fileinit();      // file table

    userinit();      // 第一个用户进程
    scheduler();

    printf("kernel main exit\n");
}