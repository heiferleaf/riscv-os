#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "printf.h"

#define MAX_IRQ 32
// 中断处理函数的函数指针数组
static void (*irq_table[MAX_IRQ])(void);

struct spinlock tickslock;

uint ticks;

extern char trampoline[], uservec[];

void kernelvec();

extern int devintr();


// 注册某IRQ的处理函数
void register_interrupt(int irq, void (*handler)(void)) {
    if(irq >= 0 && irq < MAX_IRQ)
        irq_table[irq] = handler;
}

// 分发处理函数
void interrupt_dispatch(int irq) {
    if(irq >= 0 && irq < MAX_IRQ && irq_table[irq])
        irq_table[irq]();
}

void handle_clockintr(void) {
    if(cpuid() == 0) {
        acquire(&tickslock);
        ticks++;
        wakeup(&ticks); // 唤醒等待 ticks 变化的进程
        release(&tickslock);
    }
    printf("clock interrupt\n");
    // 请求下一次定时器中断。这也会清除中断请求。1000000 大约是十分之一秒。
    w_stimecmp(r_time() + 1000000);
}

// 异常处理函数模板
void handle_illegal_instruction(void) {
    // struct proc *p = myproc();
    // printf("Illegal instruction: pid=%d sepc=0x%lx stval=0x%lx\n", p->pid, r_sepc(), r_stval());
    // setkilled(p); // 杀死进程
}

void handle_load_page_fault(void) {
    // struct proc *p = myproc();
    // printf("Load page fault: pid=%d sepc=0x%lx stval=0x%lx\n", p->pid, r_sepc(), r_stval());
    // setkilled(p);
}

void handle_store_page_fault(void) {
    // struct proc *p = myproc();
    // printf("Store page fault: pid=%d sepc=0x%lx stval=0x%lx\n", p->pid, r_sepc(), r_stval());
    // setkilled(p);
}

void handle_syscall(void) {
    struct proc *p = myproc();
    // 处理系统调用逻辑
    printf("Syscall: pid=%d syscallno=%ld\n", p->pid, p->trapframe->a7);
    // ...系统调用实现...
    p->trapframe->epc += 4; // 用户进程系统调用返回后，执行下一条指令

    intr_on(); // 允许中断

    // if(p->trapframe->a7 == 2 && p->pid == 2) {
    //     panic("chi exit");
    // }

    if(p->trapframe->a7 == 2 && p->pid == 1) {
        panic("father exit");
    }

    syscall();

}

void 
trapinit(void)
{
    initlock(&tickslock, "time");
    register_interrupt(5, handle_clockintr);           // 5: 时钟中断
    register_interrupt(2, handle_illegal_instruction); // 2: 非法指令
    register_interrupt(8, handle_syscall);             // 8: 系统调用
    register_interrupt(13, handle_load_page_fault);    // 13: 访存错误
    register_interrupt(15, handle_store_page_fault);   // 15: 访存出错
}

void
trapinithart(void)
{
    w_stvec((uint64)kernelvec);
}

uint64
usertrap(void)
{
    printf("[Usertrap] scause=0x%lx sepc=0x%lx stval=0x%lx\n",
        r_scause(), r_sepc(), r_stval());

    int which_dev = 0;

    // 如果不是从用户态进入的，则触发 panic
    if((r_sstatus() & SSTATUS_SPP) != 0)
        panic("usertrap: not from user mode");

    w_stvec((uint64)kernelvec);

    struct proc *p = myproc();

    p->trapframe->epc = r_sepc();

    uint64 scause = r_scause();
    int exc_code = scause & 0xfff;
    which_dev = devintr(); // 返回2表示时钟中断

    if(scause == 8){           // 系统调用
        interrupt_dispatch(8);
    } else if(which_dev == 2){ // 时钟中断
        yield();
    } else if(exc_code == 13){ // load page fault
        interrupt_dispatch(13);
    } else if(exc_code == 15){ // store page fault
        interrupt_dispatch(15);
    } else if(exc_code == 2){  // 非法指令
        interrupt_dispatch(2);
    } else {
        printf("usertrap(): unexpected scause 0x%lx pid=%d\n", scause, p->pid);
        printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
        setkilled(p);
    }

    // 进程被kill后，后续可补充自动退出逻辑
    if(killed(p))
        kexit(-1);

    prepare_return();

    if(p->trapframe->a7 == 3 && p->pid == 1) {
        printf("father wait return addr: 0x%lx\n", p->trapframe->epc);
        panic("father wait return");
    }

    // 设置回用户态的页表
    uint64 satp = MAKE_SATP(p->pagetable);
    return satp;
}

