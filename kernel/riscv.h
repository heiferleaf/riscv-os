#pragma once

#include "types.h"

// qemu 将 UART 寄存器映射到物理内存中的这个地址。
#define UART0 0x10000000L // UART0 的基地址
#define UART0_IRQ 10      // UART0 的中断号

// virtio mmio 接口的基地址和中断号
#define VIRTIO0 0x10001000 // virtio 磁盘的基地址
#define VIRTIO0_IRQ 1      // virtio 磁盘的中断号

// qemu 将平台级中断控制器 (PLIC) 映射到这个地址。
#define PLIC 0x0c000000L // PLIC 的基地址
#define PLIC_PRIORITY (PLIC + 0x0) // PLIC 优先级寄存器
#define PLIC_PENDING (PLIC + 0x1000) // PLIC 挂起寄存器
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100) // 每个 hart 的中断使能寄存器
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000) // 每个 hart 的优先级阈值寄存器
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000) // 每个 hart 的中断声明寄存器


/**
 * 机器状态寄存器 mstatus
 * 位数  描述
 *  0    MIE - 机器模式中断使能
 *  1    SIE - 监管模式中断使能
 *  2    UIE - 用户模式中断使能
 * 7:4   MPIE - 机器模式前一中断使能
 *  8    SPIE - 监管模式前一中断使能
 *  9    UPIE - 用户模式前一中断使能
 * 11:10 MPP - 机器模式前一特权级
 *  12 SPP - 监管模式前一特权级
 * ...
 */ 
#define MSTATUS_MPP_MASK  (3L << 11)
#define MSTATUS_MPP_M     (3L << 11)
#define MSTATUS_MPP_S     (1L << 11)
#define MSTATUS_MPP_U     (0L << 11)

// sstatus相关位
#define SSTATUS_SPP       (1L << 8)
#define SSTATUS_SPIE      (1L << 5)
// S 模式中断使能
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer

#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

// M 模式的中断使能
#define MIE_STIE (1L << 5)  // supervisor timer
// mie/mip相关位
#define MIE_MEIE          (1L << 11)
#define MIE_MTIE          (1L << 7)
#define MIE_MSIE          (1L << 3)

// PMP
#define PMP_R (1 << 0)
#define PMP_W (1 << 1)
#define PMP_X (1 << 2)
#define PMP_A (3 << 3)
#define PMP_L (1 << 7)

// 读写CSR寄存器宏
#define r_mstatus()      ({ uint64 x; asm volatile("csrr %0, mstatus" : "=r" (x)); x; })
#define w_mstatus(x)     asm volatile("csrw mstatus, %0" : : "r" (x))
#define r_mepc()         ({ uint64 x; asm volatile("csrr %0, mepc" : "=r" (x)); x; })
#define w_mepc(x)        asm volatile("csrw mepc, %0" : : "r" (x))
#define r_medeleg()      ({ uint64 x; asm volatile("csrr %0, medeleg" : "=r" (x)); x; })
#define w_medeleg(x)     asm volatile("csrw medeleg, %0" : : "r" (x))
#define r_mideleg()      ({ uint64 x; asm volatile("csrr %0, mideleg" : "=r" (x)); x; })
#define w_mideleg(x)     asm volatile("csrw mideleg, %0" : : "r" (x))
#define r_mie()          ({ uint64 x; asm volatile("csrr %0, mie" : "=r" (x)); x; })
#define w_mie(x)         asm volatile("csrw mie, %0" : : "r" (x))
#define r_mip()          ({ uint64 x; asm volatile("csrr %0, mip" : "=r" (x)); x; })
#define w_mip(x)         asm volatile("csrw mip, %0" : : "r" (x))
#define r_mtvec()        ({ uint64 x; asm volatile("csrr %0, mtvec" : "=r" (x)); x; })
#define w_mtvec(x)       asm volatile("csrw mtvec, %0" : : "r" (x))
#define r_mhartid()      ({ uint64 x; asm volatile("csrr %0, mhartid" : "=r" (x)); x; })
#define r_sstatus()      ({ uint64 x; asm volatile("csrr %0, sstatus" : "=r" (x)); x; })
#define w_sstatus(x)     asm volatile("csrw sstatus, %0" : : "r" (x))
#define r_sepc()         ({ uint64 x; asm volatile("csrr %0, sepc" : "=r" (x)); x; })
#define w_sepc(x)        asm volatile("csrw sepc, %0" : : "r" (x))
#define r_scause()       ({ uint64 x; asm volatile("csrr %0, scause" : "=r" (x)); x; })
#define r_stval()        ({ uint64 x; asm volatile("csrr %0, stval" : "=r" (x)); x; })
#define r_satp()         ({ uint64 x; asm volatile("csrr %0, satp" : "=r" (x)); x; })
#define w_satp(x)        asm volatile("csrw satp, %0" : : "r" (x))
#define r_sie()          ({ uint64 x; asm volatile("csrr %0, sie" : "=r" (x)); x; })
#define w_sie(x)         asm volatile("csrw sie, %0" : : "r" (x))
#define r_sip()          ({ uint64 x; asm volatile("csrr %0, sip" : "=r" (x)); x; })
#define w_sip(x)         asm volatile("csrw sip, %0" : : "r" (x))
#define r_time()         ({ uint64 x; asm volatile("csrr %0, time" : "=r" (x)); x; })
#define w_stimecmp(x)    asm volatile("csrw stimecmp, %0" : : "r" (x))
#define r_tp()           ({ uint64 x; asm volatile("mv %0, tp" : "=r"(x)); x; })
#define w_tp(x)          asm volatile("mv tp, %0" : : "r"(x))

