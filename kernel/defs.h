#pragma once

#include "types.h"
#include "riscv.h"
#include "memlayout.h"

extern pagetable_t kernel_pagetable;
struct context;
struct spinlock;
struct proc;

// main.c
void            main(void);

// kalloc.c
void            kinit(void);
void*           kalloc(void);
void            kfree(void *);

// string.c
void*           memset(void *dst, int c, unsigned long n);
void*           memcpy(void *dst, const void *src, unsigned long n);

// uart.c
void            uart_putc(char c);

// console.c
void            console_putc(char c);
void            console_puts(const char *s);
void            clear_screen(void);

// spinlock.c
void            initlock(struct spinlock *lk, char *name);
void            acquire(struct spinlock *lk);
void            release(struct spinlock *lk);
int             holding(struct spinlock *lk);
void            push_off(void);
void            pop_off(void);

// vm.c
pte_t*          walk_create(pagetable_t pt, uint64 va);
pte_t*          walk_lookup(pagetable_t pt, uint64 va);
int             map_region(pagetable_t pt, uint64 va, uint64 pa, uint64 sz, int perm);
int             unmap_page(pagetable_t pt, uint64 va);
int             map_page(pagetable_t pagetable, uint64 va, uint64 pa, int perm);
uint64          walkaddr(pagetable_t pt, uint64 va);
pagetable_t     create_pagetable(void);
void            destroy_pagetable(pagetable_t pt);
void            dump_pagetable(pagetable_t pt, int level);
void            kvminit(void);
void            kvminithart(void);

// proc.c
int             cpuid(void);
struct cpu*     mycpu(void);
void            sched(void);
int             killed(struct proc *p);
void            yield(void);
struct proc*    myproc(void);
void            kexit(int status);

// trap.c
extern uint     ticks;
extern struct spinlock tickslock;
void            trapinit(void);
void            trapinithart(void);
void            prepare_return(void);
void            register_interrupt(int irq, void (*handler)(void));
void            interrupt_dispatch(int irq);

// test.c
void            test_physical_memory(void);
void            test_pagetable(void);
void            test_virtual_memory(void);
void            test_printf(void);
void            test_kernel_timer_interrupt(void);