#pragma once

#include "../Baselib_Atomic.h"

// Utilities for atomic reference counter.
// Provides a decrement that has release barrier semantics and acquire-release if the reference count reaches zero.
// For an easy to use C++ atomic reference counter, use baselib::atomic_reference_counter (AtomicReferenceCounter.h)
//
// This is useful for ensuring that no reads/writes are happening on a deleted object (i.e. on memory that may be reclaimed!)
// By issuing a release barrier on every reference decrement, we ensure that no loads or reads are reordered after the decrement.
// The acquire barrier on reaching zero then ensure that operations before previous decrement-releases are visible and
// that any following memory reclaiming actions will not happen before the final release.

// Historical note:
// Baselib_atomic_ref_dec was previously known as AtomicRelease in Unity. Additionally, AtomicRetain provided a relaxed increment.


// On architectures where read-modify-write with memory_order_acq_rel is more expensive than memory_order_release
// the idea is to use a global memory_order_acquire fence instead, but only when the reference count drops to 0.
// Only then the acquire/release synchronization is needed to make sure everything prior to atomic_release happens before running a d'tor.
#if defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(_M_ARM64)

static FORCE_INLINE bool Baselib_atomic_ref_dec_16_v(void* obj)
{
    int16_t result;
    const int16_t value = -1;
    Baselib_atomic_fetch_add_16_release_v(obj, &value, &result);
    const bool reachedZero = (result == 1);
    if (reachedZero) Baselib_atomic_thread_fence_acquire();
    return reachedZero;
}

static FORCE_INLINE bool Baselib_atomic_ref_dec_32_v(void* obj)
{
    int32_t result;
    const int32_t value = -1;
    Baselib_atomic_fetch_add_32_release_v(obj, &value, &result);
    const bool reachedZero = (result == 1);
    if (reachedZero) Baselib_atomic_thread_fence_acquire();
    return reachedZero;
}

static FORCE_INLINE bool Baselib_atomic_ref_dec_64_v(void* obj)
{
    int64_t result;
    const int64_t value = -1;
    Baselib_atomic_fetch_add_64_release_v(obj, &value, &result);
    const bool reachedZero = (result == 1);
    if (reachedZero) Baselib_atomic_thread_fence_acquire();
    return reachedZero;
}

#else

static FORCE_INLINE bool Baselib_atomic_ref_dec_16_v(void* obj)
{
    int16_t result;
    const int16_t value = -1;
    Baselib_atomic_fetch_add_16_acq_rel_v(obj, &value, &result);
    return result == 1;
}

static FORCE_INLINE bool Baselib_atomic_ref_dec_32_v(void* obj)
{
    int32_t result;
    const int32_t value = -1;
    Baselib_atomic_fetch_add_32_acq_rel_v(obj, &value, &result);
    return result == 1;
}

static FORCE_INLINE bool Baselib_atomic_ref_dec_64_v(void* obj)
{
    int64_t result;
    const int64_t value = -1;
    Baselib_atomic_fetch_add_64_acq_rel_v(obj, &value, &result);
    return result == 1;
}

#endif
