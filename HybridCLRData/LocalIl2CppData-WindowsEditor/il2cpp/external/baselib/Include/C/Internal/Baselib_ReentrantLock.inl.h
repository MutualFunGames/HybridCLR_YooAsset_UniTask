#pragma once

#include "../Baselib_Lock.h"
#include "../Baselib_StaticAssert.h"
#include "../Baselib_Alignment.h"
#include "../Baselib_Thread.h"

typedef struct Baselib_ReentrantLock
{
    Baselib_Lock       lock;
    Baselib_Thread_Id  owner;
    int32_t            count;
} Baselib_ReentrantLock;

BASELIB_STATIC_ASSERT((BASELIB_ALIGN_OF(Baselib_ReentrantLock) + offsetof(Baselib_ReentrantLock, owner)) % sizeof(Baselib_Thread_Id) == 0, "Baselib_ReentrantLock::owner is not aligned for atomic use");
BASELIB_STATIC_ASSERT((BASELIB_ALIGN_OF(Baselib_ReentrantLock) + offsetof(Baselib_ReentrantLock, count)) % sizeof(int32_t) == 0, "Baselib_ReentrantLock::count is not aligned for atomic use");

BASELIB_INLINE_API Baselib_ReentrantLock Baselib_ReentrantLock_Create(void)
{
    Baselib_ReentrantLock lock = {Baselib_Lock_Create(), Baselib_Thread_InvalidId, 0};
    return lock;
}

BASELIB_INLINE_API void Baselib_ReentrantLock_CreateInplace(Baselib_ReentrantLock* lockData)
{
    Baselib_Lock_CreateInplace(&lockData->lock);
    lockData->owner = Baselib_Thread_InvalidId;
    lockData->count = 0;
}

COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_ReentrantLock_TrySpinAcquire(Baselib_ReentrantLock* lock, const uint32_t maxSpinCount)
{
    const Baselib_Thread_Id currentThreadId = Baselib_Thread_GetCurrentThreadId();
    const Baselib_Thread_Id lockOwner       = Baselib_atomic_load_ptr_relaxed(&lock->owner);
    if (OPTIMIZER_LIKELY(currentThreadId != lockOwner))
    {
        if (!Baselib_Lock_TrySpinAcquire(&lock->lock, maxSpinCount))
            return false;

        Baselib_atomic_store_ptr_relaxed(&lock->owner, currentThreadId);
        return true;
    }
    lock->count++;
    return true;
}

BASELIB_INLINE_API void Baselib_ReentrantLock_Acquire(Baselib_ReentrantLock* lock)
{
    const Baselib_Thread_Id currentThreadId = Baselib_Thread_GetCurrentThreadId();
    const Baselib_Thread_Id lockOwner       = Baselib_atomic_load_ptr_relaxed(&lock->owner);
    if (OPTIMIZER_LIKELY(currentThreadId != lockOwner))
    {
        Baselib_Lock_Acquire(&lock->lock);
        Baselib_atomic_store_ptr_relaxed(&lock->owner, currentThreadId);
        return;
    }
    lock->count++;
}

COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_ReentrantLock_TryTimedAcquire(Baselib_ReentrantLock* lock, const uint32_t timeoutInMilliseconds)
{
    const Baselib_Thread_Id currentThreadId = Baselib_Thread_GetCurrentThreadId();
    const Baselib_Thread_Id lockOwner       = Baselib_atomic_load_ptr_relaxed(&lock->owner);
    if (OPTIMIZER_LIKELY(currentThreadId != lockOwner))
    {
        if (!Baselib_Lock_TryTimedAcquire(&lock->lock, timeoutInMilliseconds))
            return false;
        Baselib_atomic_store_ptr_relaxed(&lock->owner, currentThreadId);
        return true;
    }
    lock->count++;
    return true;
}

BASELIB_INLINE_API void Baselib_ReentrantLock_Release(Baselib_ReentrantLock* lock)
{
    BaselibAssert(
        Baselib_atomic_load_ptr_relaxed(&lock->owner) == Baselib_Thread_GetCurrentThreadId() ||
        Baselib_atomic_load_ptr_relaxed(&lock->owner) == Baselib_Thread_InvalidId,
        "A recursive lock can only be unlocked by the locking thread");

    if (OPTIMIZER_LIKELY(lock->count == 0))
    {
        Baselib_atomic_store_ptr_relaxed(&lock->owner, Baselib_Thread_InvalidId);
        Baselib_Lock_Release(&lock->lock);
        return;
    }
    lock->count--;
}

BASELIB_INLINE_API void Baselib_ReentrantLock_Free(Baselib_ReentrantLock* lock)
{
    if (!lock)
        return;
    Baselib_Lock_Free(&lock->lock);
}

BASELIB_INLINE_API void Baselib_ReentrantLock_FreeInplace(Baselib_ReentrantLock* lock)
{
    if (!lock)
        return;
    Baselib_Lock_FreeInplace(&lock->lock);
}
