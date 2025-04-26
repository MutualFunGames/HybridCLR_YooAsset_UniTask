#pragma once

#include "Error-c-api.h"
#include <stdint.h>

#if defined(__cplusplus)
#include "os/Directory.h"
typedef il2cpp::os::Directory::FindHandle UnityPalFindHandle;

#else

typedef struct UnityPalFindHandle UnityPalFindHandle;

#endif

#if defined(__cplusplus)
extern "C"
{
#endif

const char* UnityPalDirectoryGetCurrent(int* error);


UnityPalFindHandle* UnityPalDirectoryFindHandleNew(const char* searchPathWithPattern);
void UnityPalDirectoryFindHandleDelete(UnityPalFindHandle* object);

int32_t UnityPalDirectoryCloseOSHandle(UnityPalFindHandle* object);

UnityPalErrorCode UnityPalDirectoryFindFirstFile(UnityPalFindHandle* findHandle, const char* searchPathWithPattern, char** resultFileName, int32_t* resultAttributes);
UnityPalErrorCode UnityPalDirectoryFindNextFile(UnityPalFindHandle*  findHandle, char** resultFileName, int32_t* resultAttributes);

#if defined(__cplusplus)
}
#endif
