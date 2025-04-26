#pragma once

#include <stdint.h>
#include "Baselib.h"
#include "C/Baselib_Atomic_TypeSafe.h"
#include "Cpp/ReentrantLock.h"

namespace il2cpp
{
namespace utils
{
    // Initializes a pointer from NULL to non-NULL exactly once.
    // Uses double checked locking to avoid taking the lock after the pointer has been initialized.

    template<typename T, typename InitBlock>
    static inline T* InitOnce(T** value, baselib::ReentrantLock* lock, InitBlock init)
    {
        // Based on double checked locking implementation in https://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/

        T* tmp = (T*)Baselib_atomic_load_ptr_relaxed((intptr_t*)value);
        Baselib_atomic_thread_fence_acquire();
        if (tmp == nullptr)
        {
            os::FastAutoLock autoLock(lock);
            tmp = (T*)Baselib_atomic_load_ptr_relaxed((intptr_t*)value);
            if (tmp == nullptr)
            {
                tmp = init(autoLock);
                Baselib_atomic_thread_fence_release();
                Baselib_atomic_store_ptr_relaxed((intptr_t*)value, (intptr_t)tmp);
            }
        }

        return tmp;
    }
}
}
