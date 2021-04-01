#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void __llvm__memcpy__p0i8__p0i8__i64(uint8_t *dest, const uint8_t *src, uint64_t len) {
    uint8_t* dest_end = dest + len;
    for (; dest != dest_end; ++src, ++dest) {
        *dest = *src;
    }
}

void __llvm__memmove__p0i8__p0i8__i64(uint8_t *dest, const uint8_t *src, uint64_t len) {
    if ((uintptr_t)dest <= (uintptr_t)src) {
        uint8_t* dest_end = dest + len;
        for (; dest != dest_end; ++src, ++dest) {
            *dest = *src;
        }
    } else {
        uint8_t* dest_start = dest;
        dest += len;
        src += len;
        while (dest != dest_start) {
            --src;
            --dest;
            *dest = *src;
        }
    }
}

void __llvm__memset__p0i8__i64(uint8_t *dest, uint8_t val, uint64_t len) {
    for (uint64_t i = 0; i < len; ++i) {
        dest[i] = val;
    }
}

uint32_t __llvm__bswap__i32(uint32_t x) {
    volatile uint32_t x0 = x;
    uint8_t a = x0 >> 0;
    uint8_t b = x >> 8;
    uint8_t c = x >> 16;
    uint8_t d = x >> 24;
    return d | (c << 8) | (b << 16) | (a << 24);
}

uint32_t __llvm__ctpop__i32(uint32_t x) {
    x = (x & 0x55555555) + ((x & 0xaaaaaaaa) >> 1);
    x = (x & 0x33333333) + ((x & 0xcccccccc) >> 2);
    x = (x & 0x0f0f0f0f) + ((x & 0xf0f0f0f0) >> 4);
    x = (x & 0x00ff00ff) + ((x & 0xff00ff00) >> 8);
    x = (x & 0x0000ffff) + ((x & 0xffff0000) >> 16);
    return x;
}
