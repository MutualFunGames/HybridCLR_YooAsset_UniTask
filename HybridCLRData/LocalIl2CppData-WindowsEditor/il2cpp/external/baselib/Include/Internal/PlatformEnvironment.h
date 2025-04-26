#pragma once

// This header handles the selection of the correct compiler and platform
// environment for the current build.

#if _MSC_VER
    #include "Compiler/CompilerEnvironmentMsvc.h"
#elif __clang__
    #include "Compiler/CompilerEnvironmentClang.h"
#elif __GNUC__ || __GCC__
    #include "Compiler/CompilerEnvironmentGcc.h"
#else
    #error "Unknown Compiler"
#endif

// There is one platform specific environment header for every platform.
// You need to specify the right platform specific include path for the correct one to be picked up.
#include "BaselibPlatformSpecificEnvironment.h"

// The following platform environment definitions defaults to 1 unless defined otherwise in BaselibPlatformSpecificEnvironment.
#ifndef PLATFORM_HAS_NATIVE_FUTEX
    #define PLATFORM_HAS_NATIVE_FUTEX 1
#endif

#ifndef PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT
    #define PLATFORM_HAS_POSIX_SOCKET_IPV6_SUPPORT 1
#endif

// Verifies all expected platform environment definitions has been declared.
#include "VerifyPlatformEnvironment.h"

#ifndef BASELIB_DEBUG_TRAP
    #define BASELIB_DEBUG_TRAP COMPILER_DEBUG_TRAP
#endif
