#include "defs.h"

void *memset(void *dst, int c, unsigned long n) {
    unsigned char *p = dst;
    while (n-- > 0) *p++ = c;
    return dst;
}

void *memcpy(void *dst, const void *src, unsigned long n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n-- > 0) *d++ = *s++;
    return dst;
}