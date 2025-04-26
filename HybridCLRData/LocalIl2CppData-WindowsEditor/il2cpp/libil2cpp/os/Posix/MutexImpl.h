#pragma once

#if IL2CPP_THREADS_PTHREAD
#include <pthread.h>

namespace il2cpp
{
namespace os
{
    class FastMutexImpl
    {
    public:

        FastMutexImpl()
        {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&m_Mutex, &attr);
            pthread_mutexattr_destroy(&attr);
        }

        ~FastMutexImpl()
        {
            pthread_mutex_destroy(&m_Mutex);
        }

        void Lock()
        {
            pthread_mutex_lock(&m_Mutex);
        }

        void Unlock()
        {
            pthread_mutex_unlock(&m_Mutex);
        }

        pthread_mutex_t* GetOSHandle()
        {
            return &m_Mutex;
        }

    private:
        pthread_mutex_t m_Mutex;
    };
}
}
#endif
