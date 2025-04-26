#pragma once

#include "OSGlobalEnums.h"

#if defined(__cplusplus)
#include "os/ErrorCodes.h"
#include "os/File.h"

typedef il2cpp::os::FileHandle UnityPalFileHandle;

#else

typedef struct UnityPalFileHandle UnityPalFileHandle;

#endif //__cplusplus

typedef struct
{
    char* name;
    int32_t attributes;
    int64_t length;
    int64_t creation_time;
    int64_t last_access_time;
    int64_t last_write_time;
} UnityPalFileStat;

#if defined(__cplusplus)
extern "C"
{
#endif

UnityPalFileAttributes UnityPalGetFileAttributes(const char* path, int* error);
int32_t UnityPalGetFileStat(const char* path, UnityPalFileStat * stat, int* error);
UnityPalFileHandle* UnityPalOpen(const char* path, int openMode, int accessMode, int shareMode, int options, int *error);
int32_t UnityPalClose(UnityPalFileHandle* handle, int *error);
int UnityPalRead(UnityPalFileHandle* handle, char *dest, int count, int *error);
int32_t UnityPalIsExecutable(const char* filename);

#if defined(__cplusplus)
}
#endif
