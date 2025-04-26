#pragma once

#include "Baselib_Cpu_Features.h"

// Emit machine instruction signaling to the CPU that a spin loop is busy waiting and that an appropriate backoff strategy can be employed.
// The intention is to pause cpu execution for <50ns while also saving power.
// This is different from a thread yield which tells the operating system thread scheduler it's okay to switch to a different thread.
// Because of that it's important to not spin for a long time - normally a finite number of iterations - and instead do a thread yield.
//
// Note: When unsupported this is treated as a no-op and does not fallback to the system scheduler (see Baselib_Thread_YieldExecution).
//
// \returns          true if platform support spin loop relaxation.
static FORCE_INLINE bool Baselib_Cpu_Hint_SpinLoop(void);

// CPU (synchronization) monitor.
// This is a theoretical representation of a CPU monitor.
// Setup varies per CPU but generally the idea is to setup a monitor that can be waited on until an interlocked store or event happens.
typedef struct
{
    const bool supported;
} Baselib_Cpu_Monitor;

// Emit machine instruction preparing the monitor for a loop. This is important to do to not have missed wakeups.
// See:
// https://community.arm.com/support-forums/f/architectures-and-processors-forum/4273/how-to-understand-armv8-sevl-instruction-in-spin-lock
//
// Example pseudo code trying to acquire a lock:
// outer_loop:
// {
//     if (TryToAcquireLock())
//         return;
//
//     Baselib_Cpu_Hint_MonitorInit();
//     inner_loop:
//     {
//         if (Baselib_Cpu_Hint_MonitorAddress(address))
//         {
//             // Value denoted by address may have changed before we established the monitor
//             // So we need to do a check here before calling wait
//             if (atomic_load(*address) == UNLOCKED)
//                 goto outer_loop;
//         }
//         Baselib_Cpu_Hint_MonitorWait();
//         if (atomic_load(*address) == UNLOCKED)
//             goto outer_loop;
//         goto inner_loop;
//     }
// }
//
// \returns          a monitor struct
static FORCE_INLINE Baselib_Cpu_Monitor Baselib_Cpu_Hint_MonitorInit(void);

// Emit machine instruction establishing a monitor for the given address. The
// monitor can be released by any number of events, but most importantly it
// will be released by a store to the monitored address.
//
// After a successful monitor has been setup for an address it's important to
// check the value of address to make sure there wasn't a race, i.e. value
// changed before or while setting up the monitor.
//
// It's also important to do this because it has been observed that without a
// a load the wait may exit prematurely.
//
// See:
// https://www.amd.com/system/files/TechDocs/24594.pdf : MONITORX and MWAITX
//
// \returns          true if monitoring an and address for store is supported by the platform.
static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorAddress(const Baselib_Cpu_Monitor monitor, const void* address);

// Emit machine instruction waiting on an active monitor or a monitor event.
// If unsupported this will fallback to `Baselib_Cpu_Hint_SpinLoop`.
//
// Timeout is in clock cycles. A value of zero disables the timeout.
// See:
// https://www.amd.com/system/files/TechDocs/24594.pdf : MONITORX and MWAITX
// https://developer.arm.com/documentation/ddi0596/2020-12/Base-Instructions/WFET--Wait-For-Event-with-Timeout-
//
// \returns          false if monitor is not supported and the code fallbacks to Baselib_Cpu_Hint_SpinLoop
static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorWait(const Baselib_Cpu_Monitor monitor, const uint32_t timeout);

// Release monitor
//
// This function is a no-op where monitor is not supported or where the monitor
// is guaranteed to be released by a store to memory.
//
// See:
// https://developer.arm.com/documentation/dui0473/m/arm-and-thumb-instructions/sev
//
// \returns          true if signaling the monitor is supported and needed.
static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorRelease(void);

#if COMPILER_CLANG || COMPILER_GCC
    #include "Internal/Compiler/Baselib_Cpu_Gcc.inl.h"
#elif COMPILER_MSVC
    #include "Internal/Compiler/Baselib_Cpu_Msvc.inl.h"
#endif
