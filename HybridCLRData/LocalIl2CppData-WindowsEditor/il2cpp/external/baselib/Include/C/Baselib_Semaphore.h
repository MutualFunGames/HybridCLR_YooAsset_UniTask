#pragma once

// Baselib_Semaphore

// In computer science, a semaphore is a variable or abstract data type used to control access to a common resource by multiple processes in a concurrent
// system such as a multitasking operating system. A semaphore is simply a variable. This variable is used to solve critical section problems and to achieve
// process synchronization in the multi processing environment. A trivial semaphore is a plain variable that is changed (for example, incremented or
// decremented, or toggled) depending on programmer-defined conditions.
//
// A useful way to think of a semaphore as used in the real-world system is as a record of how many units of a particular resource are available, coupled with
// operations to adjust that record safely (i.e. to avoid race conditions) as units are required or become free, and, if necessary, wait until a unit of the
// resource becomes available.
//
// "Semaphore (programming)", Wikipedia: The Free Encyclopedia
// https://en.wikipedia.org/w/index.php?title=Semaphore_(programming)&oldid=872408126

// This is the max number of tokens guaranteed to be held by the semaphore at
// any given point in time. Tokens submitted that exceed this value may silently be discarded.
static const int32_t Baselib_Semaphore_MaxGuaranteedCount = UINT16_MAX;

#if PLATFORM_HAS_NATIVE_FUTEX
#include "Internal/Baselib_Semaphore_FutexBased.inl.h"
#else
#include "Internal/Baselib_Semaphore_SemaphoreBased.inl.h"
#endif

// Creates a counting semaphore synchronization primitive.
// Use Baselib_Semaphore_Free() to free semaphore.
//
// If there are not enough system resources to create a semaphore, process abort is triggered.
//
// For optimal performance, the returned Baselib_Semaphore should be stored at a cache aligned memory location.
//
// \returns          A struct representing a semaphore instance. Use Baselib_Semaphore_Free to free the semaphore.
BASELIB_INLINE_API Baselib_Semaphore Baselib_Semaphore_Create(void);

// Creates a counting semaphore synchronization primitive in-place with memory provided by caller.
// Use Baselib_Semaphore_FreeInplace() to free semaphore.
//
// If there are not enough system resources to create a semaphore, process abort is triggered.
//
// For optimal performance, created Baselib_Semaphore should be stored at a cache aligned memory location.
BASELIB_INLINE_API void Baselib_Semaphore_CreateInplace(Baselib_Semaphore* semaphore);

// Wait for semaphore token to become available
//
// This function is guaranteed to emit an acquire barrier.
// Returns if token was consumed or was woken up by Baselib_Semaphore_ResetAndReleaseWaitingThreads.
BASELIB_INLINE_API void Baselib_Semaphore_Acquire(Baselib_Semaphore* semaphore);

// Try to consume a token and return immediately.
//
// When successful this function is guaranteed to emit an acquire barrier.
//
// \returns          true if token was consumed. false if not.
COMPILER_WARN_UNUSED_RESULT
BASELIB_FORCEINLINE_API bool Baselib_Semaphore_TryAcquire(Baselib_Semaphore* semaphore)
{
    return Baselib_Semaphore_TrySpinAcquire(semaphore, 0);
}

// Try to consume a token.
//
// When successful this function is guaranteed to emit an acquire barrier.
//
// \param maxSpinCount  Max number of times to spin in user space before falling back to the kernel. The actual number
//                      may differ depending on the underlying implementation but will never exceed the maxSpinCount
//                      value.
// \returns          true if token was consumed. false if not.
COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_Semaphore_TrySpinAcquire(Baselib_Semaphore* semaphore, uint32_t maxSpinCount);

// Wait for semaphore token to become available
//
// When successful this function is guaranteed to emit an acquire barrier.
//
// Acquire with a zero timeout differs from TryAcquire in that TryAcquire is guaranteed to be a user space operation
// while Acquire may enter the kernel and cause a context switch.
//
// Timeout passed to this function may be subject to system clock resolution.
// If the system clock has a resolution of e.g. 16ms that means this function may exit with a timeout error 16ms earlier than originally scheduled.
//
// \param timeout   Time to wait for token to become available.
//
// \returns          true if token was consumed or was woken up by Baselib_Semaphore_ResetAndReleaseWaitingThreads. false if timeout was reached.
COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Baselib_Semaphore_TryTimedAcquire(Baselib_Semaphore* semaphore, uint32_t timeoutInMilliseconds);

// Submit tokens to the semaphore.
//
// When successful this function is guaranteed to emit a release barrier.
//
// Increase the number of available tokens on the semaphore by `count`. Any waiting threads will be notified there are new tokens available.
// If count reach `Baselib_Semaphore_MaxGuaranteedCount` this function may silently discard any overflow.
BASELIB_INLINE_API void Baselib_Semaphore_Release(Baselib_Semaphore* semaphore, uint16_t count);

// If threads are waiting on Baselib_Semaphore_Acquire / Baselib_Semaphore_TryTimedAcquire,
// releases enough tokens to wake them up. Otherwise consumes all available tokens.
//
// When successful this function is guaranteed to emit a release barrier.
//
// \returns          number of released threads.
BASELIB_INLINE_API uint32_t Baselib_Semaphore_ResetAndReleaseWaitingThreads(Baselib_Semaphore* semaphore);

// Reclaim resources and memory held by the semaphore.
//
// If threads are waiting on the semaphore, calling free will trigger an assert and may cause process abort.
// Calling this function with a nullptr result in a no-op
BASELIB_INLINE_API void Baselib_Semaphore_Free(Baselib_Semaphore* semaphore);

// Reclaim resources held by the semaphore. Caller is responsible for freeing memory pointed to by semaphore
//
// If threads are waiting on the semaphore, calling free will trigger an assert and may cause process abort.
// Calling this function with a nullptr result in a no-op
BASELIB_INLINE_API void Baselib_Semaphore_FreeInplace(Baselib_Semaphore* semaphore);
