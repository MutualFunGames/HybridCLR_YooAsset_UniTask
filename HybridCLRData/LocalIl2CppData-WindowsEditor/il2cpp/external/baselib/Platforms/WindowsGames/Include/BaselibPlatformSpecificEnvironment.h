#pragma once

enum { Baselib_SystemSemaphore_PlatformSize = 1 }; // unused but 1 to simplify things

// These affect <windows.h> behavior; define them here so that no matter how/if later windows.h is included, it will get consistent result.
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX 1
#endif

#ifndef EXPORTED_SYMBOL
    #define EXPORTED_SYMBOL __declspec(dllexport)
#endif
#ifndef IMPORTED_SYMBOL
    #define IMPORTED_SYMBOL __declspec(dllimport)
#endif

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

// Malloc is specified to have 16 byte alignment on 64bit platforms.
// see https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/malloc?view=vs-2019
#ifndef PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT
    #if PLATFORM_ARCH_64
        #define PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT 16
    #else
        #define PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT 8
    #endif
#endif

// Cache line size in bytes
#ifndef PLATFORM_PROPERTY_CACHE_LINE_SIZE
    #define PLATFORM_PROPERTY_CACHE_LINE_SIZE 64
#endif
