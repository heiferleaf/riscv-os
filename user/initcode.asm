
user/initcode.o:     file format elf64-littleriscv


Disassembly of section .text:

0000000000000000 <start>:
#include "kernel/syscall.h"

.globl start
start:
    // 测试 getpid 系统调用
    li a7, SYS_getpid
   0:	48ad                	li	a7,11
    ecall
   2:	00000073          	ecall
    // a0 现在包含进程ID
    
    // 测试 fork 系统调用
    li a7, SYS_fork
   6:	4885                	li	a7,1
    ecall
   8:	00000073          	ecall
    
    // 检查是父进程还是子进程
    beqz a0, child
   c:	c911                	beqz	a0,20 <child>
    
    // 父进程代码：等待子进程
    li a7, SYS_wait
   e:	488d                	li	a7,3
    mv a0, zero
  10:	00000513          	li	a0,0
    ecall
  14:	00000073          	ecall
    
    // 测试完成，父进程退出
    li a7, SYS_exit
  18:	4889                	li	a7,2
    li a0, 0
  1a:	4501                	li	a0,0
    ecall
  1c:	00000073          	ecall

0000000000000020 <child>:
    
    // 子进程代码：简单退出
child:
    li a7, SYS_exit
  20:	4889                	li	a7,2
    li a0, 42
  22:	02a00513          	li	a0,42
    ecall
  26:	00000073          	ecall

000000000000002a <spin>:

// 以防万一的无限循环
spin:
  2a:	a001                	j	2a <spin>
