#pragma once

#include "types.h"
#include "riscv.h"
#include "memlayout.h"

extern pagetable_t kernel_pagetable;

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
void console_putc(char c);
void console_puts(const char *s);
void clear_screen(void);

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


// test.c
void            test_physical_memory(void);
void            test_pagetable(void);
void            test_virtual_memory(void);