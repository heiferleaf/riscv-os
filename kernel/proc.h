#pragma once
#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"

struct context {
    uint64 ra;
    uint64 sp;

    // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

enum procstatus { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Trap现场保存结构（trapframe），用于trap发生时保存/恢复所有必要寄存器
struct trapframe {
    uint64 kernel_satp;     // 内核页表
    uint64 kernel_sp;       // 进程的内核栈顶
    uint64 kernel_trap;     // usertrap入口地址
    uint64 epc;             // 用户态PC（硬件自动保存）
    uint64 kernel_hartid;   // 当前CPU编号

    // 用户态的所有寄存器（ra, sp, gp, tp, t0-t6, s0-s11, a0-a7）
    uint64 ra, sp, gp, tp;
    uint64 t0, t1, t2, s0, s1;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
    uint64 s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint64 t3, t4, t5, t6;
};

struct proc {
    struct spinlock lock;

    // 需要持有lock才能访问的字段
    enum procstatus state;      // 进程状态
    void *chan;                 // 如果进程在睡眠，则为睡眠通道，否则为0，用于同步的唤醒
    int killed;                 // 如果进程被杀死，则为非0
    int xstate;                 // 进程退出状态，供父进程使用
    int pid;                    // 进程ID

    struct proc *parent;        // 父进程指针

    uint64 kstack;              // 进程内核栈虚拟地址
    uint64 sz;                  // 进程内存大小（字节）
    pagetable_t pagetable;      // 进程页表
    struct trapframe *trapframe;// 进程trapframe
    struct context context;     // 进程上下文，用于切换
    struct file *ofile[NOFILE];  // Open files
    struct inode *cwd;           // Current directory
    char name[16];              // 进程名称
};

struct cpu {
    struct proc *proc;          // 当前运行在该CPU上的进程
    struct context context;     // 该CPU的上下文
    int noff;                   // 该CPU上关闭中断的嵌套深度，为0的时候就可以打开中断
    int intena;                 // 中断之前是否开启
};

extern struct proc proc[NPROC];