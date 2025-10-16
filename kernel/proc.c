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

extern char trampoline[]; // trampoline.S

struct spinlock wait_lock;

void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    // kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
    if(map_page(kpgtbl, va, (uint64)pa, PTE_R | PTE_W) != 0)
      panic("mapstack");
  }
}

// 初始化进程管理
void
procinit(void)
{
    struct proc *p;

    initlock(&pid_lock, "nextpid");
    initlock(&wait_lock, "wait_lock");
    for(p = proc; p < &proc[NPROC]; p++) {
        initlock(&p->lock, "proc");
        p->state = UNUSED;
        p->kstack = KSTACK((int) (p - proc));
    }
}

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
yield() {
    struct proc *p = myproc();
    acquire(&p->lock);
    p->state = RUNNABLE;
    sched();
    release(&p->lock);
}

// 切换到调度器（scheduler）。
// 必须只持有 p->lock，并且已经修改了 proc->state。
// 保存和恢复 intena，因为 intena 是该内核线程的属性，而不是 CPU 的属性。
// 理论上应该是 proc->intena 和 proc->noff，但在某些持锁但没有进程的地方会出错。
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  // 必须持有当前进程的锁，否则 panic。
  if(!holding(&p->lock))
    panic("sched p->lock");
  // 必须只关闭一次中断（noff == 1），否则 panic。
  if(mycpu()->noff != 1)
    panic("sched locks");
  // 当前进程状态不能是 RUNNING，否则 panic。
  if(p->state == RUNNING)
    panic("sched RUNNING");
  // 必须确保中断已经关闭，否则 panic。
  if(intr_get())
    panic("sched interruptible");

  // 保存当前 CPU 的中断使能状态。
  intena = mycpu()->intena;
  // 切换上下文，从当前进程切换到调度器。
  proc_switch(&p->context, &mycpu()->context);
  // 恢复之前保存的中断使能状态。
  mycpu()->intena = intena;
}


void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // The most recent process to run may have had interrupts
    // turned off; enable them to avoid a deadlock if all
    // processes are waiting. Then turn them back off
    // to avoid a possible race between an interrupt
    // and wfi.
    intr_on();
    intr_off();

    int found = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
      printf("[scheduler]: check pid=%d state=%d\n", p->pid, p->state);
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        printf("before switch to pid=%d\n", p->pid);
        proc_switch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        found = 1;
      }
      release(&p->lock);
    }
    if(found == 0) {
      // nothing to run; stop running on this core until an interrupt.
      asm volatile("wfi");
    }
  }
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  extern char userret[];
  static int first = 1;
  struct proc *p = myproc();

  // printf("count: %d", first++);

  // 仍然持有 scheduler 传递过来的 p->lock。
  release(&p->lock);

//   if (first) {
//     // 文件系统初始化必须在常规进程上下文中运行（例如会调用 sleep），因此不能在 main() 中运行。
//     fsinit(ROOTDEV);

//     first = 0;
//     // 确保其他核心能看到 first=0。
//     __sync_synchronize();

//     // 文件系统初始化完成后可以调用 kexec()。
//     // 将 kexec 的返回值（argc）放入 a0。
//     p->trapframe->a0 = kexec("/init", (char *[]){ "/init", 0 });
//     if (p->trapframe->a0 == -1) {
//       panic("exec");
//     }
//   }

  // 这里选择直接运行测试代码
  printf("forkret: enter pid=%d, kstack=%p\n", p->pid, (void*)p->kstack);
  test_entry();

  // 返回用户空间，模拟 usertrap() 的返回。
//   prepare_return();
//   uint64 satp = MAKE_SATP(p->pagetable);
//   uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
//   ((void (*)(uint64))trampoline_userret)(satp);
}

// 进程睡眠，等待chan事件
void sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();

    // 必须持有p->lock修改状态
    acquire(&p->lock);
    release(lk);

    p->chan = chan;
    p->state = SLEEPING;

    sched();

    // 唤醒后清理chan
    p->chan = 0;

    release(&p->lock);
    acquire(lk);
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

