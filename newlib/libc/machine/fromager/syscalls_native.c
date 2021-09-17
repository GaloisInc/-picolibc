// System calls and other low-level function implementations for the native
// target.  This file is used instead of `syscalls.c` for native builds.  Since
// native builds don't support MicroRAM compiler intrinsics, we must provide
// alternate definitions of those here.
//
// Currently, we directly invoke Linux system calls for functions such as
// `mmap` and `write`.  In the future, it might be possible to replace these
// with calls to the system libc for better portability.  Of course, linking
// two different libcs into a single binary isn't exactly a standard or
// well-supported configuration, so this will require some linker tricks.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include "fromager.h"
#include "cc_native.h"


// Indicate that the current trace is invalid.
void __cc_flag_invalid(void) {
    __cc_trace("INVALID");
    _exit(1);
}
// Indicate that the current trace has exhibited a bug.
void __cc_flag_bug(void) {
    __cc_trace("BUG");
}

// Print a message during evaluation in the MicroRAM interpreter.
void __cc_trace(const char* msg) {
    fprintf(stderr, "[TRACE] %s\n", msg);
}


uintptr_t __cc_read_unchecked(uintptr_t* ptr) {
    return *ptr;
}

void __cc_write_unchecked(uintptr_t* ptr, uintptr_t val) {
    *ptr = val;
}

void __cc_access_valid(char* start, char* end) {}
void __cc_access_invalid(char* start, char* end) {}


uintptr_t* __cc_advise_poison(char* start, char* end) {
    return NULL;
}

void __cc_write_and_poison(uintptr_t* ptr, uintptr_t val) {
    *ptr = val;
}


void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return cc_native_mmap(addr, length, prot, flags, fd, offset);
}

int open(const char* name, int flags, int mode) {
    return cc_native_open(name, flags, mode);
}

int close(int fd) {
    return cc_native_close(fd);
}

_READ_WRITE_RETURN_TYPE write(int fd, const void* buf, size_t count) {
    return cc_native_write(fd, buf, count);
}

_READ_WRITE_RETURN_TYPE read(int fd, void* buf, size_t count) {
    return cc_native_read(fd, buf, count);
}

void _exit(int status) {
    cc_native_exit(status);
}

void __cc_malloc_init(void* addr) __attribute__((noinline)) {
    void* addr2 = mmap(addr, 64 * 1024 * 1024, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (addr2 != addr) {
        abort();
    }
}

void __cc_malloc_init_from_snapshot(void* addr, size_t len) __attribute__((noinline)) {
    __cc_valid_if(__cc_malloc_heap_end() == NULL,
        "heap has already been initialized");
    void* heap = __cc_malloc_heap_start();
    void* heap2 = mmap(heap, 64 * 1024 * 1024, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (heap2 != heap) {
        abort();
    }
    memcpy(heap, addr, len);
    __cc_malloc_set_heap_end(heap + len);
}


static int fromager_getchar(FILE *file) {
    char c;
    read(0, &c, 1);
    return c;
}

static int fromager_putchar(char c, FILE *file) {
    write(1, &c, 1);
    return 0;
}

static int fromager_putchar_err(char c, FILE *file) {
    write(2, &c, 1);
    return 0;
}

static FILE __stdin = FDEV_SETUP_STREAM(NULL, fromager_getchar, NULL, _FDEV_SETUP_READ);
static FILE __stdout = FDEV_SETUP_STREAM(fromager_putchar, NULL, NULL, _FDEV_SETUP_WRITE);
static FILE __stderr = FDEV_SETUP_STREAM(fromager_putchar_err, NULL, NULL, _FDEV_SETUP_WRITE);

FILE *const __iob[3] = { &__stdin, &__stdout, &__stderr };


void __cc_trace_exec(
        const char* name,
        uintptr_t arg0,
        uintptr_t arg1,
        uintptr_t arg2,
        uintptr_t arg3,
        uintptr_t arg4,
        uintptr_t arg5,
        uintptr_t arg6,
        uintptr_t arg7) {
    // Avoid infinite recursion.  `printf` gets instrumented, so
    // `__cc_trace_exec` calls `printf` and `printf` calls `__cc_trace_exec`.
    static int depth = 0;
    if (depth > 0) {
        return;
    }
    ++depth;

    int count = 8;
    if (count == 8 && arg7 == 0) { --count; }
    if (count == 7 && arg6 == 0) { --count; }
    if (count == 6 && arg5 == 0) { --count; }
    if (count == 5 && arg4 == 0) { --count; }
    if (count == 4 && arg3 == 0) { --count; }
    if (count == 3 && arg2 == 0) { --count; }
    if (count == 2 && arg1 == 0) { --count; }
    if (count == 1 && arg0 == 0) { --count; }

    printf("[FUNC] %s(", name);
    if (count >= 1) { printf("%lx", arg0); }
    if (count >= 2) { printf(", %lx", arg1); }
    if (count >= 3) { printf(", %lx", arg2); }
    if (count >= 4) { printf(", %lx", arg3); }
    if (count >= 5) { printf(", %lx", arg4); }
    if (count >= 6) { printf(", %lx", arg5); }
    if (count >= 7) { printf(", %lx", arg6); }
    if (count >= 8) { printf(", %lx", arg7); }
    printf(")\n");

    --depth;
}
