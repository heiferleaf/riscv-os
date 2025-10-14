#pragma once

#define UART0      0x10000000L
#define RHR 0     // 接收寄存器 (没用)
#define THR 0     // 发送寄存器
#define LSR 5     // 状态寄存器
#define LSR_TX_IDLE (1 << 5)
#define VIRTIO0    0x10001000

#define KERNBASE 0x80000000L // 内核的基地址
#define PHYSTOP (KERNBASE + 128*1024*1024) // 内核使用的物理内存结束地址 (128MB)

extern char _end[]; // 链接脚本提供的符号，内核结束地址

// 把trampoline页映射到最高地址，
// 在用户空间和内核空间中都一样。
#define TRAMPOLINE (MAXVA - PGSIZE)

// 把内核栈映射到trampoline之下，
// 每个栈都被无效的保护页包围。
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE) // 每个进程的内核栈地址

// 用户内存布局。
// 地址从零开始:
//   text (代码段)
//   original data and bss (数据段和未初始化数据段)
//   fixed-size stack (固定大小的栈)
//   expandable heap (可扩展的堆)
//   ...
//   TRAPFRAME 所有的用户进程这个值都相同
//   TRAMPOLINE (与内核中的 trampoline 页相同)
#define TRAPFRAME (TRAMPOLINE - PGSIZE) // 用户 trapframe 的虚拟地址