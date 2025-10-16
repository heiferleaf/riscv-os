#include <stdarg.h>
#include <limits.h>
#include "printf.h"
#include "defs.h"
#include "spinlock.h"

volatile int panicking = 0; // printing a panic message
volatile int panicked = 0;

static struct {
    struct spinlock lock;
} pr ;

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

    if(panicking == 0)
        acquire(&pr.lock);

    va_start(ap, fmt);
    for(i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if(c != '%') {
            console_putc(c);
            continue;
        }
        i++;
        c = fmt[i] & 0xff;
        // 支持 long 类型前缀
        if(c == 'l') {
            i++;
            c = fmt[i] & 0xff;
            // long 十进制
            if(c == 'd') {
                printint(va_arg(ap, uint64), 10, 1);
            }
            // long 十六进制
            else if(c == 'x') {
                printint(va_arg(ap, uint64), 16, 0);
            }
            // long 类型指针
            else if(c == 'p') {
                void *ptr = va_arg(ap, void*);
                uint64 x = (uint64)ptr;
                console_putc('0');
                console_putc('x');
                printint(x, 16, 0);
            }
            else {
                // 未知格式，直接输出
                console_putc('%');
                console_putc('l');
                console_putc(c);
            }
        }
        // 原有格式支持
        else if(c == 'd') {
            printint(va_arg(ap, int), 10, 1);
        } 
        else if(c == 'x') {
            printint(va_arg(ap, uint32), 16, 0);
        } 
        else if(c == 's') {
            char *s = va_arg(ap, char*);
            if(s == 0) s = "(null)";
            while(*s) console_putc(*s++);
        } 
        else if(c == 'c') {
            char ch = (char)va_arg(ap, int);    
            console_putc(ch);
        }
        else if(c == 'p') {
            void *ptr = va_arg(ap, void*);
            uint64 x = (uint64)ptr;
            console_putc('0');
            console_putc('x');
            printint(x, 16, 0);
        }
        else if(c == '%' ) {
            console_putc('%');
        } else {
            console_putc('%');
            console_putc(c);
        }
    }
    va_end(ap);

    if(panicking == 0)
        release(&pr.lock);

    return 0;
}

void
panic(char *s)
{
  panicking = 1;
  printf("panic: ");
  printf("%s\n", s);
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}