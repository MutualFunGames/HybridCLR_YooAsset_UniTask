#pragma once

#include "../Baselib_Atomic_TypeSafe.h"
#include "../Baselib_SystemSemaphore.h"
#include "../Baselib_Thread.h"
#include "Baselib_SpinLoop.h"

#if PLATFORM_HAS_NATIVE_FUTEX
    #error "It's highly recommended to use Baselib_Lock_FutexBased.inl.h on platforms which has native lock support"
#endif

typedef struct Baselib_Lock
{
    Baselib_SystemSemaphore_Handle handle;
    int32_t count;
    // Make the lock take a full cache line so that if the user cacheline aligned lock,
    // llsc operations on count will not spuriously fail.
    char _cachelineSpacer[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(int32_t) - sizeof(Baselib_SystemSemaphore_Handle)];
    char _systemSemaphoreData[Baselib_SystemSemaphore_PlatformSize];
} Baselib_Lock;

BASELIB_STATIC_ASSERT((offsetof(Baselib_Lock, count) + PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(Baselib_SystemSemaphore_Handle)) ==
    offsetof(Baselib_Lock, _systemSemaphoreData), "count and internalData must not share cacheline");

BASELIB_INLINE_API Baselib_Lock Baselib_Lock_Create(void)
{
    Baselib_Lock lock = {Baselib_SystemSemaphore_Create(), 1, {0}, {0}};
    return lock;
}

BASELIB_INLINE_API void Baselib_Lock_CreateInplace(Baselib_Lock* lockData)
{
    lockData->handle = Baselib_SystemSemaphore_CreateInplace(&lockData->_systemSemaphoreData);
    lockData->count = 1;
}

COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_Lock_TrySpinAcquire(Baselib_Lock* lock, uint32_t maxSpinCount)
{
    int32_t previousCount = Baselib_atomic_load_32_relaxed(&lock->count);
    while (true)
    {
        while (OPTIMIZER_LIKELY(previousCount == 1))
        {
            if (OPTIMIZER_LIKELY(Baselib_atomic_compare_exchange_weak_32_acquire_relaxed(&lock->count, &previousCount, previousCount - 1)))
                return true;
        }
        if (!Detail_Baselib_SpinLoop(&lock->count, &previousCount, &maxSpinCount))
            return false;
    }
}

BASELIB_INLINE_API void Baselib_Lock_Acquire(Baselib_Lock* lock)
{
    const int32_t previousCount = Baselib_atomic_fetch_add_32_acquire(&lock->count, -1);
    if (OPTIMIZER_LIKELY(previousCount == 1))
        return;

    Baselib_SystemSemaphore_Acquire(lock->handle);
}

COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_Lock_TryTimedAcquire(Baselib_Lock* lock, const uint32_t timeoutInMilliseconds)
{
    const int32_t previousCount = Baselib_atomic_fetch_add_32_acquire(&lock->count, -1);
    if (OPTIMIZER_LIKELY(previousCount == 1))
        return true;

    if (OPTIMIZER_LIKELY(Baselib_SystemSemaphore_TryTimedAcquire(lock->handle, timeoutInMilliseconds)))
        return true;

    // When timeout occurs we need to make sure we do one of the following:
    // Increase count by one from a negative value (give our acquired token back) or consume a wakeup.
    //
    // If count is not negative it's likely we are racing with a release operation in which case we
    // may end up having a successful acquire operation.
    do
    {
        int32_t count = Baselib_atomic_load_32_relaxed(&lock->count);
        while (count < 0)
        {
            if (Baselib_atomic_compare_exchange_weak_32_relaxed_relaxed(&lock->count, &count, count + 1))
                return false;
        }
        // Likely a race, yield to give the release operation room to complete.
        // This includes a full memory barrier which ensures that there is no reordering between changing/reading count and wakeup consumption.
        Baselib_Thread_YieldExecution();
    }
    while (!Baselib_SystemSemaphore_TryAcquire(lock->handle));
    return true;
}

BASELIB_INLINE_API void Baselib_Lock_Release(Baselib_Lock* lock)
{
    int32_t previousCount = 0;
    while (!Baselib_atomic_compare_exchange_weak_32_release_relaxed(&lock->count, &previousCount, previousCount + 1))
    {
        if (previousCount == 1)
            return;
    }

    Baselib_Cpu_Hint_MonitorRelease();
    if (OPTIMIZER_UNLIKELY(previousCount < 0))
    {
        Baselib_SystemSemaphore_Release(lock->handle, 1);
    }
}

BASELIB_INLINE_API void Baselib_Lock_Free(Baselib_Lock* lock)
{
    if (!lock)
        return;
    const int32_t count = Baselib_atomic_load_32_seq_cst(&lock->count);
    BaselibAssert(count >= 0, "Destruction is not allowed when there are still threads waiting on the lock.");
    Baselib_SystemSemaphore_Free(lock->handle);
}

BASELIB_INLINE_API void Baselib_Lock_FreeInplace(Baselib_Lock* lock)
{
    if (!lock)
        return;
    const int32_t count = Baselib_atomic_load_32_seq_cst(&lock->count);
    BaselibAssert(count >= 0, "Destruction is not allowed when there are still threads waiting on the lock.");
    Baselib_SystemSemaphore_FreeInplace(lock->handle);
}
