#pragma once
#include <stddef.h>

enum { Baselib_SystemSemaphore_PlatformSize = sizeof(void*) * 4 }; // 16 on 32bit and 32 on 64bit

#define MAX_PATH PATH_MAX

#ifndef EXPORTED_SYMBOL
    #define EXPORTED_SYMBOL __attribute__((visibility("default")))
#endif
#ifndef IMPORTED_SYMBOL
    #define IMPORTED_SYMBOL
#endif

// From #include <signal.h>
#ifdef __cplusplus
extern "C" {
    extern int raise(int __sig) throw ();
}
#else
extern int raise(int __sig);
#endif

// SIGTRAP from #include <signal.h>
// checked via static assert in platform config.
#define DETAIL_BASELIB_SIGTRAP 5

#define BASELIB_DEBUG_TRAP() raise(DETAIL_BASELIB_SIGTRAP)

// Enable LLSC native support for supported compilers and architectures/profiles
#ifndef PLATFORM_HAS_NATIVE_LLSC
    #if COMPILER_CLANG && (defined(__arm__) || defined(__arm64__))
        #define PLATFORM_HAS_NATIVE_LLSC 1
    #else
        #define PLATFORM_HAS_NATIVE_LLSC 0
    #endif
#endif

#ifndef PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT
#   define PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT 1
#endif

#ifndef PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT
    #define PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT COMPILER_ALIGN_OF(max_align_t)
#endif

// Cache line size in bytes
#ifndef PLATFORM_PROPERTY_CACHE_LINE_SIZE
    #define PLATFORM_PROPERTY_CACHE_LINE_SIZE 64
#endif
