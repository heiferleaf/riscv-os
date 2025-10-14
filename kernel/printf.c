#include <stdarg.h>
#include <limits.h>
#include "printf.h"
#include "defs.h"

static char digits[] = "0123456789abcdef";
/**
设置一个整数打印制定进制的函数
 */
static void printint(long long xx, int base, int sign)
{
    // 存放转换进制之后的字符
    char buf[20];
    int i;
    unsigned long long x;

    // 对xx是最小值时进行额外处理，防止-xx时溢出，导致x的赋值错误
    if(xx == LLONG_MIN) {
        console_putc('-');
        char *min_str = "9223372036854775808";
        while(*min_str) console_putc(*min_str++);        
        return;
    }

    if(sign && (sign = (xx < 0)))
        // 当且仅当xx时有符号数且为负数时，x取-xx
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);

    if(sign) console_putc('-');
    while(--i >= 0) console_putc(buf[i]);
}

/**
printf 用于输出可变参数的打印信息
*/
int printf(const char *fmt, ...)
{
    va_list ap;
    int i , c;

    va_start(ap, fmt);
    // 得到字符的后8位，解析 ASCII 码
    for(i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if(c != '%') {
            console_putc(c);
            continue;
        }
        i++;
        c = fmt[i] & 0xff;
        // 十进制整数
        if(c == 'd') {
            printint(va_arg(ap, int), 10, 1);
        } 
        // 16进制
        else if(c == 'x') {
            printint(va_arg(ap, uint32), 16, 0);
        } 
        // 字符串
        else if(c == 's') {
            char *s = va_arg(ap, char*);
            if(s == 0) s = "(null)";
            while(*s) console_putc(*s++);
        } 
        // 字符
        else if(c == 'c') {
            char ch = (char)va_arg(ap, int);    
            console_putc(ch);
        }
        // 16 进制输出指针
        else if(c == 'p') {
            void *ptr = va_arg(ap, void*);
            uint64 x = (uint64)ptr;
            console_putc('0');
            console_putc('x');
            printint(x, 16, 0);

        }
        // 转移输出%
        else if(c == '%' ) {
            console_putc('%');
        } else {
            // 和 xv6 相同的实现
            // Print unknown % sequence to draw attention.
            console_putc('%');
            console_putc(c);
        }
    }
}

void
panic(char *s)
{
//   panicking = 1;
  printf("panic: ");
  printf("%s\n", s);
//   panicked = 1; // freeze uart output from other CPUs
//   for(;;)
//     ;
}