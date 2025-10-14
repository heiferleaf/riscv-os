#
# 从用户态进入内核态
# 1. 保存用户态进程的通用寄存器到进程trapframe
# 2. 设置 cpu 的运行栈为内核栈， 运行的页表为内核页表
# > 为了实现第二点，需要把用户和内核空间映射到相同的虚拟地址（对于这段汇编代码的物理地址）
# 需要 kernel.ld 链接脚本把这段代码放在页面的开始

#include "asm.h"

.section trampsec
.globl trampoline
.globl usertrap
trampoline:
.align 4
.globl uservec
uservec:
        # trap.c 会把中断向量表的基址设置为这里
        # 所有从用户空间的 trap 都会进入这里
        # 但是已经是 S 模式了，所以后面可以访问内核的PCB中的trapframe，可以执行这里的汇编

        # a0存放 trapframe
        csrw sscratch, a0

        # 此时还是用户页表，所有用户进程对于trapframe的物理地址会映射到相同的虚拟地址
        li a0, TRAPFRAME

        # 保存用户态进程的通用寄存器到进程trapframe
        sd ra, 40(a0)
        sd sp, 48(a0)
        sd gp, 56(a0)
        sd tp, 64(a0)
        sd t0, 72(a0)
        sd t1, 80(a0)
        sd t2, 88(a0)
        sd s0, 96(a0)
        sd s1, 104(a0)
        sd a1, 120(a0)
        sd a2, 128(a0)
        sd a3, 136(a0)
        sd a4, 144(a0)
        sd a5, 152(a0)
        sd a6, 160(a0)
        sd a7, 168(a0)
        sd s2, 176(a0)
        sd s3, 184(a0)
        sd s4, 192(a0)
        sd s5, 200(a0)
        sd s6, 208(a0)
        sd s7, 216(a0)
        sd s8, 224(a0)
        sd s9, 232(a0)
        sd s10, 240(a0)
        sd s11, 248(a0)
        sd t3, 256(a0)
        sd t4, 264(a0)
        sd t5, 272(a0)
        sd t6, 280(a0)

        # 在把原来a0寄存器的值保存
        csrr t0, sscratch
        sd t0, 112(a0)

        # 初始化内核栈指针，从p->trapframe->kernel_sp获取
        ld sp, 8(a0)

        # 让tp保存当前hartid，从p->trapframe->kernel_hartid获取
        ld tp, 32(a0)

        # 加载usertrap()的地址，从p->trapframe->kernel_trap获取
        ld t0, 16(a0)

        # 获取内核页表地址，从p->trapframe->kernel_satp获取
        ld t1, 0(a0)

        # 等待之前的内存操作完成，
        # 以确保它们使用用户页表。
        sfence.vma zero, zero

        # 切换到内核页表。
        csrw satp, t1

        # 清除TLB中已过期的用户项。
        sfence.vma zero, zero

        # 调用usertrap()
        jalr t0

.globl userret
userret:
        # usertrap()返回到这里，a0返回值中保存用户satp。
        # 从内核返回到用户空间。

        # 切换到用户页表。
        sfence.vma zero, zero
        csrw satp, a0
        sfence.vma zero, zero

        li a0, TRAPFRAME

        # 从TRAPFRAME恢复除a0外的所有寄存器
        ld ra, 40(a0)
        ld sp, 48(a0)
        ld gp, 56(a0)
        ld tp, 64(a0)
        ld t0, 72(a0)
        ld t1, 80(a0)
        ld t2, 88(a0)
        ld s0, 96(a0)
        ld s1, 104(a0)
        ld a1, 120(a0)
        ld a2, 128(a0)
        ld a3, 136(a0)
        ld a4, 144(a0)
        ld a5, 152(a0)
        ld a6, 160(a0)
        ld a7, 168(a0)
        ld s2, 176(a0)
        ld s3, 184(a0)
        ld s4, 192(a0)
        ld s5, 200(a0)
        ld s6, 208(a0)
        ld s7, 216(a0)
        ld s8, 224(a0)
        ld s9, 232(a0)
        ld s10, 240(a0)
        ld s11, 248(a0)
        ld t3, 256(a0)
        ld t4, 264(a0)
        ld t5, 272(a0)
        ld t6, 280(a0)

        # 恢复用户a0
        ld a0, 112(a0)
        
        # 返回到用户模式和用户pc。
        # usertrapret()已设置好sstatus和sepc。
        sret