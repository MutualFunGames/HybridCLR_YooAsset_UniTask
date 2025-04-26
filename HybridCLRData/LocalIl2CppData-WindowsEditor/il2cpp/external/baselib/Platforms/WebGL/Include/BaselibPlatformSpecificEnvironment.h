#pragma once

#ifndef __EMSCRIPTEN_PTHREADS__
/*
namespace detail
{
    struct Semaphore
    {
        uint32_t counter;
    };
}
*/
enum { Baselib_SystemSemaphore_PlatformSize = 4 }; // size should match size of struct above from no thread implementation
#else
enum { Baselib_SystemSemaphore_PlatformSize = 16 }; // sem_t with support for threads
#endif

#ifndef EXPORTED_SYMBOL
    #define EXPORTED_SYMBOL __attribute__((visibility("default")))
#endif
#ifndef IMPORTED_SYMBOL
    #define IMPORTED_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

// This file is included throughout baselib in many compilation units, avoid pulling inp
// extra includes, so forward-declare the symbol we need from #include <emscripten.h>
void emscripten_debugger(void);

#ifdef __cplusplus
}
#endif

// The debugger statement invokes any available debugging functionality, such as setting a breakpoint.
// If no debugging functionality is available, this statement has no effect.
#define BASELIB_DEBUG_TRAP() emscripten_debugger()

#ifndef PLATFORM_HAS_NATIVE_FUTEX
    #ifndef __EMSCRIPTEN_PTHREADS__
        #define PLATFORM_HAS_NATIVE_FUTEX 0
    #endif
#endif

#ifndef PLATFORM_HAS_NATIVE_LLSC
    #define PLATFORM_HAS_NATIVE_LLSC 0
#endif

#ifndef PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT
#   define PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT 1
#endif

// The default, std::max_align_t, has the same value as this, but C's max_align_t is not available.
// See: https://github.com/emscripten-core/emscripten/blob/2bca083cbbd5a4133db61fbd74d04f7feecfa907/tests/core/test_stddef.cpp
#ifndef PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT
// Currently Emscripten min alignment is at 8 bytes, independent of what max_align_t returns.
    #define PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT 8
#endif

// Cache line size in bytes
#ifndef PLATFORM_PROPERTY_CACHE_LINE_SIZE
    #define PLATFORM_PROPERTY_CACHE_LINE_SIZE 64
#endif
