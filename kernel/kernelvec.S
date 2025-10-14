#
# 这里处理在 S 模式下的中断和异常
#
# 现在的栈已经是内核栈，原因可以参考uservec部分
# 先进行现场的保护，然后进入分发处理函数，最后返回恢复

.globl kerneltrap
.globl kernelvec
.align 4
kernelvec:
        # 输出 'A' 到 UART
        li t0, 0x10000000    # UART0 基地址
        li t1, 'A'           # 要输出的字符
        sb t1, 0(t0)         # 写入 UART0, 发送 'A'

        # make room to save registers.
        addi sp, sp, -256

        # save caller-saved registers.
        sd ra, 0(sp)
        sd gp, 16(sp)
        sd tp, 24(sp)
        sd t0, 32(sp)
        sd t1, 40(sp)
        sd t2, 48(sp)
        sd a0, 72(sp)
        sd a1, 80(sp)
        sd a2, 88(sp)
        sd a3, 96(sp)
        sd a4, 104(sp)
        sd a5, 112(sp)
        sd a6, 120(sp)
        sd a7, 128(sp)
        sd t3, 216(sp)
        sd t4, 224(sp)
        sd t5, 232(sp)
        sd t6, 240(sp)

        # call the C trap handler in trap.c
        call kerneltrap

        # 输出 'B' 到 UART
        li t0, 0x10000000
        li t1, 'B'
        sb t1, 0(t0)

        # restore registers.
        ld ra, 0(sp)
        ld gp, 16(sp)
        ld t0, 32(sp)
        ld t1, 40(sp)
        ld t2, 48(sp)
        ld a0, 72(sp)
        ld a1, 80(sp)
        ld a2, 88(sp)
        ld a3, 96(sp)
        ld a4, 104(sp)
        ld a5, 112(sp)
        ld a6, 120(sp)
        ld a7, 128(sp)
        ld t3, 216(sp)
        ld t4, 224(sp)
        ld t5, 232(sp)
        ld t6, 240(sp)

        addi sp, sp, 256

        # return to whatever we were doing in the kernel.
        sret