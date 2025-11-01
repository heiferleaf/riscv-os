// 简化的初始用户程序，仅测试基本系统调用
#include "kernel/syscall.h"

.globl start
start:
    // 测试 getpid 系统调用
    li a7, SYS_getpid
    ecall
    // a0 现在包含进程ID
    
    // 测试 fork 系统调用
    li a7, SYS_fork
    ecall
    
    // 检查是父进程还是子进程
    beqz a0, child
    
    // 父进程代码：等待子进程
    li a7, SYS_wait
    mv a0, zero
    ecall
    
    // 测试完成，父进程退出
    li a7, SYS_exit
    li a0, 0
    ecall
    
    // 子进程代码：简单退出
child:
    li a7, SYS_exit
    li a0, 42
    ecall

// 以防万一的无限循环
spin:
    j spin