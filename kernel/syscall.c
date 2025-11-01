#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"


// 提取调用参数的函数
// 获取第n个系统调用参数（从a0-a5寄存器）
static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  return -1;
}

// 获取整形的参数
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// 获取指针类型的参数
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// 从用户空间获取整型,保证访问地址的安全性
int
fetchaddr(uint64 addr, uint *ip)
{
    struct proc *p = myproc();
    if(addr >= p->sz || addr+sizeof(uint) > p->sz) // 超过进程虚拟空间大小
        return -1;
    if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
        return -1;
    return 0;
}

// 从用户空间获取字符串,保证访问地址的安全性
int
fetchstr(uint64 addr, char *buf, int max)
{
    struct proc *p = myproc();
    if(copyinstr(p->pagetable, buf, addr, max) < 0)
        return -1;
    return strlen(buf);
}

/**
 * 获取字符串类型的参数
 * @param n 参数索引
 * @param buf 存放字符串的缓冲区
 * @param max 缓冲区大小
 * @return 成功返回字符串长度，失败返回-1
 */
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// 系统调用处理函数声明
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);

// 简化的系统调用表，只包含我们实现的系统调用
static uint64 (*syscalls[])(void) = {
  [SYS_fork]    sys_fork,
  [SYS_exit]    sys_exit,
  [SYS_wait]    sys_wait,
  [SYS_kill]    sys_kill,
  [SYS_getpid]  sys_getpid,
};

// trapframe->a7存放系统调用号，同时在系统调用执行后，需要存放返回值到a0中
void
syscall(void)
{
    int num;
    struct proc *p = myproc();

    num = p->trapframe->a7;
    if(num > 0 && num < SYS_end && syscalls[num]) {
        // 根据系统调用号，从分发表中找到对应的处理函数并调用
        // 将返回值存放到a0中
        p->trapframe->a0 = syscalls[num]();
    } else {
        printf("%d %s: unknown sys call %d\n",
                p->pid, p->name, num);
        p->trapframe->a0 = -1;
    }
}