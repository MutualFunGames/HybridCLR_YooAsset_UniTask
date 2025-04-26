#include "os/c-api/il2cpp-config-platforms.h"

#include "os/Path.h"
#include "utils/PathUtils.h"
#include "Allocator.h"

extern "C"
{
    const char* UnityPalGetTempPath()
    {
        return Allocator::CopyToAllocatedStringBuffer(il2cpp::os::Path::GetTempPath());
    }

    int32_t UnityPalIsAbsolutePath(const char* path)
    {
        if (path == NULL)
            return 0;
        std::string path_string = path;
        return il2cpp::os::Path::IsAbsolute(path_string);
    }
}
