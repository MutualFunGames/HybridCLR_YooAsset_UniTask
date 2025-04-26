#pragma once

#if (IL2CPP_THREADS_PTHREAD || IL2CPP_THREADS_WIN32)

#include "os/Generic/WaitObject.h"
#include "os/ErrorCodes.h"
#include "os/WaitStatus.h"

#include <stdint.h>

namespace il2cpp
{
namespace os
{
    class SemaphoreImpl : public WaitObject
    {
    public:

        SemaphoreImpl(int32_t initialValue, int32_t maximumValue)
            : WaitObject(kSemaphore)
            , m_MaximumValue(maximumValue)
        {
            m_Count = initialValue;
        }

        ~SemaphoreImpl()
        {
        }

        bool Post(int32_t releaseCount, int32_t* previousCount)
        {
            uint32_t oldCount;
            {
                WaitObject::ReleaseOnDestroy lock(m_Mutex);

                oldCount = m_Count;

                // Make sure we stay within range. Account for 32bit overflow.
                if (static_cast<uint64_t>(oldCount) + releaseCount > m_MaximumValue)
                    return false;

                m_Count += releaseCount;

                WakeupOneThread();
            }

            if (previousCount)
                *previousCount = oldCount;

            return true;
        }

    protected:
        uint32_t m_MaximumValue;
    };
}
}

#endif
