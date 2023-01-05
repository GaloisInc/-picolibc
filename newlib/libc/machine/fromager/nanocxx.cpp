#include <stdlib.h>
//#include <new>
#include "fromager.h"

void* operator new(size_t sz) {
    return malloc(sz);
}

void operator delete(void* ptr)  throw() {
    free(ptr);
}

extern "C" {
    void __cxa_pure_virtual() {
        __cc_flag_invalid();
    }
}
