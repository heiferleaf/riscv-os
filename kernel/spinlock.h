#pragma once

// 目前的模拟的简化自旋锁实现，后续可以真正实现
struct spinlock {
    int locked; // 是否加锁
};

static inline void initlock(struct spinlock *lk, const char *name) {
    lk->locked = 0;
}

static inline void acquire(struct spinlock *lk) {
    lk->locked = 1;
  }
  
  static inline void release(struct spinlock *lk) {
    lk->locked = 0;
  }