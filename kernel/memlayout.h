#pragma once

#define UART0      0x10000000L
#define RHR 0     // 接收寄存器 (没用)
#define THR 0     // 发送寄存器
#define LSR 5     // 状态寄存器
#define LSR_TX_IDLE (1 << 5)
#define VIRTIO0    0x10001000
#define PLIC       0x0c000000

#define KERNBASE 0x80000000L // 内核的基地址
#define PHYSTOP (KERNBASE + 128*1024*1024) // 内核使用的物理内存结束地址 (128MB)

extern char _end[]; // 链接脚本提供的符号，内核结束地址