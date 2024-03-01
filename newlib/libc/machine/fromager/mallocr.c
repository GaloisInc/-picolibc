/*
 * Implementation of `malloc` using Fromager/Cheesecloth intrinsics.
 *
 * The functions are split into separate objects ...TODO
 * Not all functions are currently implemented, so some of those objects will
 * be empty, and anything that calls the corresponding function will get a
 * linker error.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fromager.h>

// Specialized MicroRAM compiler intrinsics for memory allocation and memory
// safety checking.

// Allocate `size` bytes of memory.
char* __cc_malloc(size_t size);
// Free the allocation starting at `ptr`.
void __cc_free(char* ptr);

// Let the prover arbitrarily choose a word to poison in the range `start <=
// ptr < start + len`.  This function returns an offset within the range.  If
// `offset < len`, then the word starting at `start + offset` should be
// poisoned; otherwise, nothing should be poisoned.
uintptr_t __cc_advise_poison_offset(char* start, uintptr_t len);

// Write `val` to `*ptr` and poison `*ptr`.  If `*ptr` is already poisoned, the
// trace is invalid.
void __cc_write_and_poison(uintptr_t* ptr, uintptr_t val);


#define FROMAGER_SIMPLE_MALLOC


#ifndef FROMAGER_SIMPLE_MALLOC

#ifdef DISABLE_MALLOC_POISON
#error "DISABLE_MALLOC_POISON set without FROMAGER_SIMPLE_MALLOC, but this allocator has not been tested with DISABLE_MALLOC_POISON yet. Set FROMAGER_SIMPLE_MALLOC if you need DISABLE_MALLOC_POISON."
#endif

#ifdef DEFINE_MALLOC
// Allocate a block of `size` bytes.
char* malloc_internal(size_t size) {
    char* ptr = __cc_malloc(size + 2 * sizeof(uintptr_t));

    // Compute and validate the size of the allocation provided by the prover.
    uintptr_t addr = (uintptr_t)ptr;
    size_t region_size = 1ull << ((addr >> 58) & 63);
    // The allocated region must have space for `size` bytes, plus two
    // additional words for metadata.
    __cc_valid_if(region_size >= size + 2 * sizeof(uintptr_t),
        "allocated region size is too small");
    __cc_valid_if(addr % region_size == 0,
        "allocated address is misaligned for its region size");
    // Note that `region_size` is always a power of two and is at least the
    // word size, so the address must be a multiple of the word size.

    // Write two words of metadata at the end of the allocated region.

    // Write 1 (allocated) to the first metadata word, and poison it to prevent
    // tampering.  This will make the trace invalid if the metadata word is
    // already poisoned (this happens if the prover tries to return the same
    // region for two separate allocations).
    uintptr_t* metadata = (uintptr_t*)(ptr + region_size - 2 * sizeof(uintptr_t));
    __cc_write_and_poison(metadata, 1);

    // Write the original size of the allocation to the second metadata word.
    size_t* size_ptr = (size_t*)(ptr + region_size - sizeof(uintptr_t));
    __cc_write_unchecked((uintptr_t*)size_ptr, (uintptr_t)size);

    __cc_access_valid(ptr, ptr + size);

    // Choose a word to poison in the range `ptr .. metadata`.
    //
    // FIXME: If the program touches only the second metadata word (the size
    // field), then we can't catch that out-of-bounds access since there is no
    // way to poison that word at the moment.
    char* padding_start = ptr + size;
    uintptr_t padding_len = (char*)metadata - padding_start;
    uintptr_t poison_offset = __cc_advise_poison_offset(padding_start, padding_len);
    if (poison_offset < padding_len) {
        uintptr_t* poison = (uintptr_t*)(padding_start + poison_offset);
        // The poisoned address must be well-aligned.
        __cc_valid_if((uintptr_t)poison % sizeof(uintptr_t) == 0,
            "poison address is not word-aligned");
        // The poisoned address is guaranteed to be in the unused space at the
        // end of the region.
        __cc_write_and_poison(poison, 0);
    }

    return ptr;
}

void* malloc(size_t size) {
    return (void*)malloc_internal(size);
}
#endif

#ifdef DEFINE_FREE
void free_internal(char* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Get the allocation size.
    uintptr_t log_region_size = (uintptr_t)ptr >> 58;
    uintptr_t region_size = 1ull << log_region_size;

    // Ensure `ptr` points to the start of a region.
    __cc_bug_if((uintptr_t)ptr % region_size != 0,
        "freed pointer not the start of a region");

    // Write to `*ptr`.  This memory access lets us catch double-free and
    // free-before-alloc by turning them into use-after-free and
    // use-before-alloc bugs, which we catch by other means.
    (*ptr) = 0;

    size_t* size_ptr = (size_t*)(ptr + region_size - sizeof(uintptr_t));
    size_t size = (size_t)__cc_read_unchecked((uintptr_t*)size_ptr);
    __cc_access_invalid(ptr, ptr + size);

    // Choose a word to poison within the freed region.  Note we forbid
    // choosing the metadata word, which is already poisoned.
    char* freed_start = ptr;
    uintptr_t freed_len = region_size - 2 * sizeof(uintptr_t);
    uintptr_t poison_offset = __cc_advise_poison_offset(freed_start, freed_len);
    if (poison_offset < freed_len) {
        uintptr_t* poison = (uintptr_t*)(freed_start + poison_offset);
        // The poisoned address must be well-aligned.
        __cc_valid_if((uintptr_t)poison % sizeof(uintptr_t) == 0,
            "poison address is not word-aligned");
        // The pointer is guaranteed to be somewhere within the freed region.
        __cc_write_and_poison(poison, 0);
    }
}

void free(void* ptr) {
    free_internal((char*)ptr);
}
#endif

#ifdef DEFINE_REALLOC
void *realloc(void *ptr, size_t size) {
    uintptr_t log_region_size = (uintptr_t)ptr >> 58;
    uintptr_t region_size = 1ull << log_region_size;
    __cc_bug_if((uintptr_t)ptr % region_size != 0,
        "realloc'd pointer not the start of a region");

    size_t* size_ptr = (size_t*)(ptr + region_size - sizeof(uintptr_t));
    size_t old_size = (size_t)__cc_read_unchecked((uintptr_t*)size_ptr);

    size_t copy_size = old_size < size ? old_size : size;
    void* new_ptr = malloc(size);
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);

    return new_ptr;
}
#endif

#ifdef DEFINE_MEMALIGN
int posix_memalign(void **memptr, size_t alignment, size_t size) {
    // `malloc(N)` always returns a pointer that is aligned to the next power
    // of two >= `N`.
    if (alignment > size) {
        size = alignment;
    }
    *memptr = malloc(size);
    return 0;
}
#endif

#else // FROMAGER_SIMPLE_MALLOC

#if FROMAGER_TRACE
# include <stdio.h>
# define TRACE(...) fprintf(stderr, "[TRACE] " __VA_ARGS__)
#else
# define TRACE(...) ((void)0)
#endif

#define POS_INIT 0x100000000ul

#ifdef DEFINE_MALLOC
void* malloc(size_t size) {
    void* out;
    posix_memalign(&out, 16, size);
    TRACE("malloc: %lu bytes at %lx\n", size, out);
    return out;
}
#endif

#ifdef DEFINE_FREE
void free(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    if ((uintptr_t)ptr < POS_INIT) {
        // XXX: hack - ignore free of non-heap pointers.  LLVM memory folding
        // converts some heap allocations into statics, but doesn't prevent the
        // program from calling `free` on those allocations later.
        return;
    }

    size_t size = (size_t)__cc_read_unchecked((uintptr_t*)(ptr - sizeof(uintptr_t)));
    __cc_access_invalid(ptr, ptr + size);
    // TODO: detect invalid free + double free
    // TODO: if DISABLE_MALLOC_POISON is not set, poison freed memory
}
#endif

#ifdef DEFINE_REALLOC
void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }

    TRACE("realloc %x to %d\n", (uintptr_t)ptr, size);
    size_t old_size = (size_t)__cc_read_unchecked((uintptr_t*)(ptr - sizeof(uintptr_t)));
    size_t copy_size = old_size < size ? old_size : size;
    TRACE("  got old size %d, copy %d", old_size, copy_size);
    void* new_ptr = malloc(size);
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);

    TRACE("realloc: %lu bytes at %lx\n", size, new_ptr);
    return new_ptr;
}
#endif

#ifdef DEFINE_MEMALIGN
void __cc_malloc_init(void* addr) __attribute__((noinline));

// Two words of padding, so there's always at least one well-aligned word
// somewhere within the padding.
#define MALLOC_PADDING 64

static uintptr_t pos = 0;

void* __cc_heap_snapshot(size_t* len) __attribute__((noinline)) {
    *len = pos - POS_INIT;
    return (void*)POS_INIT;
}

void* __cc_malloc_heap_start() __attribute__((noinline)) {
    return (void*)POS_INIT;
}

void* __cc_malloc_heap_end() __attribute__((noinline)) {
    return (void*)pos;
}

void __cc_malloc_set_heap_end(void* new_end) __attribute__((noinline)) {
    pos = (uintptr_t)new_end;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) __attribute__((noinline)) {
    if (!pos) {
        pos = POS_INIT;
        __cc_malloc_init((void*)pos);
    }

    if (alignment < sizeof(uintptr_t)) {
        alignment = sizeof(uintptr_t);
    }

    pos += sizeof(uintptr_t);
    pos = (pos + alignment - 1) & ~(alignment - 1);
    *memptr = (void*)pos;
    __cc_access_valid((char*)pos, (char*)pos + size);
    __cc_write_unchecked((uintptr_t*)(pos - sizeof(uintptr_t)), (uintptr_t)size);
    pos += size;

    char* padding_start = (char*)pos;
    pos += MALLOC_PADDING;

#ifndef DISABLE_MALLOC_POISON
    uintptr_t poison_offset = __cc_advise_poison_offset(padding_start, MALLOC_PADDING);
    if (poison_offset < MALLOC_PADDING) {
        uintptr_t* poison = (uintptr_t*)(padding_start + poison_offset);
        // The poisoned address must be well-aligned.
        __cc_valid_if((uintptr_t)poison % sizeof(uintptr_t) == 0,
            "poison address is not word-aligned");
        // The pointer is guaranteed to be somewhere within the padding region.
        __cc_write_and_poison(poison, 0);
    }
#endif

    return 0;
}
#endif

#endif // FROMAGER_SIMPLE_MALLOC


// Functions common to both malloc implementations

#ifdef DEFINE_CALLOC
void* calloc(size_t count, size_t size) {
    size_t total_size;
    __cc_valid_if(__builtin_mul_overflow(count, size, &total_size),
            "calloc size overflowed");
    return malloc(total_size);
}
#endif
