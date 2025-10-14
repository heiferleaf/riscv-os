#include <stdint.h>

#include "defs.h"

// register_interrupt(UART0_IRQ, uartintr);

static inline void mmio_write(uint64_t addr, uint8_t value) {
    *(volatile uint8_t *)addr = value;
}

static inline uint8_t mmio_read(uint64_t addr) {
    return *(volatile uint8_t *)addr;
}

void uart_putc(char c) {
    // 等待 UART 空闲
    while ((mmio_read(UART0 + LSR) & LSR_TX_IDLE) == 0);
    mmio_write(UART0 + THR, c);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}
