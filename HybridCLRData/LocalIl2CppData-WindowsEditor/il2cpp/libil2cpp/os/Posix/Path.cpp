#include "il2cpp-config.h"

#if IL2CPP_TARGET_POSIX && !IL2CPP_USE_PLATFORM_SPECIFIC_PATH
#include "os/Environment.h"
#include "os/Path.h"
#include "utils/PathUtils.h"
#include <string>

#if defined(__APPLE__)
#include "mach-o/dyld.h"
#elif IL2CPP_TARGET_LINUX || IL2CPP_TARGET_ANDROID
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#elif IL2CPP_TARGET_QNX
#include <unistd.h>
#endif

namespace il2cpp
{
namespace os
{
    std::string Path::GetExecutablePath()
    {
#if defined(__APPLE__)
        char path[1024];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0)
            return path;

        std::string result;
        result.resize(size + 1);
        _NSGetExecutablePath(&result[0], &size);
        return result;
#elif IL2CPP_TARGET_LINUX || IL2CPP_TARGET_ANDROID
        char path[PATH_MAX];
        char dest[PATH_MAX + 1];
        //readlink does not null terminate
        memset(dest, 0, PATH_MAX + 1);
        pid_t pid = getpid();
        sprintf(path, "/proc/%d/exe", pid);
        if (readlink(path, dest, PATH_MAX) == -1)
            return std::string();
        return dest;
#elif IL2CPP_TARGET_QNX
        char path[PATH_MAX];
        char dest[PATH_MAX + 1];
        pid_t pid = getpid();
        sprintf(path, "/proc/%d/exefile", pid);
        auto* fh = fopen(path, "r");
        if (fh)
        {
            const auto read = fread(dest, 1, sizeof(dest), fh);
            const auto errorFlag = ferror(fh);
            fclose(fh);
            if (errorFlag == 0)
            {
                return dest;
            }
        }
        return "";
#else
        return std::string();
#endif
    }

    std::string Path::GetApplicationFolder()
    {
        return utils::PathUtils::DirectoryName(GetExecutablePath());
    }

    std::string Path::GetTempPath()
    {
        static const char* tmpdirs[] = { "TMPDIR", "TMP", "TEMP", NULL};

        for (size_t i = 0; tmpdirs[i] != NULL; ++i)
        {
            std::string tmpdir = Environment::GetEnvironmentVariable(tmpdirs[i]);

            if (!tmpdir.empty())
                return tmpdir;
        }

#if IL2CPP_TARGET_ANDROID
        return std::string("/data/local/tmp");
#else
        return std::string("/tmp");
#endif
    }

    bool Path::IsAbsolute(const std::string& path)
    {
        return path[0] == '/';
    }
}
}

#endif
