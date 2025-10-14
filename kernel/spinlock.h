#pragma once

// 目前的模拟的简化自旋锁实现，后续可以真正实现
struct spinlock {
    int locked; // 是否加锁
    char *name;
    struct cpu *cpu; // 持有锁的CPU
};