#pragma once

#include "../Baselib_Atomic_TypeSafe.h"
#include "../Baselib_CountdownTimer.h"
#include "../Baselib_SystemFutex.h"
#include "../Baselib_Thread.h"
#include "Baselib_SpinLoop.h"

#if !PLATFORM_HAS_NATIVE_FUTEX
    #error "Only use this implementation on top of a proper futex, in all other situations us Baselib_BinarySemaphore_SemaphoreBased.inl.h"
#endif

// Space out to different cache lines.
// the idea here is that threads waking up from sleep should not have to
// access the cache line where count is stored, and only touch wakeups.
// the only exception to that rule is if we hit a timeout.
typedef struct Baselib_BinarySemaphore
{
    int32_t wakeups;
    char _cachelineSpacer0[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(int32_t)];
    int32_t state;
    char _cachelineSpacer1[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(int32_t)];
} Baselib_BinarySemaphore;

BASELIB_STATIC_ASSERT(sizeof(Baselib_BinarySemaphore) == PLATFORM_PROPERTY_CACHE_LINE_SIZE * 2, "Baselib_BinarySemaphore (Futex) size should match 2 * cacheline size");
BASELIB_STATIC_ASSERT(offsetof(Baselib_BinarySemaphore, wakeups) == (offsetof(Baselib_BinarySemaphore, state) - PLATFORM_PROPERTY_CACHE_LINE_SIZE), "Baselib_BinarySemaphore (futex) wakeups and count shouldn't share cacheline");

enum Detail_Baselib_BinarySemaphore_State
{
    Detail_Baselib_BinarySemaphore_CLOSED       = 0,
    Detail_Baselib_BinarySemaphore_OPEN         = 1,
};

static FORCE_INLINE bool Detail_Baselib_BinarySemaphoreContended(const int32_t state)
{
    return state < Detail_Baselib_BinarySemaphore_CLOSED;
}

static FORCE_INLINE bool Detail_Baselib_BinarySemaphore_ConsumeWakeup(Baselib_BinarySemaphore* semaphore)
{
    int32_t previousCount = Baselib_atomic_load_32_relaxed(&semaphore->wakeups);
    while (previousCount > 0)
    {
        if (Baselib_atomic_compare_exchange_weak_32_acquire_relaxed(&semaphore->wakeups, &previousCount, previousCount - 1))
            return true;
    }
    return false;
}

BASELIB_INLINE_API void Baselib_BinarySemaphore_CreateInplace(Baselib_BinarySemaphore* semaphoreData)
{
    semaphoreData->wakeups = 0;
    semaphoreData->state = Detail_Baselib_BinarySemaphore_CLOSED;
}

BASELIB_INLINE_API Baselib_BinarySemaphore Baselib_BinarySemaphore_Create(void)
{
    Baselib_BinarySemaphore semaphore;
    Baselib_BinarySemaphore_CreateInplace(&semaphore);
    return semaphore;
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

    while (!Detail_Baselib_BinarySemaphore_ConsumeWakeup(semaphore))
    {
        Baselib_SystemFutex_Wait(&semaphore->wakeups, 0, UINT32_MAX);
    }
}

COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_BinarySemaphore_TryTimedAcquire(Baselib_BinarySemaphore* semaphore, const uint32_t timeoutInMilliseconds)
{
    const int32_t previousState = Baselib_atomic_fetch_add_32_acquire(&semaphore->state, -1);
    if (OPTIMIZER_LIKELY(previousState == Detail_Baselib_BinarySemaphore_OPEN))
        return true;

    if (Detail_Baselib_BinarySemaphore_ConsumeWakeup(semaphore))
        return true;

    uint32_t timeLeft = timeoutInMilliseconds;
    const Baselib_CountdownTimer timer = Baselib_CountdownTimer_StartMs(timeoutInMilliseconds);
    do
    {
        Baselib_SystemFutex_Wait(&semaphore->wakeups, 0, timeLeft);
        if (Detail_Baselib_BinarySemaphore_ConsumeWakeup(semaphore))
            return true;
        timeLeft = Baselib_CountdownTimer_GetTimeLeftInMilliseconds(timer);
    }
    while (timeLeft);

    // When timeout occurs we need to make sure we do one of the following:
    // Increase count by one from a negative value (give our acquired token back) or consume a wakeup.
    //
    // If count is not negative it's likely we are racing with a release operation in which case we
    // may end up having a successful acquire operation.
    do
    {
        int32_t previousState = Baselib_atomic_load_32_relaxed(&semaphore->state);
        while (Detail_Baselib_BinarySemaphoreContended(previousState))
        {
            if (Baselib_atomic_compare_exchange_weak_32_relaxed_relaxed(&semaphore->state, &previousState, previousState + 1))
                return false;
        }
        // Likely a race, yield to give the release operation room to complete.
        // This includes a full memory barrier which ensures that there is no reordering between changing/reading count and wakeup consumption.
        Baselib_Thread_YieldExecution();
    }
    while (!Detail_Baselib_BinarySemaphore_ConsumeWakeup(semaphore));
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
        Baselib_atomic_fetch_add_32_relaxed(&semaphore->wakeups, 1);
        Baselib_SystemFutex_Notify(&semaphore->wakeups, 1, Baselib_WakeupFallbackStrategy_OneByOne);
    }
    return true;
}

BASELIB_INLINE_API uint32_t Baselib_BinarySemaphore_ResetAndReleaseWaitingThreads(Baselib_BinarySemaphore* semaphore)
{
    const int32_t previousState = Baselib_atomic_exchange_32_release(&semaphore->state, Detail_Baselib_BinarySemaphore_CLOSED);
    if (OPTIMIZER_LIKELY(!Detail_Baselib_BinarySemaphoreContended(previousState)))
        return 0;

    const int32_t threadsToWakeup = -previousState;
    Baselib_atomic_fetch_add_32_relaxed(&semaphore->wakeups, threadsToWakeup);
    Baselib_SystemFutex_Notify(&semaphore->wakeups, threadsToWakeup, Baselib_WakeupFallbackStrategy_All);
    return threadsToWakeup;
}

BASELIB_INLINE_API void Baselib_BinarySemaphore_Free(Baselib_BinarySemaphore* semaphore)
{
    if (!semaphore)
        return;

    const int32_t state = Baselib_atomic_load_32_seq_cst(&semaphore->state);
    BaselibAssert(!Detail_Baselib_BinarySemaphoreContended(state), "Destruction is not allowed when there are still threads waiting on the semaphore.");
}

BASELIB_INLINE_API void Baselib_BinarySemaphore_FreeInplace(Baselib_BinarySemaphore* semaphore)
{
    Baselib_BinarySemaphore_Free(semaphore);
}
