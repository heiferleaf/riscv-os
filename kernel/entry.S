    .section .text
    .globl _start
_start:
    # 1. 给出启动标记，验证 UART 工作
    li t0, 0x10000000   # UART0 基地址（virt 平台手册里写死）
    li t1, 'S'
    sb t1, 0(t0)        # 把 'S' 写入 UART 寄存器，屏幕应显示 S

    # 2. 设置栈指针
    la sp, stack_top    # 把栈顶地址装入 sp
    li t1, 'P'
    sb t1, 0(t0)        # 输出 'P'，说明栈已经就绪

    # 3. 清空 BSS 段（全局未初始化变量所在）
    la a0, bss_start
    la a1, bss_end
1:
    bge a0, a1, 2f      # 如果 a0 >= a1，跳出循环
    sw zero, 0(a0)      # 向地址 a0 写入 0
    addi a0, a0, 4
    j 1b
2:
    # 4. 跳转到 C 主函数
    call start          

hang:
    j hang              # 如果返回了，就死循环
