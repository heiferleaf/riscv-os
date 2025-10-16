#include "defs.h"
#include "printf.h"
#include "param.h"
#include "proc.h"

void test_printf(void) {
    // 基本功能测试
    
    printf("Int: %d\n", 42);
    printf("Hex: 0x%x\n", 0xABC);
    printf("Str: %s\n", "Hello");
    printf("Char: %c\n", 'X');
    printf("Percent: %%\n");

    // 边界测试
    printf("Boundary Test:\n");
    printf("INT_MIN: %d\n", -9223372036854775807LL - 1);
    printf("NULL Str: %s\n", (char*)0);
    printf("Unknown: %q\n"); // 未知格式测试

    // 清屏测试
    printf("Before Clear\n");
    clear_screen();
    printf("After Clear\n");

    // 性能测试 (大量输出)
    printf("Performance Test:\n");
    for (int i = 0; i < 100; i++) {
        printf("Count: %d\n", i);
    }
}

void test_physical_memory(void) {
    kinit();
    // 分配两个页
    void *page1 = kalloc();
    void *page2 = kalloc();
    
    printf("Allocated pages at %p and %p\n", page1, page2);

    if(page1 == page2) {
        printf("Allocated the same page twice!\n");
        return;
    }

    // 读入数据并读回
    *(int*) page1 = 0x12345678;
    if(*(int*) page1 != 0x12345678) {
        printf("Memory read/write test failed!\n");
        return;
    }

    // 释放、重分配
    kfree(page1);
    void *page3 = kalloc();
    printf("Re-allocated page at %p\n", page3);
    if(page3 != page1) {
        printf("Memory re-allocation test failed!\n");
        return;
    }

    kfree(page2);
    kfree(page3);

    printf("Physical memory allocation test passed.\n");
}

void test_pagetable(void) {
    printf("\n");
    // 1. 初始化一个页表
    pagetable_t pgtal = create_pagetable();

    // 2. 测试把一个虚拟地址映射到一个物理地址
    uint64 va = 0x1000000;  // 这里要注意Sv39的地址范围
    uint64 pa = (uint64)kalloc();   // 分配一个物理页
    printf("kalloc physical address:%p \n", pa);  // 打印一下 pa 的地址
    if(pa == 0) {
        printf("Failed to allocate physical page for testing\n");
        return;
    }
    if(map_page(pgtal, va, pa, PTE_R | PTE_W | PTE_U) != 0) {
        printf("map_page failed\n");
        kfree((void*)pa);
        return;
    }
    printf("Mapped VA 0x%x to PA 0x%x\n", (uint32)va, (uint32)pa);

    // 3. 测试walk_lookup能否找到正确的PTE
    pte_t* pte = walk_lookup(pgtal, va);
    if(pte == 0 || (PTE2PA(*pte)) != pa) {
        printf("walk_lookup failed to find correct PTE\n");
        unmap_page(pgtal, va);
        kfree((void*)pa);
        return;
    }
    printf("walk_lookup found correct PTE: 0x%x\n", (uint32)(*pte));

    // 4. 测试 pte 的权限位
    if((*pte & (PTE_R | PTE_W | PTE_U)) != (PTE_R | PTE_W | PTE_U)) {
        printf("PTE permissions incorrect\n");
        unmap_page(pgtal, va);
        kfree((void*)pa);
        return;
    }
    printf("PTE permissions correct\n");

    printf("\n");
}

void test_virtual_memory(void) {
    printf("Before enabling paging...\n");
    kvminit();
    kvminithart();
    printf("After enabling paging...\n");

    // 检查内核代码、数据可访问
    printf("kernel pagetable: %p\n", (uint64)kernel_pagetable);

    // 检查 UART 是否可用
    uart_putc('T');
    printf("\nUART test done\n");
}

void test_syscall() {
    asm volatile("li a7, 123; ecall"); // a7=123为系统调用号
}

void test_illegal_instruction() {
    asm volatile(".word 0xFFFFFFFF"); // 非法指令
}

void test_load_page_fault() {
    volatile int *p = (int*)0x12345678; // 未映射地址
    int x = *p; // 触发load异常
}

