#pragma once

#if (IL2CPP_THREADS_PTHREAD || IL2CPP_THREADS_WIN32)

#include <stdint.h>
#include <limits.h>
#include "utils/NonCopyable.h"
#include "os/WaitStatus.h"
#include "Baselib.h"
#include "Cpp/Lock.h"
#include "utils/dynamic_array.h"
#if IL2CPP_THREADS_WIN32
//#include "os/Win32/ThreadImpl.h"
#else
//#include "os/Posix/ThreadImpl.h"
#endif


namespace il2cpp
{
namespace os
{
    class ThreadImpl;
////TODO: generalize this so that it can be used with c++11 condition variables

    class WaitObject : public il2cpp::utils::NonCopyable
    {
    public:
        const uint32_t kNoTimeout = UINT_MAX;
        class ReleaseOnDestroy
        {
        public:
            FORCE_INLINE ReleaseOnDestroy(baselib::Lock& lockReference) : m_LockReference(lockReference) {m_LockReference.Acquire();}
            FORCE_INLINE ~ReleaseOnDestroy() { m_LockReference.Release(); }
        private:
            baselib::Lock& m_LockReference;
        };

        ~WaitObject();

        WaitStatus Wait(bool interruptible = false);
        WaitStatus Wait(uint32_t ms, bool interruptible = false);

        void* GetOSHandle();

        static void LockWaitObjectDeletion();
        static void UnlockWaitObjectDeletion();

    protected:

        enum Type
        {
            kMutex, /// All mutexes are recursive.
            kManualResetEvent,
            kAutoResetEvent,
            kSemaphore
        };

        struct SThreadPairPosix
        {
            SThreadPairPosix(ThreadImpl* _thread, WaitObject* _owner)
            {
                thread = _thread;
                owner = _owner;
            }

            ThreadImpl* thread;
            WaitObject* owner;
        };

        WaitObject(Type type);

        Type m_Type;

        /// Always have to acquire this mutex to touch m_Count.
        baselib::Lock m_Mutex;

        /// Array of threads waiting for this sync primitive
        il2cpp::utils::dynamic_array<SThreadPairPosix> m_WaitingThreads;

        /// "Release" count for the primitive. Means different things depending on the type of primitive
        /// but for all primitives, we wait until this is zero. Semaphores are the only primitive for which
        /// this can go past 1.
        uint32_t m_Count;

        /// Number of threads waiting on this object. This is used to prevent unnecessary signals
        /// on m_Condition.
        uint32_t m_WaitingThreadCount;

        bool HaveWaitingThreads() const { return (m_WaitingThreadCount != 0); }

        void WakeupOneThread() { WakeupThreads(true); }
        void WakeupAllThreads() { WakeupThreads(false); }
        void WakeupThreads(bool wakeupOneThread);

        void ConditionWait(ThreadImpl* thread);
        bool ConditionTimedWait(ThreadImpl* thread, uint32_t timeout);

        void PushThreadToWaitersList(WaitObject* owner, ThreadImpl* thread);
        void PopThreadFromWaitersList(ThreadImpl* thread);
    };
}
}

#endif // (IL2CPP_THREADS_PTHREAD || IL2CPP_THREADS_WIN32)
