#include "spinlock.h"
#include "defs.h"

/**
 * 物理内存页分配器,一个页持有下一个页的地址
 */
struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;   // 空闲链表的头指针
    int freepages;        // 空闲页的数量
} kmem;

// 初始化物理内存分配器
void
kinit(void) {
    initlock(&kmem.lock, "kmem");
    freerange(_end, (void*)PHYSTOP);
}

// 释放一段物理内存区域
void
freerange(void *pa_start, void *pa_end) {
    char *p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
        kfree(p);
}

// 释放一页物理内存
void
kfree(void *pa) {
    struct run *r;

    // 当pa的地址没有和页对齐，或者pa的地址小于内核结束地址，或者pa的地址大于等于物理内存结束地址时，触发panic
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < _end || (uint64)pa >= PHYSTOP)
    {
        printf("kfree: bad address %p\n", pa);
        panic("kfree: invalid address\n");
    }

    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    kmem.freepages++;
    release(&kmem.lock);
}

// 分配一个物理内存页
void *
kalloc(void) {
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if(r) {
        kmem.freelist = r->next;
        kmem.freepages--;
    }
    release(&kmem.lock);
    return (void*)r;
}

// 连续分配多个物理页,返回第一个页面的指针
// void *
// kalloc(int n) {
//     struct run *first_page, *r;

//     acquire(&kmem.lock);
//     if(kmem.freepages < n) {
//         release(&kmem.lock);
//         return 0; // 分配失败
//     }
//     first_page = kmem.freelist;
//     r = first_page;
//     for(int i = 1; i < n; i++) {
//         if(r == 0) {
//             // 这不应该发生，因为我们已经检查了freepages
//             release(&kmem.lock);
//             return 0;
//         }
//         r = r->next;
//     }
//     kmem.freelist = r->next; // 更新空闲链表的头指针
//     kmem.freepages -= n;
//     release(&kmem.lock);
//     return (void*)first_page;
// }