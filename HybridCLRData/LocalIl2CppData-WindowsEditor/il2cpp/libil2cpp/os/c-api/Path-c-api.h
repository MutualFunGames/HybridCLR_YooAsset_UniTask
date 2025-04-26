#pragma once

#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif

char* UnityPalGetTempPath();
int32_t UnityPalIsAbsolutePath(const char* path);

#if defined(__cplusplus)
}
#endif
