#pragma once

#include "../Baselib_Atomic_TypeSafe.h"
#include "../Baselib_SystemSemaphore.h"
#include "../Baselib_Thread.h"
#include "Baselib_SpinLoop.h"

#if PLATFORM_HAS_NATIVE_FUTEX
    #error "It's highly recommended to use Baselib_BinarySemaphore_FutexBased.inl.h on platforms which has native semaphore support"
#endif

typedef struct Baselib_BinarySemaphore
{
    Baselib_SystemSemaphore_Handle handle;
    int32_t state;
    // Make the semaphore take a full cache line so that if the user cacheline aligned semaphore,
    // llsc operations on state will not spuriously fail.
    char _cachelineSpacer[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(int32_t) - sizeof(Baselib_SystemSemaphore_Handle)];
    char _systemSemaphoreData[Baselib_SystemSemaphore_PlatformSize];
} Baselib_BinarySemaphore;

BASELIB_STATIC_ASSERT((offsetof(Baselib_BinarySemaphore, state) + PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(Baselib_SystemSemaphore_Handle)) ==
    offsetof(Baselib_BinarySemaphore, _systemSemaphoreData), "state and internalData must not share cacheline");

enum Detail_Baselib_BinarySemaphore_State
{
    Detail_Baselib_BinarySemaphore_CLOSED       = 0,
    Detail_Baselib_BinarySemaphore_OPEN         = 1,
};

static FORCE_INLINE bool Detail_Baselib_BinarySemaphoreContended(const int32_t state)
{
    return state < Detail_Baselib_BinarySemaphore_CLOSED;
}

BASELIB_INLINE_API Baselib_BinarySemaphore Baselib_BinarySemaphore_Create(void)
{
    Baselib_BinarySemaphore semaphore = {Baselib_SystemSemaphore_Create(), Detail_Baselib_BinarySemaphore_CLOSED, {0}, {0}};
    return semaphore;
}

BASELIB_INLINE_API void Baselib_BinarySemaphore_CreateInplace(Baselib_BinarySemaphore* semaphoreData)
{
    semaphoreData->handle = Baselib_SystemSemaphore_CreateInplace(&semaphoreData->_systemSemaphoreData);
    semaphoreData->state = Detail_Baselib_BinarySemaphore_CLOSED;
}

COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_BinarySemaphore_TrySpinAcquire(Baselib_BinarySemaphore* semaphore, uint32_t maxSpinCount)
{
    int32_t previousState = Baselib_atomic_load_32_relaxed(&semaphore->state);
    while (true)
    {
        while (OPTIMIZER_LIKELY(previousState == Detail_Baselib_BinarySemaphore_OPEN))
        {
            if (OPTIMIZER_LIKELY(Baselib_atomic_compare_exchange_weak_32_acquire_relaxed(&semaphore->state, &previousState, Detail_Baselib_BinarySemaphore_CLOSED)))
                return true;
        }
        if (!Detail_Baselib_SpinLoop(&semaphore->state, &previousState, &maxSpinCount))
            return false;
    }
}

BASELIB_INLINE_API void Baselib_BinarySemaphore_Acquire(Baselib_BinarySemaphore* semaphore)
{
    const int32_t previousState = Baselib_atomic_fetch_add_32_acquire(&semaphore->state, -1);
    if (OPTIMIZER_LIKELY(previousState == Detail_Baselib_BinarySemaphore_OPEN))
        return;

    Baselib_SystemSemaphore_Acquire(semaphore->handle);
}

COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_BinarySemaphore_TryTimedAcquire(Baselib_BinarySemaphore* semaphore, const uint32_t timeoutInMilliseconds)
{
    const int32_t previousState = Baselib_atomic_fetch_add_32_acquire(&semaphore->state, -1);
    if (OPTIMIZER_LIKELY(previousState == Detail_Baselib_BinarySemaphore_OPEN))
        return true;

    if (OPTIMIZER_LIKELY(Baselib_SystemSemaphore_TryTimedAcquire(semaphore->handle, timeoutInMilliseconds)))
        return true;

    // When timeout occurs we need to make sure we do one of the following:
    // Increase state by one from a negative value (give our acquired token back) or consume a wakeup.
    //
    // If state is not negative it's likely we are racing with a release operation in which case we
    // may end up having a successful acquire operation.
    do
    {
        int32_t state = Baselib_atomic_load_32_relaxed(&semaphore->state);
        while (state < 0)
        {
            if (Baselib_atomic_compare_exchange_weak_32_relaxed_relaxed(&semaphore->state, &state, state + 1))
                return false;
        }
        // Likely a race, yield to give the release operation room to complete.
        // This includes a full memory barrier which ensures that there is no reordering between changing/reading state and wakeup consumption.
        Baselib_Thread_YieldExecution();
    }
    while (!Baselib_SystemSemaphore_TryAcquire(semaphore->handle));
    return true;
}

BASELIB_INLINE_API bool Baselib_BinarySemaphore_Release(Baselib_BinarySemaphore* semaphore)
{
    int32_t previousState = Detail_Baselib_BinarySemaphore_CLOSED;
    while (!Baselib_atomic_compare_exchange_weak_32_release_relaxed(&semaphore->state, &previousState, previousState + 1))
    {
        if (previousState == Detail_Baselib_BinarySemaphore_OPEN)
            return false;
    }

    Baselib_Cpu_Hint_MonitorRelease();
    if (OPTIMIZER_UNLIKELY(Detail_Baselib_BinarySemaphoreContended(previousState)))
    {
        Baselib_SystemSemaphore_Release(semaphore->handle, 1);
    }
    return true;
}

BASELIB_INLINE_API uint32_t Baselib_BinarySemaphore_ResetAndReleaseWaitingThreads(Baselib_BinarySemaphore* semaphore)
{
    const int32_t previousState = Baselib_atomic_exchange_32_release(&semaphore->state, Detail_Baselib_BinarySemaphore_CLOSED);
    if (OPTIMIZER_LIKELY(!Detail_Baselib_BinarySemaphoreContended(previousState)))
        return 0;

    const int32_t threadsToWakeup = -previousState;
    Baselib_SystemSemaphore_Release(semaphore->handle, threadsToWakeup);
    return threadsToWakeup;
}

BASELIB_INLINE_API void Baselib_BinarySemaphore_Free(Baselib_BinarySemaphore* semaphore)
{
    if (!semaphore)
        return;

    const int32_t state = Baselib_atomic_load_32_seq_cst(&semaphore->state);
    BaselibAssert(!Detail_Baselib_BinarySemaphoreContended(state), "Destruction is not allowed when there are still threads waiting on the semaphore.");
    Baselib_SystemSemaphore_Free(semaphore->handle);
}

BASELIB_INLINE_API void Baselib_BinarySemaphore_FreeInplace(Baselib_BinarySemaphore* semaphore)
{
    if (!semaphore)
        return;

    const int32_t state = Baselib_atomic_load_32_seq_cst(&semaphore->state);
    BaselibAssert(!Detail_Baselib_BinarySemaphoreContended(state), "Destruction is not allowed when there are still threads waiting on the semaphore.");
    Baselib_SystemSemaphore_FreeInplace(semaphore->handle);
}
