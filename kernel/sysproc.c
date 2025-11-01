#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_fork(void)
{
    return kfork();
}

/**
 * 退出当前进程，状态码为n
 */
uint64
sys_exit(void)
{
    int n;
    argint(0, &n);
    kexit(n);
    return 0;   // 实际上不会执行到这里，在kexit中已经切换了进程
}

/**
 * 返回当前进程的PID
 * 会把子进程的退出状态写入系统调用第一个参数指向的地址
 */
uint64
sys_wait(void)
{
    uint64 p;
    argaddr(0, &p);
    return kwait(p);
}

uint64
sys_kill(void)
{
    int pid;

    argint(0, &pid);
    return kkill(pid);
}

// 实现getpid系统调用 - 获取当前进程ID
uint64
sys_getpid(void)
{
  return myproc()->pid;
}