pagetable_t
proc_pagetable(struct proc *p)
{
    pagetable_t pagetable;

    pagetable = create_pagetable();
    if(pagetable == 0)
        return 0;

    // 对于进程的用户空间，最后一段需要是 TRAMPOLINE 
    //  这个虚拟地址到物理地址的映射，和内核是完全一样的
    // 当进程运行中触发中断，一定会进入内核态，所以不需要设置 PTE_U
    if(map_page(pagetable, TRAMPOLINE, (uint64)trampoline, PTE_R | PTE_X) < 0){
        uvmfree(pagetable, 0);
        return 0;
    }

    if(map_page(pagetable, TRAPFRAME, (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
        unmap_page(pagetable, TRAMPOLINE);
        uvmfree(pagetable, 0);
        return 0;
    }

    return pagetable;
}

void            
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
    unmap_page(pagetable, TRAMPOLINE);
    unmap_page(pagetable, TRAPFRAME);
    uvmfree(pagetable, sz);
}


int 
allocpid(void)
{
    int pid;
    acquire(&pid_lock);
    pid = nextpid++;
    release(&pid_lock);
    return pid;
}

/**
 * allocproc - 分配一个新的进程
 * 1. 遍历进程表，找到一个状态为 UNUSED 的进程
 * 2. 分配一个新的 PID
 * 3. 分配内核栈、页表和 trapframe
 * 4. 初始化进程的上下文，使其能从 forkret 返回，并初始化自己的栈指针
 */
struct proc*
allocproc(void)
{
  printf("[TEXT] enter allocproc\n");
    struct proc *p;

    for(p = proc; p < &proc[NPROC]; p++) {
      printf("[TEXT] checking proc %d, state=%d\n", (int)(p - proc), p->state);
        acquire(&p->lock);
        if(p->state == UNUSED) {
            p->pid = allocpid();
            printf("[TEXT] allocproc: pid=%d\n", p->pid);
            p->state = USED;

            // 给内核的页表添加该进程的 trap 帧的物理页
            p->trapframe = (struct trapframe *)kalloc();
            printf("[TEXT] allocproc: pid=%d, trapframe=%p\n", p->pid, (void*)p->trapframe);
            if(p->trapframe == 0) {
                freeproc(p);
                release(&p->lock);
                return 0;
            }
            // 分配用户页表
            p->pagetable = proc_pagetable(p);
            if(p->pagetable == 0) {
                freeproc(p);
                release(&p->lock);
                return 0;
            }
            // 初始化上下文
            memset(&p->context, 0, sizeof(p->context));
            p->context.ra = (uint64)forkret;
            p->context.sp = p->kstack + PGSIZE;

            printf("[TEXT] allocproc: pid=%d, kstack=%p\n", p->pid, (void*)p->kstack);
            return p;
        } else
            release(&p->lock);
    }
    return 0;
}

void
freeproc(struct proc *p)
{
    if(p->trapframe) kfree((void*)p->trapframe);
    p->trapframe = 0;
    if(p->pagetable) proc_freepagetable(p->pagetable, p->sz);
    p->pagetable = 0;
    p->sz = 0;
    p->pid = 0;
    p->parent = 0;
    p->name[0] = 0;
    p->chan = 0;
    p->killed = 0;
    p->xstate = 0;
    p->state = UNUSED;
}

int
kfork(void)
{
    int i, pid;
    struct proc *np;
    struct proc *p = myproc();  // 父进程

    printf("[DEBUG] in kfork, pid: %d\n", p->pid);

    if((np = allocproc()) == 0) {
        return -1;
    }

    // 先赋值一份映射
    if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
        freeproc(np);
        release(&np->lock);
        return -1;
    }
    // 赋值进程内存空间大小
    np->sz = p->sz;
    // 赋值trap帧内容
   *(np->trapframe) = *(p->trapframe);
    // 让子进程的返回值为0
    np->trapframe->a0 = 0;

    // 复制父进程的文件描述符，暂时先不处理
    // for(i = 0; i < NOFILE; i++) {
    //     if(p->ofile[i]) {
    //         np->ofile[i] = filedup(p->ofile[i]);
    //     }
    // }
    // np->cwd = idup(p->cwd);

    safestrcpy(np->name, p->name, sizeof(p->name));

    pid = np->pid;

    release(&np->lock);

    acquire(&wait_lock);
    np->parent = p;
    release(&wait_lock);

    acquire(&np->lock);
    np->state = RUNNABLE;
    release(&np->lock);

    return pid;
}

// 进程退出
void kexit(int status) {
    struct proc *p = myproc();

    // 关闭所有打开的文件
    // for(int fd = 0; fd < NOFILE; fd++) {
    //     if(p->ofile[fd]) { fileclose(p->ofile[fd]); p->ofile[fd] = 0; }
    // }

    // 释放工作目录
    // if(p->cwd) { iput(p->cwd); p->cwd = 0; }

    // reparent所有子进程
    for(struct proc *pp = proc; pp < &proc[NPROC]; pp++) {
        if(pp->parent == p) {
            pp->parent = initproc;
        }
    }

    // 唤醒父进程
    wakeup(p->parent);

    // 设置退出状态与进程状态
    acquire(&p->lock);
    p->xstate = status;
    p->state = ZOMBIE;
    release(&p->lock);

    // 进入调度器
    sched();
    panic("zombie exit"); // 这行代码不会被执行
}

// 父进程等待子进程退出
int kwait(uint64 *status) {
    struct proc *p = myproc();
    int havekids, pid;

    for(;;) {
        havekids = 0;
        for(struct proc *pp = proc; pp < &proc[NPROC]; pp++) {
            if(pp->parent == p) {
                havekids = 1;
                acquire(&pp->lock);
                if(pp->state == ZOMBIE) {
                    // 回收资源
                    pid = pp->pid;
                    if(status) *status = pp->xstate;
                    freeproc(pp);
                    release(&pp->lock);
                    return pid;
                }
                release(&pp->lock);
            }
        }
        if(!havekids) return -1;   // 没有子进程
        // 没有ZOMBIE就睡眠
        sleep(p, &p->lock);
    }
}