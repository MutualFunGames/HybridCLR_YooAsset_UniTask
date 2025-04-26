#pragma once

enum { Baselib_SystemSemaphore_PlatformSize = 16 }; // max size of all android versions

#ifndef EXPORTED_SYMBOL
    #define EXPORTED_SYMBOL __attribute__((visibility("default")))
#endif
#ifndef IMPORTED_SYMBOL
    #define IMPORTED_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif
// From #include <signal.h>
int raise(int sig);
#ifdef __cplusplus
}
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

// Posix specification says alignof(max_align_t) should be min alignment supported.
// However, tests revealed that some pointers were only aligned to 8 byte!
#ifndef PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT
    #define PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT 8
#endif

// Cache line size in bytes
#ifndef PLATFORM_PROPERTY_CACHE_LINE_SIZE
    #define PLATFORM_PROPERTY_CACHE_LINE_SIZE 64
#endif