/**
 * 这是为了给下一次trap做准备，把进程所属CPU的内核信息更新
 */
void
prepare_return(void)
{
    struct proc *p = myproc();

    // 即将把 trap 的目标从 kerneltrap() 切换到 usertrap()。
    // 因为从内核态代码 trap 到 usertrap 会导致灾难，先关闭中断。
    intr_off();

    // 发送系统调用、中断和异常到 trampoline.S 中的 uservec
    uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
    w_stvec(trampoline_uservec);

    // 设置 trapframe 的值，uservec 在下次进程 trap 到内核时会用到。
    p->trapframe->kernel_satp = r_satp();         // 内核页表
    p->trapframe->kernel_sp = p->kstack + PGSIZE; // 进程的内核栈
    p->trapframe->kernel_trap = (uint64)usertrap;
    p->trapframe->kernel_hartid = r_tp();         // 用于 cpuid() 的 hartid

    // 设置 trampoline.S 的 sret 指令用于返回用户态的寄存器值

    // 设置 S Previous Privilege mode 为 User。
    unsigned long x = r_sstatus();
    x &= ~SSTATUS_SPP; // 清除 SPP，设置为用户态
    x |= SSTATUS_SPIE; // 允许用户态中断
    w_sstatus(x);

    // 设置 S Exception Program Counter 为保存的用户程序计数器
    w_sepc(p->trapframe->epc);
}

void 
kerneltrap()
{
  // which_dev: 标记中断设备类型（2为时钟中断，1为其他设备，0为未知）
  int which_dev = 0;
  // sepc: 保存的是从 kernelvec 中执行 sret 时返回的地址
  // 从 kerneltrap 返回 kernelvec 的时候, 是通过 函数调用的 ra 寄存器返回的
  uint64 sepc = r_sepc();
  // sstatus: 保存异常发生时的状态寄存器值
  uint64 sstatus = r_sstatus();
  // scause: 保存异常/中断原因码
  uint64 scause = r_scause();

//   printf("[KERNELTRAP] scause=0x%lx sepc=0x%lx sstatus=0x%lx stval=0x%lx sp=0x%lx ra=0x%lx\n",
//     scause, sepc, sstatus, r_stval(), (uint64)__builtin_frame_address(0), r_ra());
  
  // SPP 位表示在处理这个中断/异常之前的特权级别
  // 而kerneltrap只能从内核态进入，因此SPP必须为1
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  // 中断期间不处理其他中断，防止内核栈的溢出
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  // 0 表示未知中断源
  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    // printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    // panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  // 有进程触发时钟中断，并且不是空闲进程，则进行调度
  if(which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

int 
devintr()
{
    uint64 scause = r_scause();

    // 如果是 S 模式下的外部设备中断
    if(scause == 0x8000000000000009L) {
        // 获取中断号
        int irq = plic_claim();

        if(irq == UART0_IRQ){

        } else if(irq == VIRTIO0_IRQ){

        }

        if(irq)
            plic_complete(irq);
        return 1;
    } else if(scause == 0x8000000000000005L) {
        // 处理定时器中断
        interrupt_dispatch(5); // 5=IRQ_S_TIMER
        return 2;
    } else {
        return 0;
    }
}