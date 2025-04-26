#pragma once

//
// The following is to ensure backwards compatibility if any deprecated defines are preprocessor definitions
// This file (and references) as well as the corresponding post-include should be removed once deprecated defines
// have been removed in all Unity code repositories using baselib.
//

#ifdef PLATFORM_FUTEX_NATIVE_SUPPORT
    #define PLATFORM_HAS_NATIVE_FUTEX PLATFORM_FUTEX_NATIVE_SUPPORT
#endif

#ifdef PLATFORM_LLSC_NATIVE_SUPPORT
    #define PLATFORM_HAS_NATIVE_LLSC PLATFORM_LLSC_NATIVE_SUPPORT
#endif

#ifdef PLATFORM_POSIX_SOCKET_IP6_SUPPORT
    #define PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT PLATFORM_POSIX_SOCKET_IP6_SUPPORT
#endif

#ifdef PLATFORM_CACHE_LINE_SIZE
    #define PLATFORM_PROPERTY_CACHE_LINE_SIZE PLATFORM_CACHE_LINE_SIZE
#endif

#ifdef PLATFORM_MEMORY_MALLOC_MIN_ALIGNMENT
    #define PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT PLATFORM_MEMORY_MALLOC_MIN_ALIGNMENT
#endif
