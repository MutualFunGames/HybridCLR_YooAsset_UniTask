#include "il2cpp-config.h"

#if IL2CPP_TARGET_WINDOWS

#include "WindowsHelpers.h"

#include "os/NativeMethods.h"
#include "utils/Expected.h"
#include "utils/Il2CppError.h"

namespace il2cpp
{
namespace os
{
    bool NativeMethods::CloseProcess(ProcessHandle* handle)
    {
        return ::CloseHandle(handle) != FALSE;
    }

    utils::Expected<bool> NativeMethods::GetExitCodeProcess(ProcessHandle* handle, int32_t* exitCode)
    {
        return ::GetExitCodeProcess((HANDLE)handle, (LPDWORD)exitCode);
    }

    int32_t NativeMethods::GetCurrentProcessId()
    {
        return ::GetCurrentProcessId();
    }

    utils::Expected<ProcessHandle*> NativeMethods::GetCurrentProcess()
    {
        return (ProcessHandle*)::GetCurrentProcess();
    }
}
}

#endif
