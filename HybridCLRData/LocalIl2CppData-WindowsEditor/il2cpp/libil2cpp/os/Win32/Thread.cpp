#include "il2cpp-config.h"

#if IL2CPP_TARGET_WINDOWS

#include "WindowsHeaders.h"
#include "os/Thread.h"

#include <processthreadsapi.h>

namespace il2cpp
{
namespace os
{
    bool Thread::GetCurrentThreadStackBounds(void** low, void** high)
    {
        GetCurrentThreadStackLimits((PULONG_PTR)low, (PULONG_PTR)high);
        return true;
    }
}
}

#endif
