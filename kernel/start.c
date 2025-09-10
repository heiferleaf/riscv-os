#include <stdint.h>  // 添加以便使用 uint8_t（可选，但规范）

extern void uart_puts(const char *s);

void start() {
    uart_puts("Hello OS\n");
    while (1) {}
}