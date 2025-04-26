#include "os/c-api/il2cpp-config-platforms.h"

#include "os/File.h"
#include "os/c-api/File-c-api.h"
#include "os/c-api/Allocator.h"

extern "C"
{
    UnityPalFileAttributes UnityPalGetFileAttributes(const char* path, int* error)
    {
        return il2cpp::os::File::GetFileAttributes(path, error);
    }

    int32_t UnityPalGetFileStat(const char* path, UnityPalFileStat * stat, int* error)
    {
        il2cpp::os::FileStat cppStat;
        bool result = il2cpp::os::File::GetFileStat(path, &cppStat, error);

        stat->name = Allocator::CopyToAllocatedStringBuffer(cppStat.name);
        stat->attributes = cppStat.attributes;
        stat->creation_time = cppStat.creation_time;
        stat->last_access_time = cppStat.last_access_time;
        stat->last_write_time = cppStat.last_write_time;
        stat->length = cppStat.length;

        return result;
    }

    UnityPalFileHandle* UnityPalOpen(const char* path, int openMode, int accessMode, int shareMode, int options, int *error)
    {
        int localError;
        il2cpp::os::FileHandle* handle = il2cpp::os::File::Open(path, openMode, accessMode, shareMode, options, &localError);
        if (error != NULL)
            *error = localError;

        if (localError != il2cpp::os::kErrorCodeSuccess)
            return NULL;

        return handle;
    }

    int32_t UnityPalClose(UnityPalFileHandle* handle, int *error)
    {
        return il2cpp::os::File::Close(handle, error);
    }

    int UnityPalRead(UnityPalFileHandle* handle, char *dest, int count, int *error)
    {
        return il2cpp::os::File::Read(handle, dest, count, error);
    }

    int32_t UnityPalIsExecutable(const char* filename)
    {
        return il2cpp::os::File::IsExecutable(filename).Get();
    }
}
