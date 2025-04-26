#pragma once

#include "Atomic.h"
#include "../C/Baselib_Atomic_TypeSafe.h"

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        // Atomic Reference counter.
        // Provides a decrement that has release barrier semantics and acquire-release if the reference count reaches zero.
        // Increment does NOT provide any memory synchronization!
        //
        // This is useful for ensuring that no reads/writes are happening on a deleted object (i.e. on memory that may be reclaimed!)
        // By issuing a release barrier on every reference decrement, we ensure that no loads or reads are reordered after the decrement.
        // The acquire barrier on reaching zero then ensure that operations before previous decrement-releases are finished and
        // that any following memory reclaiming actions will not happen before the final release.
        struct atomic_reference_counter
        {
            atomic_reference_counter(int32_t initialValue = 0) : referenceCount(initialValue) {}

            atomic_reference_counter(const atomic_reference_counter& other) = delete;
            atomic_reference_counter& operator=(const atomic_reference_counter& other) = delete;
            atomic_reference_counter(atomic_reference_counter&& other) = delete;
            atomic_reference_counter& operator=(const atomic_reference_counter&& other) = delete;

            // Underlying counter. Direct access should only be necessary in exceptional cases!
            baselib::atomic<int32_t> referenceCount;


            // Decrements a reference counter with acquire-release memory-barrier if value reaches 0, otherwise release.
            // Returns true if operation caused counter to reach 0, false otherwise.
            // Note that we intentionally only overload the pre-decrement, as the semantics of post increment aren't entirely clear.
            FORCE_INLINE bool operator--() { return Baselib_atomic_ref_dec_32(&referenceCount.obj); }

            // Increments a reference counter by 1 relaxed memory barrier semantics.
            // Returns true if the reference counter was 0 prior to incrementing
            FORCE_INLINE bool operator++() { return referenceCount.fetch_add(1, baselib::memory_order_relaxed) == 0; }

            // Increments a reference counter by 1 relaxed memory barrier semantics.
            // Returns true if the reference counter was 0 prior to incrementing
            FORCE_INLINE bool operator++(int) { return referenceCount.fetch_add(1, baselib::memory_order_relaxed) == 0; }

            // Increments a reference counter by 1 relaxed memory barrier semantics.
            // Returns true if the reference counter was 0 prior to incrementing
            FORCE_INLINE bool acquire() { return referenceCount.fetch_add(1, baselib::memory_order_relaxed) == 0; }

            // Decrements a reference counter with acquire-release memory-barrier if value reaches 0, otherwise release.
            // Returns true if operation caused counter to reach 0, false otherwise.
            FORCE_INLINE bool release() { return Baselib_atomic_ref_dec_32(&referenceCount.obj); }
        };
    }
}
