#pragma once

#include "Time.h"

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        // Object that will on destruction call Release() on the sync primitive referenced by syncPrimitivePtr.
        // Initializing ScopedRelease with a nullptr signals Acquire failed.
        //
        template<typename T>
        class ScopedRelease
        {
        public:
            FORCE_INLINE ScopedRelease(T* syncPrimitivePtr) : m_SyncPrimitivePtr(syncPrimitivePtr) {}
            FORCE_INLINE ~ScopedRelease() { if (m_SyncPrimitivePtr) m_SyncPrimitivePtr->Release(); }

            // non-copyable
            ScopedRelease(const ScopedRelease& other) = delete;
            ScopedRelease& operator=(const ScopedRelease& other) = delete;

            // move-constructable, but not assignable as it doesn't make much sense to swap locks.
            ScopedRelease(ScopedRelease&& other)
            {
                this->m_SyncPrimitivePtr = other.m_SyncPrimitivePtr;
                other.m_SyncPrimitivePtr = nullptr;
            }

            ScopedRelease& operator=(ScopedRelease&& other) = delete;

            // Returns false if either this object was created from a failed TryAcquireScoped/TryTimedAcquireScoped or ownership was moved.
            bool WasSuccessfullyAcquired() const { return m_SyncPrimitivePtr != nullptr; }

            // Returns false if either this object was created from a failed TryAcquireScoped/TryTimedAcquireScoped or ownership was moved.
            operator bool() const { return WasSuccessfullyAcquired(); }

        private:
            T* m_SyncPrimitivePtr;
        };

        // Call Acquire and return an object that calls Release on its destruction.
        //
        // Example usage:
        //  {
        //      auto scope = AcquireScoped(lock);
        //      enteredCriticalSection++;
        //  }
        template<class T>
        COMPILER_WARN_UNUSED_RESULT
        FORCE_INLINE ScopedRelease<T> AcquireScoped(T& synchronizationObject, const uint32_t maxSpinCount = 0)
        {
            synchronizationObject.Acquire(maxSpinCount);
            return ScopedRelease<T>(&synchronizationObject);
        }

        // Call TryAcquire and return an object that calls Release on its destruction.
        // The returned object will be flagged unsuccessful and not call Release if TryAcquire failed.
        //
        // Example usage:
        //  {
        //      auto result = TryAcquireScoped(lock);
        //      if (result)
        //      {
        //          enteredCriticalSection++;
        //      ]
        //  }
        template<class T>
        COMPILER_WARN_UNUSED_RESULT
        FORCE_INLINE ScopedRelease<T> TryAcquireScoped(T& synchronizationObject, const uint32_t maxSpinCount = 0)
        {
            return ScopedRelease<T>(synchronizationObject.TryAcquire(maxSpinCount) ? &synchronizationObject : nullptr);
        }

        // Call TryTimedAcquire and return an object that calls Release on its destruction.
        // The returned object will be flagged unsuccessful and not call Release if TryTimedAcquire failed.
        //
        // Example usage:
        //  {
        //      auto result = TryTimedAcquireScoped(lock, std::chrono::minutes(1));
        //      if (result)
        //      {
        //          enteredCriticalSection++;
        //      }
        //      assert(result);
        //  }
        template<class T>
        COMPILER_WARN_UNUSED_RESULT
        FORCE_INLINE ScopedRelease<T> TryTimedAcquireScoped(T& synchronizationObject, const timeout_ms timeoutInMilliseconds, const uint32_t maxSpinCount = 0)
        {
            return ScopedRelease<T>(synchronizationObject.TryTimedAcquire(timeoutInMilliseconds, maxSpinCount) ? &synchronizationObject : nullptr);
        }

        // Call Acquire, invoke user defined function then call Release.
        //
        // Example usage:
        //  AcquireScoped(lock, [] {
        //      enteredCriticalSection++;
        //  });
        template<class T, class FunctionType>
        FORCE_INLINE void AcquireScoped(T& synchronizationObject, const FunctionType& func, const uint32_t maxSpinCount = 0)
        {
            const auto scopedRelease = baselib::AcquireScoped(synchronizationObject, maxSpinCount);
            func();
        }

        // Call TryAcquire, if successful invoke user defined function then call Release.
        //
        // Example usage:
        //  bool lockAcquired = TryAcquireScoped(lock, [] {
        //      enteredCriticalSection++;
        //  });
        //
        // Return:  true if acquired and user function was invoked.
        template<class T, class FunctionType>
        FORCE_INLINE bool TryAcquireScoped(T& synchronizationObject, const FunctionType& func, const uint32_t maxSpinCount = 0)
        {
            const auto scopedRelease = baselib::TryAcquireScoped(synchronizationObject, maxSpinCount);
            if (scopedRelease)
            {
                func();
                return true;
            }
            return false;
        }

        // Call TryTimedAcquire, if successful invoke user defined function then call Release.
        //
        // Example usage:
        //  bool lockAcquired = lock.TryTimedAcquireScoped(std::chrono::minutes(1), [] {
        //      enteredCriticalSection++;
        //  });
        //  assert(lockAcquired);
        //
        // Return:  true if acquired and user function was invoked.
        template<class T, class FunctionType>
        FORCE_INLINE bool TryTimedAcquireScoped(T& synchronizationObject, const timeout_ms timeoutInMilliseconds, const FunctionType& func, const uint32_t maxSpinCount = 0)
        {
            const auto scopedRelease = baselib::TryTimedAcquireScoped(synchronizationObject, timeoutInMilliseconds, maxSpinCount);
            if (scopedRelease)
            {
                func();
                return true;
            }
            return false;
        }
    }
}
