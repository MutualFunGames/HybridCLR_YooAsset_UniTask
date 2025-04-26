#pragma once

#if (IL2CPP_THREADS_PTHREAD || IL2CPP_THREADS_WIN32)

#include "os/ErrorCodes.h"
#include "os/WaitStatus.h"
#include "os/Generic/WaitObject.h"

namespace il2cpp
{
namespace os
{
    class EventImpl : public WaitObject
    {
    public:

        EventImpl(bool manualReset, bool signaled)
            : WaitObject(manualReset ? kManualResetEvent : kAutoResetEvent)
        {
            if (signaled)
                m_Count = 1;
        }

        ~EventImpl()
        {
        }

        ErrorCode Set()
        {
            WaitObject::ReleaseOnDestroy lock(m_Mutex);

            m_Count = 1;

            if (HaveWaitingThreads())
                WakeupAllThreads();

            return kErrorCodeSuccess;
        }

        ErrorCode Reset()
        {
            WaitObject::ReleaseOnDestroy lock(m_Mutex);
            m_Count = 0;

            return kErrorCodeSuccess;
        }
    };
}
}

#endif
