# 这是发生进程调度的上下文保护场所
#
# 调度函数的触发：void switch(struct context *old, struct context *new);
#
# 对于函数来说，第一个参数在 a0, 第二个参数在 a1
# 值得注意的是，对于上下文切换，载入新的上下文中，会把ra也载入，如此一来，在 ret 的时候，会返回新进程之前的调度点之后，或者别的内核态函数之后
# 总之，调度结束之后，还是在内核态，然后通过trampoline在恢复对应的用户态执行环境

.global proc_switch
proc_switch:
        sd ra, 0(a0)
        sd sp, 8(a0)
        sd s0, 16(a0)
        sd s1, 24(a0)
        sd s2, 32(a0)
        sd s3, 40(a0)
        sd s4, 48(a0)
        sd s5, 56(a0)
        sd s6, 64(a0)
        sd s7, 72(a0)
        sd s8, 80(a0)
        sd s9, 88(a0)
        sd s10, 96(a0)
        sd s11, 104(a0)

        ld ra, 0(a1)
        ld sp, 8(a1)
        ld s0, 16(a1)
        ld s1, 24(a1)
        ld s2, 32(a1)
        ld s3, 40(a1)
        ld s4, 48(a1)
        ld s5, 56(a1)
        ld s6, 64(a1)
        ld s7, 72(a1)
        ld s8, 80(a1)
        ld s9, 88(a1)
        ld s10, 96(a1)
        ld s11, 104(a1)

        ret