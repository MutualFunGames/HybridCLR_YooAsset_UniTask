#include "os/c-api/il2cpp-config-platforms.h"

#include "os/Directory.h"
#include "os/c-api/Directory-c-api.h"
#include "Allocator.h"
#include "utils/StringUtils.h"
#include "utils/StringViewUtils.h"

#include <string>

extern "C"
{
    const char* UnityPalDirectoryGetCurrent(int* error)
    {
        return Allocator::CopyToAllocatedStringBuffer(il2cpp::os::Directory::GetCurrent(error));
    }

    UnityPalFindHandle* UnityPalDirectoryFindHandleNew(const char* searchPathWithPattern)
    {
        Il2CppNativeString pattern(il2cpp::utils::StringUtils::Utf8ToNativeString(searchPathWithPattern));
        return new il2cpp::os::Directory::FindHandle(STRING_TO_STRINGVIEW(pattern));
    }

    void UnityPalDirectoryFindHandleDelete(UnityPalFindHandle* object)
    {
        IL2CPP_ASSERT(object);
        delete object;
    }

    int32_t UnityPalDirectoryCloseOSHandle(UnityPalFindHandle* object)
    {
        IL2CPP_ASSERT(object);
        return object->CloseOSHandle();
    }

    UnityPalErrorCode UnityPalDirectoryFindFirstFile(UnityPalFindHandle* findHandle, const char* searchPathWithPattern, char** resultFileName, int32_t* resultAttributes)
    {
        Il2CppNativeString pattern(il2cpp::utils::StringUtils::Utf8ToNativeString(searchPathWithPattern));
        Il2CppNativeString nativeFileName;
        UnityPalErrorCode retVal = il2cpp::os::Directory::FindFirstFile(findHandle, STRING_TO_STRINGVIEW(pattern), &nativeFileName, resultAttributes);
        *resultFileName = Allocator::CopyToAllocatedStringBuffer(il2cpp::utils::StringUtils::NativeStringToUtf8(nativeFileName));
        return retVal;
    }

    UnityPalErrorCode UnityPalDirectoryFindNextFile(UnityPalFindHandle*  findHandle, char** resultFileName, int32_t* resultAttributes)
    {
        Il2CppNativeString nativeFileName;
        UnityPalErrorCode retVal = il2cpp::os::Directory::FindNextFile(findHandle, &nativeFileName, resultAttributes);
        *resultFileName = Allocator::CopyToAllocatedStringBuffer(il2cpp::utils::StringUtils::NativeStringToUtf8(nativeFileName));
        return retVal;
    }
}