static inline void 
w_stvec(uint64 x)
{
  asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64
r_stvec()
{
  uint64 x;
  asm volatile("csrr %0, stvec" : "=r" (x) );
  return x;
}

// Machine Environment Configuration Register
static inline uint64
r_menvcfg()
{
  uint64 x;
  // asm volatile("csrr %0, menvcfg" : "=r" (x) );
  asm volatile("csrr %0, 0x30a" : "=r" (x) );
  return x;
}

static inline void 
w_menvcfg(uint64 x)
{
  // asm volatile("csrw menvcfg, %0" : : "r" (x));
  asm volatile("csrw 0x30a, %0" : : "r" (x));
}

static inline void
w_pmpcfg0(uint64 x)
{
  asm volatile("csrw pmpcfg0, %0" : : "r" (x));
}

static inline void
w_pmpaddr0(uint64 x)
{
  asm volatile("csrw pmpaddr0, %0" : : "r" (x));
}

#define SATP_SV39 (8L << 60)

#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))


// Machine-mode Counter-Enable
static inline void 
w_mcounteren(uint64 x)
{
  asm volatile("csrw mcounteren, %0" : : "r" (x));
}

static inline uint64
r_mcounteren()
{
  uint64 x;
  asm volatile("csrr %0, mcounteren" : "=r" (x) );
  return x;
}

static inline void
intr_on()
{
  w_sstatus(r_sstatus() | SSTATUS_SIE);
}

static inline void
intr_off()
{
    w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

static inline uint64
intr_get()
{
    uint64 x = r_sstatus();
    return (x & SSTATUS_SIE) != 0;
}

static inline uint64
r_sp()
{
  uint64 x;
  asm volatile("mv %0, sp" : "=r" (x) );
  return x;
}

static inline uint64
r_ra()
{
  uint64 x;
  asm volatile("mv %0, ra" : "=r" (x) );
  return x;
}


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

#define PTE_FLAGS(pte)  ((pte) & 0x3ff)           // 获取页表项的权限标志

#define PXMASK          0x1FF // 9 bits掩码
#define PXSHIFT(level)  (PGSHIFT+(9*(level))) // 对应每一级的VPN起始位偏移（L0=12，L1=21，L2=30）
#define PX(level, va) ((((uint64) (va)) >> PXSHIFT(level)) & PXMASK) 

// 虚拟内存最大地址，实际上用38位，定义了39位
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))