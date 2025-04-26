#pragma once

#include "../Baselib_Atomic_TypeSafe.h"
#include "../Baselib_Cpu.h"

static const uint32_t kMonitorSpinIncrement = 64;
static const uint32_t kMonitorSpinWaitMultiplier = 32;

// Spin while address holds expectedValue.
// This loop does not make use of any monitoring facilities but relies solely
// on spin loop relaxation.
//
// \param expectedValue will be updated with the value denoted by address.
// \param targetSpinCount will be updated with the number of remaining spin counts.
//
// \return true if address is detected to hold a different value from expectedValue.
COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Detail_Baselib_SpinLoop_NoMonitor(const int32_t* address, int32_t* expectedValue, uint32_t* targetSpinCount)
{
    const int32_t expectation = *expectedValue;
    while (*targetSpinCount)
    {
        *targetSpinCount -= 1;
        Baselib_Cpu_Hint_SpinLoop();
        if (expectation != (*expectedValue = Baselib_atomic_load_32_relaxed(address)))
            return true;
    }
    return false;
}

// Spin while address holds expectedValue.
//
// \param expectedValue will be updated with the value denoted by address.
// \param targetSpinCount will be updated with the number of remaining spin counts.
//
// \return true if address is detected to hold a different value from expectedValue.
COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Detail_Baselib_SpinLoop(const int32_t* address, int32_t* expectedValue, uint32_t* targetSpinCount)
{
    const Baselib_Cpu_Monitor monitor = Baselib_Cpu_Hint_MonitorInit();
    if (monitor.supported)
    {
        const int32_t expectation = *expectedValue;
        while (*targetSpinCount)
        {
            // Instead of doing one long wait we do many small ones to account for spurious wakeups etc
            const uint32_t spinIncrement = *targetSpinCount < kMonitorSpinIncrement ? *targetSpinCount : kMonitorSpinIncrement;
            *targetSpinCount -= spinIncrement;

            if (Baselib_Cpu_Hint_MonitorAddress(monitor, address))
            {
                if (expectation != (*expectedValue = Baselib_atomic_load_32_relaxed(address)))
                    return true;
            }
            Baselib_Cpu_Hint_MonitorWait(monitor, spinIncrement * kMonitorSpinWaitMultiplier);
            if (expectation != (*expectedValue = Baselib_atomic_load_32_relaxed(address)))
                return true;
        }
        return false;
    }
    return Detail_Baselib_SpinLoop_NoMonitor(address, expectedValue, targetSpinCount);
}

// Spin while address holds expectedValue.
// This loop does not make use of any monitoring facilities but relies solely
// on spin loop relaxation.
//
// \param expectedValue will be updated with the value denoted by address.
// \param targetSpinCount will be updated with the number of remaining spin counts.
//
// \return true if address is detected to hold a different value from expectedValue.
COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Detail_Baselib_SpinLoop64_NoMonitor(const int64_t* address, int64_t* expectedValue, uint32_t* targetSpinCount)
{
    const int64_t expectation = *expectedValue;
    while (*targetSpinCount)
    {
        *targetSpinCount -= 1;
        Baselib_Cpu_Hint_SpinLoop();
        if (expectation != (*expectedValue = Baselib_atomic_load_64_relaxed(address)))
            return true;
    }
    return false;
}

// Spin while address holds expectedValue.
//
// \param expectedValue will be updated with the value denoted by address.
// \param targetSpinCount will be updated with the number of remaining spin counts.
//
// \return true if address is detected to hold a different value from expectedValue.
COMPILER_WARN_UNUSED_RESULT
BASELIB_INLINE_API bool Detail_Baselib_SpinLoop64(const int64_t* address, int64_t* expectedValue, uint32_t* targetSpinCount)
{
    // Instead of doing one long wait we do many small ones to account for spurious wakeups
    const Baselib_Cpu_Monitor monitor = Baselib_Cpu_Hint_MonitorInit();
    if (monitor.supported)
    {
        const int64_t expectation = *expectedValue;
        while (*targetSpinCount)
        {
            const uint32_t spinIncrement = *targetSpinCount < kMonitorSpinIncrement ? *targetSpinCount : kMonitorSpinIncrement;
            *targetSpinCount -= spinIncrement;

            if (Baselib_Cpu_Hint_MonitorAddress(monitor, address))
            {
                if (expectation != (*expectedValue = Baselib_atomic_load_64_relaxed(address)))
                    return true;
            }
            Baselib_Cpu_Hint_MonitorWait(monitor, spinIncrement * kMonitorSpinWaitMultiplier);
            if (expectation != (*expectedValue = Baselib_atomic_load_64_relaxed(address)))
                return true;
        }
        return false;
    }
    return Detail_Baselib_SpinLoop64_NoMonitor(address, expectedValue, targetSpinCount);
}
