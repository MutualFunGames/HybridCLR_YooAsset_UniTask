#pragma once

#if !IL2CPP_THREADS_STD && IL2CPP_THREADS_WIN32

#include "os/ErrorCodes.h"
#include "os/Thread.h"
#include "os/WaitStatus.h"
#include "utils/NonCopyable.h"

#include "WindowsHeaders.h"
#include "os/Generic/WaitObject.h"
#include "Baselib.h"
#include "Cpp/CappedSemaphore.h"
#include "Cpp/Atomic.h"

#define IL2CPP_DEFAULT_STACK_SIZE ( 1 * 1024 * 1024)            // default .NET stacksize is 1mb

namespace il2cpp
{
namespace os
{
    class ThreadImpl : public il2cpp::utils::NonCopyable
    {
    public:
        ThreadImpl();
        ~ThreadImpl();

        static void AllocateStaticData();
        static void FreeStaticData();

        size_t Id();
        ErrorCode Run(Thread::StartFunc func, void* arg, int64_t affinityMask);
        void SetName(const char* name);
        void SetPriority(ThreadPriority priority);
        ThreadPriority GetPriority();

        void SetStackSize(size_t newsize)
        {
            // only makes sense if it's called BEFORE the thread has been created
            IL2CPP_ASSERT(m_ThreadHandle == NULL);
            // if newsize is zero we use the per-platform default value for size of stack
            if (newsize == 0)
            {
                newsize = IL2CPP_DEFAULT_STACK_SIZE;
            }
            m_StackSize = newsize;
        }

        void CheckForUserAPCAndHandle();
        void SetWaitObject(WaitObject* waitObject);
        void ReleaseSemaphore() {m_ConditionSemaphore.Release(1);}
        void AcquireSemaphore() {m_ConditionSemaphore.Acquire();}
        bool TryTimedAcquireSemaphore(uint32_t timeout) { return m_ConditionSemaphore.TryTimedAcquire(baselib::timeout_ms(timeout));}

        static int GetMaxStackSize();

        void QueueUserAPC(Thread::APCFunc func, void* context);

        ApartmentState GetApartment();
        ApartmentState GetExplicitApartment();
        ApartmentState SetApartment(ApartmentState state);
        void SetExplicitApartment(ApartmentState state);

        static void Sleep(uint32_t ms, bool interruptible);
        static size_t CurrentThreadId();
        static ThreadImpl* CreateForCurrentThread();
        static ThreadImpl* GetCurrentThread();

        static bool YieldInternal();

#if IL2CPP_HAS_NATIVE_THREAD_CLEANUP
        static void SetNativeThreadCleanup(Thread::ThreadCleanupFunc cleanupFunction);
        static void RegisterCurrentThreadForCleanup(void* arg);
        static void UnregisterCurrentThreadForCleanup();
        static void OnCurrentThreadExiting();
#endif

        baselib::atomic<WaitObject*> m_CurrentWaitObject;
        struct APCRequest
        {
            Thread::APCFunc callback;
            void* context;

            APCRequest(Thread::APCFunc callback, void* context) :
                callback(callback), context(context)
            {
            }
        };

        baselib::Lock m_PendingAPCsMutex;
        std::vector<APCRequest> m_PendingAPCs;
        baselib::CappedSemaphore m_ConditionSemaphore;

    private:
        HANDLE m_ThreadHandle;
        volatile DWORD m_ThreadId;
        SIZE_T m_StackSize;
        ApartmentState m_ApartmentState;
        ThreadPriority m_Priority;

        void SetNameForDebugger(const char* name);
    };
}
}

#endif
