#include "defs.h"

/**
当前的 console 仅支持提供给 printf 的控制台输出接口
简化了 xv6-riscv 的 console.c 实现：
    暂不实现锁
    暂不支持 write 和 read 的系统调用
但是提供了 xv6 中没有的清屏功能
*/

void console_putc(char c)
{
    // 输入换行的时候，同时输出回车
    if(c == '\n') {
        uart_putc('\r');
    }
    uart_putc(c);
}

void console_puts(const char *s)
{
    while(*s) {
        console_putc(*s++);
    }
}

void clear_screen(void)
{
    // ANSI 转义序列：\033[2J 清屏，\033[H 光标归位
    const char *clear_seq = "\033[2J\033[H";
    console_puts(clear_seq);
}