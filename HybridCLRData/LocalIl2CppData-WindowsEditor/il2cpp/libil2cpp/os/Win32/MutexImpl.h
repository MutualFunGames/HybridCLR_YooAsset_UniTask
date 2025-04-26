#pragma once

#if IL2CPP_THREADS_WIN32
#include "os/Win32/WindowsHeaders.h"

namespace il2cpp
{
namespace os
{
    class FastMutexImpl
    {
    public:

        FastMutexImpl()
        {
            InitializeCriticalSection(&m_CritialSection);
        }

        ~FastMutexImpl()
        {
            DeleteCriticalSection(&m_CritialSection);
        }

        void Lock()
        {
            EnterCriticalSection(&m_CritialSection);
        }

        void Unlock()
        {
            LeaveCriticalSection(&m_CritialSection);
        }

        CRITICAL_SECTION* GetOSHandle()
        {
            return &m_CritialSection;
        }

    private:
        CRITICAL_SECTION m_CritialSection;
    };
}
}
#endif
