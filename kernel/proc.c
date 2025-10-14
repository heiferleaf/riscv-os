#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];
struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

struct spinlock wait_lock;

// 得到当前的cpu id
int
cpuid()
{
    int id = r_tp();
    return id;
}

// 得到当前cpu的结构体
struct cpu*
mycpu(void)
{
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

void
kexit(int status)
{
    // struct proc *p = myproc();

    // if(p == initproc)
    //     panic("init exiting");

    // // 关闭所有打开的文件描述符
    // for(int fd = 0; fd < NOFILE; fd++){
    //     if(p->ofile[fd]){
    //         struct file *f = p->ofile[fd];
    //         p->ofile[fd] = 0;
    //         fileclose(f);
    //     }
    // }

    // begin_op();
    // iput(p->cwd);
    // end_op();
    // p->cwd = 0;

    // acquire(&wait_lock);

    // // 将子进程重新分配给 init 进程
    // for(struct proc *pp = proc; pp < &proc[NPROC]; pp++){
    //     if(pp->parent == p){
    //         pp->parent = initproc;
    //         if(pp->state == ZOMBIE){
    //             wakeup(initproc);
    //         }
    //     }
    // }

    // // 设置退出状态，并将进程状态改为 ZOMBIE
    // acquire(&p->lock);
    // p->xstate = status;
    // p->state = ZOMBIE;
    // release(&p->lock);

    // // 唤醒父进程
    // wakeup(p->parent);

    // // 进入调度器
    // sched();
    
    // panic("kexit: zombi exit");
}

void 
yield() {
    struct proc *p = myproc();
    acquire(&p->lock);
    p->state = RUNNABLE;
    sched();
    release(&p->lock);
}

void 
sched() {
    struct proc *p = myproc();
    for (int i = 0; i < NPROC; i++) {
        if (proc[i].state == RUNNABLE && &proc[i] != p) {
            proc_switch(&p->context, &proc[i].context);
            break;
        }
    }
}

void 
wakeup(void *chan)
{
    struct proc *p;

    for(p = proc; p < &proc[NPROC]; p++) {
        if(p != myproc()) {
            acquire(&p->lock);
            if(p->state == SLEEPING && p->chan == chan) {
                p->state = RUNNABLE;
            }
            release(&p->lock);
        }
    }
}