#pragma once

#include "types.h"

typedef uint64 pte_t;
typedef uint64 *pagetable_t;

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1)) //将任意大小sz向上对齐到页面边界。例如PGROUNDUP(5001)=8192。
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))  //将任意地址a向下对齐到页面边界。例如PGROUNDDOWN(5001)=4096。

#define PGSIZE 4096
#define PGSHIFT 12

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)

#define PA2PTE(pa) ((((uint64)(pa)) >> 12) << 10)   // 物理地址转为页表项格式
#define PTE2PA(pte) (((pte) >> 10) << 12)           // 页表项格式转为物理地址

#define PXMASK          0x1FF // 9 bits掩码
#define PXSHIFT(level)  (PGSHIFT+(9*(level))) // 对应每一级的VPN起始位偏移（L0=12，L1=21，L2=30）
#define PX(level, va) ((((uint64) (va)) >> PXSHIFT(level)) & PXMASK) 