#include "defs.h"
#include "printf.h"

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
    w_stimecmp(now + 100); // 100个时钟周期后触发（极短，确保尽快中断）
    printf("[TEST] Timer interrupt set for now+10 cycles (%lu)\n", now+10);
}