void test_kernel_timer_interrupt() {
    printf("[TEST] Forcing timer interrupt from kernel...\n");
    uint64 now = r_time();
    w_stimecmp(now + 100000); // 100个时钟周期后触发（极短，确保尽快中断）
    printf("[TEST] Timer interrupt set for now+10 cycles (%lu)\n", now+10);
}


// 进程分配和释放测试
void test_allocproc_freeproc() {
    printf("=== allocproc/freeproc test ===\n");
    struct proc *p1 = allocproc();
    if(p1) {
        printf("allocproc success: pid=%d\n", p1->pid);
        // 为了安全，先把进程设为 UNUSED（freeproc 已经不持锁）
        freeproc(p1);
        release(&p1->lock);
        printf("freeproc success: pid freed\n");
    } else {
        printf("allocproc failed\n");
    }
}

// kfork测试（只检测 kfork 是否返回有效 pid 并设置 parent）
void test_kfork() {
    printf("=== kfork test ===\n");
    int pid = kfork();
    if(pid > 0) {
        printf("kfork success: child pid=%d\n", pid);
        // 查找子进程并打印 parent info
        for(struct proc *pp = proc; pp < &proc[NPROC]; pp++) {
            if(pp->pid == pid) {
                printf("child found: pid=%d parent_pid=%d state=%d\n", pp->pid, pp->parent ? pp->parent->pid : 0, pp->state);
                break;
            }
        }
    } else {
        printf("kfork failed\n");
    }
}

// exit/kwait 测试（在单根进程环境中：手动把子进程设为 ZOMBIE，再 kwait 回收）
void test_exit_kwait_simulated() {
    printf("=== simulated kexit/kwait test ===\n");
    int pid = kfork();
    if(pid <= 0) {
        printf("kfork failed for simulated exit/kwait\n");
        return;
    }
    // 找到子进程 PCB，并把它设为 ZOMBIE，设置退出码
    struct proc *child = 0;
    for(struct proc *pp = proc; pp < &proc[NPROC]; pp++) {
        if(pp->pid == pid) { child = pp; break; }
    }
    if(child == 0) {
        printf("child not found\n");
        return;
    }

    // 模拟子进程已经退出（因为我们无法实际调度子去执行 kexit）
    acquire(&child->lock);
    child->xstate = 123;
    child->state = ZOMBIE;
    release(&child->lock);

    // 父进程（当前进程）调用 kwait 回收
    uint64 status = 0;
    int ret = kwait(&status);
    printf("kwait returned: pid=%d status=%llu\n", ret, status);
}

// kwait 测试：同上（只是演示）
void test_kwait() {
    printf("=== kwait test (simulated) ===\n");
    test_exit_kwait_simulated();
}

// sleep/wakeup 测试：不能让当前进程 sleep（会导致调度器等待），
// 所以我们直接构造一个假的进程使其处于 SLEEPING，然后调用 wakeup 验证 wakeup 把它置为 RUNNABLE。
void test_sleep_wakeup_simulated() {
    printf("=== sleep/wakeup simulated test ===\n");
    struct proc *p = allocproc();
    if(!p) {
        printf("allocproc for sleep/wakeup failed\n");
        return;
    }

    // 把这个进程放到 SLEEPING 上，并设置 chan
    // acquire(&p->lock);
    p->chan = (void*)0xdead;
    p->state = SLEEPING;
    release(&p->lock);

    // 调用 wakeup，期望把 p->state 置为 RUNNABLE
    wakeup((void*)0xdead);

    // 检查状态
    acquire(&p->lock);
    int state = p->state;
    release(&p->lock);

    printf("after wakeup: pid=%d state=%d (expect RUNNABLE=%d)\n", p->pid, state, RUNNABLE);

    // 清理
    acquire(&p->lock);
    if(p->state == RUNNABLE) {
        // 手动回收（因为 kvfork 等未真正运行）
        p->state = UNUSED;
    } else {
        // 如果仍然 SLEEPING/其他，直接 free
        p->state = UNUSED;
    }
    release(&p->lock);
}

// 统一测试入口（按顺序运行不会阻塞调度器）
void test_entry() {
    test_allocproc_freeproc();
    test_kfork();
    test_kwait();
    test_sleep_wakeup_simulated();
    printf("=== all tests done ===\n");

    initproc->state = ZOMBIE; // 让 initproc 退出，结束模拟
    acquire(&initproc->lock);
    sched();
}