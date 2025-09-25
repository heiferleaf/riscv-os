#include <stdint.h>
#include "printf.h"
#include "defs.h"

extern void uart_puts(const char *s);

void start() {

    test_physical_memory();

    test_pagetable();

    test_virtual_memory();

    // 基本功能测试
    // printf("Int: %d\n", 42);
    // printf("Hex: 0x%x\n", 0xABC);
    // printf("Str: %s\n", "Hello");
    // printf("Char: %c\n", 'X');
    // printf("Percent: %%\n");

    // // 边界测试
    // printf("Boundary Test:\n");
    // printf("INT_MIN: %d\n", -9223372036854775807LL - 1);
    // printf("NULL Str: %s\n", (char*)0);
    // printf("Unknown: %q\n"); // 未知格式测试

    // // 清屏测试
    // printf("Before Clear\n");
    // clear_screen();
    // printf("After Clear\n");

    // // 性能测试 (大量输出)
    // printf("Performance Test:\n");
    // for (int i = 0; i < 100; i++) {
    //     printf("Count: %d\n", i);
    // }
}