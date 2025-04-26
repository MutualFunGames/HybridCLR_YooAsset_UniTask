#pragma once

#include "../Time.h"
#include "../ScopedAcquire.h"

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        namespace detail
        {
            template<typename T>
            class ScopedAcquireMixin
            {
            public:
                // Call Acquire and return an object that calls Release on its destruction.
                //
                // Example usage:
                //  {
                //      auto scope = lock.AcquireScoped();
                //      enteredCriticalSection++;
                //  }
                COMPILER_WARN_UNUSED_RESULT
                FORCE_INLINE ScopedRelease<T> AcquireScoped(const uint32_t maxSpinCount = 0)
                {
                    return baselib::AcquireScoped(SynchronizationObject(), maxSpinCount);
                }

                // Call Acquire, invoke user defined function then call Release.
                //
                // Example usage:
                //  lock.AcquireScoped([] {
                //      enteredCriticalSection++;
                //  });
                template<class FunctionType>
                FORCE_INLINE void AcquireScoped(const FunctionType& func, const uint32_t maxSpinCount = 0)
                {
                    return baselib::AcquireScoped(SynchronizationObject(),  func, maxSpinCount);
                }

                // Call TryAcquire and return an object that calls Release on its destruction.
                // The returned object will be flagged unsuccessful and not call Release if TryAcquire failed.
                //
                // Example usage:
                //  {
                //      auto result = lock.TryAcquireScoped();
                //      if (result)
                //      {
                //          enteredCriticalSection++;
                //      ]
                //  }
                COMPILER_WARN_UNUSED_RESULT
                FORCE_INLINE ScopedRelease<T> TryAcquireScoped(const uint32_t maxSpinCount = 0)
                {
                    return baselib::TryAcquireScoped(SynchronizationObject(), maxSpinCount);
                }

                // Call TryAcquire, if successful invoke user defined function then call Release.
                //
                // Example usage:
                //  bool lockAcquired = lock.TryAcquireScoped([] {
                //      enteredCriticalSection++;
                //  });
                //
                // Return:  true if acquired and user function was invoked.
                template<class FunctionType>
                FORCE_INLINE bool TryAcquireScoped(const FunctionType& func, const uint32_t maxSpinCount = 0)
                {
                    return baselib::TryAcquireScoped(SynchronizationObject(), func, maxSpinCount);
                }

                // Call TryTimedAcquire and return an object that calls Release on its destruction.
                // The returned object will be flagged unsuccessful and not call Release if TryTimedAcquire failed.
                //
                // Example usage:
                //  {
                //      auto result = lock.TryTimedAcquireScoped(std::chrono::minutes(1));
                //      if (result)
                //      {
                //          enteredCriticalSection++;
                //      }
                //      assert(result);
                //  }
                COMPILER_WARN_UNUSED_RESULT
                FORCE_INLINE ScopedRelease<T> TryTimedAcquireScoped(const timeout_ms timeoutInMilliseconds, const uint32_t maxSpinCount = 0)
                {
                    return baselib::TryTimedAcquireScoped(SynchronizationObject(), timeoutInMilliseconds, maxSpinCount);
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
                template<class FunctionType>
                FORCE_INLINE bool TryTimedAcquireScoped(const timeout_ms timeoutInMilliseconds, const FunctionType& func, const uint32_t maxSpinCount = 0)
                {
                    return baselib::TryTimedAcquireScoped(SynchronizationObject(), timeoutInMilliseconds, func, maxSpinCount);
                }

            private:
                constexpr T& SynchronizationObject() const { return *(T*)this; }
            };
        }
    }
}
