#pragma once

#include <stdint.h>

#if defined(__cplusplus)
#include "os/Error.h"
typedef il2cpp::os::ErrorCode UnityPalErrorCode;

#else

typedef int32_t UnityPalErrorCode;

#endif


#if defined(__cplusplus)
extern "C"
{
#endif

int32_t UnityPalSuccess(UnityPalErrorCode code);

#if defined(__cplusplus)
}
#endif
