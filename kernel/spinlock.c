#include "types.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "printf.h"
#include "defs.h"

void 
initlock(struct spinlock *lk, char* name)
{
    lk->name = name;
    lk->locked = 0;
    lk->cpu = 0;
}

/**
 * 获得锁
 */
void
acquire(struct spinlock *lk)
{
    push_off(); // 关闭中断，防止死锁
    if(holding(lk)) {
        // panic 还未实现
        // panic("acquire");
    }
    while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ; // 自旋等待
    __sync_synchronize(); // 内存屏障，防止指令重排
    lk->cpu = mycpu(); // 记录当前cpu
}

/**
 * 释放锁
 */
void
release(struct spinlock *lk)
{
    if(!holding(lk)) {
        // panic 还未实现
        // panic("release");
    }
    lk->cpu = 0;
    __sync_synchronize(); // 内存屏障，防止指令重排
    __sync_lock_release(&lk->locked); // 释放锁
    pop_off(); // 恢复中断
}

int
holding(struct spinlock *lk)
{
    int re;
    // 原来的实现，考虑当前运行的cpu是否正确 re = (lk->locked && lk->cpu == mycpu());
    // 现在简化多核运行环境，单核环境下只需要判断是否加锁
    re = (lk->locked != 0);
    return re;

}

void
push_off(void)
{
    int old = intr_get();

    intr_off();
    if(mycpu()->noff == 0)
        mycpu()->intena = old;
    mycpu()->noff += 1;
}

void
pop_off(void)
{
    struct cpu *c = mycpu();
    // 如果中断已经开启，说明有问题
    if(intr_get())
        panic("pop_off - interruptible");
    if(c->noff < 1)
        panic("pop_off");
    c->noff -= 1;
    if(c->noff == 0 && c->intena)
        intr_on();
}