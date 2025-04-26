#pragma once

//
// The following is to ensure backwards compatibility for deprecated defines
// This file (and references) as well as the corresponding pre-include should be removed once deprecated defines
// have been removed in all Unity code repositories using baselib.
//

// Set for testing compliance integrity (all deprecated defines removed from code-base)
#ifndef BASELIB_DISABLE_DEPRECATED_DEFINES_HEADER

#ifndef PLATFORM_FUTEX_NATIVE_SUPPORT
#define PLATFORM_FUTEX_NATIVE_SUPPORT PLATFORM_HAS_NATIVE_FUTEX
#endif

#ifndef PLATFORM_LLSC_NATIVE_SUPPORT
#define PLATFORM_LLSC_NATIVE_SUPPORT PLATFORM_HAS_NATIVE_LLSC
#endif

#ifndef PLATFORM_POSIX_SOCKET_IP6_SUPPORT
#define PLATFORM_POSIX_SOCKET_IP6_SUPPORT PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT
#endif

#ifndef PLATFORM_CACHE_LINE_SIZE
#define PLATFORM_CACHE_LINE_SIZE PLATFORM_PROPERTY_CACHE_LINE_SIZE
#endif

#ifndef PLATFORM_MEMORY_MALLOC_MIN_ALIGNMENT
#define PLATFORM_MEMORY_MALLOC_MIN_ALIGNMENT PLATFORM_PROPERTY_MEMORY_MALLOC_MIN_ALIGNMENT
#endif

#endif // BASELIB_DISABLE_DEPRECATED_DEFINES_HEADER
