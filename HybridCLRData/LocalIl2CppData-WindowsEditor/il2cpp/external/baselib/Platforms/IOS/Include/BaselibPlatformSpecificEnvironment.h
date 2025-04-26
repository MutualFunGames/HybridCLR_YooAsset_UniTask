#pragma once
#include <stddef.h>

/*
namespace detail
{
    struct Semaphore
    {
        dispatch_semaphore_t handle;
        baselib::atomic<int32_t> counter;
    };
}
*/
enum { Baselib_SystemSemaphore_PlatformSize = 16 }; // Size should match size of struct above from darwin api implementation

#ifndef EXPORTED_SYMBOL
    #define EXPORTED_SYMBOL __attribute__((visibility("default")))
#endif
#ifndef IMPORTED_SYMBOL
    #define IMPORTED_SYMBOL
#endif

#ifndef PLATFORM_HAS_NATIVE_FUTEX
    #define PLATFORM_HAS_NATIVE_FUTEX 0
#endif

// Enable LLSC native support for supported compilers and architectures/profiles
#ifndef PLATFORM_HAS_NATIVE_LLSC
    #if COMPILER_CLANG && (defined(__arm__) || defined(__arm64__))
        #define PLATFORM_HAS_NATIVE_LLSC 1
    #else
        #define PLATFORM_HAS_NATIVE_LLSC 0
    #endif
#endif

#ifndef PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT
    #define PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT COMPILER_ALIGN_OF(max_align_t)
#endif

// Cache line size in bytes
#ifndef PLATFORM_PROPERTY_CACHE_LINE_SIZE
    #define PLATFORM_PROPERTY_CACHE_LINE_SIZE 64
#endif
