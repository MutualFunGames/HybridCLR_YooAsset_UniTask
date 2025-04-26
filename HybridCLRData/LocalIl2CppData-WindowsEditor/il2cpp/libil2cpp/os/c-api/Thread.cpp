#include "os/c-api/il2cpp-config-platforms.h"

#include "Thread-c-api.h"
#include "os/Thread.h"

void UnityPalSleep(uint32_t milliseconds)
{
    il2cpp::os::Thread::Sleep(milliseconds, false);
}

UnityPalThreadId UnityPalGetCurrentThreadId()
{
    return il2cpp::os::Thread::GetCurrentThread()->Id();
}
