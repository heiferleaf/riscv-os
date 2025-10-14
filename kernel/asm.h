#pragma once

// UART
#define UART0      0x10000000L
#define UART0_IRQ  10

// VIRTIO
#define VIRTIO0    0x10001000L
#define VIRTIO0_IRQ 1

// Platform-Level Interrupt Controller (PLIC)
#define PLIC       0x0c000000L

// Memory layout
#define KERNBASE   0x80000000L
#define PHYSTOP    (KERNBASE + 128 * 1024 * 1024)

// Paging and virtual memory
#define PGSIZE     4096
#define PGSHIFT    12
#define MAXVA      (1L << (9 + 9 + 9 + 12 - 1))

// Trampoline and trapframe virtual addresses
#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME  (TRAMPOLINE - PGSIZE)