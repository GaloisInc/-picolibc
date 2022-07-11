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
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include "fromager.h"
#include "cc_native.h"
#include "noni.h"

// No ops for taint analysis.
label cc_current_label = bottom;
void noniSetLabelU8( uint8_t* p, label l) {}
void noniSinkU8( uint8_t* p, label l) {}
void noniSetLabelI8( int8_t* p, label l) {}
void noniSinkI8( int8_t* p, label l) {}
void noniSetLabelU16( uint16_t* p, label l) {}
void noniSinkU16( uint16_t* p, label l) {}
void noniSetLabelI16( int16_t* p, label l) {}
void noniSinkI16( int16_t* p, label l) {}
void noniSetLabelU32( uint32_t* p, label l) {}
void noniSinkU32( uint32_t* p, label l) {}
void noniSetLabelI32( int32_t* p, label l) {}
void noniSinkI32( int32_t* p, label l) {}
void noniSetLabelU64( uint64_t* p, label l) {}
void noniSinkU64( uint64_t* p, label l) {}
void noniSetLabelI64( int64_t* p, label l) {}
void noniSinkI64( int64_t* p, label l) {}


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


uintptr_t __cc_advise_poison_offset(char* start, uintptr_t len) {
    return len;
}

void __cc_write_and_poison(uintptr_t* ptr, uintptr_t val) {
    *ptr = val;
}


void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return cc_native_mmap(addr, length, prot, flags, fd, offset);
}

int open(const char* name, int flags, ...) {
    int mode = 0;
    if (flags & O_CREAT) {
        va_list va;
        va_start(va, flags);
        mode = va_arg(va, mode_t);
        va_end(va);
    }
    printf("RECORDING: open(%p, %d, %d)", name, flags, mode);
    int ret = cc_native_open(name, flags, mode);
    printf(" = %d\n", ret);
    return ret;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    printf("RECORDING: connect(%d, %p, %d)", sockfd, addr, addrlen);
    int ret = cc_native_connect(sockfd, addr, addrlen);
    printf(" = %d\n", ret);
    return ret;
}

int shutdown(int sockfd, int how) {
    printf("RECORDING: shutdown(%d, %d)", sockfd, how);
    int ret = cc_native_shutdown(sockfd, how);
    printf(" = %d\n", ret);
    return ret;
}


int fstat(int fd, struct stat* st) {
    printf("RECORDING: fstat(%d, %p)", fd, st);
    int ret = cc_native_fstat(fd, st);
    printf(" = %d ({ st_dev: %lu, st_ino: %lu })\n", ret, st->st_dev, st->st_ino); // TODO: These are the only field we care about for openssl.
    return ret;
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    printf("RECORDING: poll(%p, %lu, %d)", fds, nfds, timeout);
    int ret = cc_native_poll(fds, nfds, timeout);
    printf(" = %d (revents = %d)\n", ret, fds->revents); // TODO: Only prints the first fds's revents.
    return ret;
}

off_t lseek(int fd, off_t offset, int whence) {
    printf("RECORDING: lseek(%d, %ld, %d)", fd, offset, whence);
    int ret = cc_native_lseek(fd, offset, whence);
    printf(" = %d\n", ret);
    return ret;
}

int kill(pid_t pid, int sig) {
    printf("RECORDING: kill(%d, %d)", pid, sig);
    int ret = cc_native_kill(pid, sig);
    printf(" = %d\n", ret);
    return ret;
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    printf("RECORDING: setsockopt(%d, %d, %d, %p, %d)", sockfd, level, optname, optval, optlen);
    int ret = cc_native_setsockopt(sockfd, level, optname, optval, optlen);
    printf(" = %d\n", ret);
    return ret;
}

// int ioctl(int fd, unsigned long request, ...) {
//     printf("RECORDING: ioctl(%d, %d, ...)", fd, request);
//     res = cc_native_ioctl() // What to do for varargs?
//     printf(" = %d\n", ret);
//     return ret;
// }

int close(int fd) {
    printf("RECORDING: close(%d)", fd);
    int ret = cc_native_close(fd);
    printf(" = %d\n", ret);
    return ret;
}

_READ_WRITE_RETURN_TYPE write(int fd, const void* buf, size_t count) {
    // printf("RECORDING: write(%d, %p, %d)", fd, buf, count);
    int ret = cc_native_write(fd, buf, count);
    // printf(" = %d\n", ret);
    return ret;
}

void print_buf(void* buf, int count) {
    printf("{\n  ");
    for (int z=0; z<count; z++) {
        printf("0x%02X",((uint8_t*) buf)[z]);
        if (z + 1 == count) {
            printf("\n};\n");
        }
        else if ((z+1)%16 == 0) {
            printf(",\n  ");
        }
        else {
            printf(", ");
        }
    }
}

_READ_WRITE_RETURN_TYPE read(int fd, void* buf, size_t count) {
    printf("RECORDING: read(%d, %p, %zu)", fd, buf, count);
    int ret = cc_native_read(fd, buf, count);
    printf(" = %d\n", ret);
    static int num = 0;
    printf("unsigned char read_buf_%d[%d] = ", num++, ret);
    print_buf(buf, ret);
    return ret;
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


int socket(int domain, int type, int protocol) {
    printf("RECORDING: socket(%d,%d,%d)", domain, type, protocol);
    int ret = cc_native_socket(domain, type, protocol);
    printf(" = %d\n", ret);
    return ret;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    printf("RECORDING: bind(%d,%d,%d)", sockfd, addr->sa_family, addrlen); // TODO: Log full addr.
    int ret = cc_native_bind(sockfd, addr, addrlen);
    printf(" = %d\n", ret);
    return ret;
}

int listen(int sockfd, int backlog) {
    printf("RECORDING: listen(%d,%d)", sockfd, backlog);
    int ret = cc_native_listen(sockfd, backlog);
    printf(" = %d\n", ret);
    return ret;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    printf("RECORDING: accept(%d,%d,%p)", sockfd, addr->sa_family, addrlen); // TODO: Log full addr.
    int ret = cc_native_accept(sockfd, addr, addrlen);
    printf(" = %d\n", ret);
    return ret;
}


void __cc_trace_exec(
        const char* name,
        uintptr_t arg0,
        uintptr_t arg1,
        uintptr_t arg2,
        uintptr_t arg3) {
    // Avoid infinite recursion.  `printf` gets instrumented, so
    // `__cc_trace_exec` calls `printf` and `printf` calls `__cc_trace_exec`.
    static int depth = 0;
    if (depth > 0) {
        return;
    }
    ++depth;

    int count = 4;
    if (count == 4 && arg3 == 0) { --count; }
    if (count == 3 && arg2 == 0) { --count; }
    if (count == 2 && arg1 == 0) { --count; }
    if (count == 1 && arg0 == 0) { --count; }

    printf("[FUNC] %s(", name);
    if (count >= 1) { printf("%lx", arg0); }
    if (count >= 2) { printf(", %lx", arg1); }
    if (count >= 3) { printf(", %lx", arg2); }
    if (count >= 4) { printf(", %lx", arg3); }
    printf(")\n");

    --depth;
}
