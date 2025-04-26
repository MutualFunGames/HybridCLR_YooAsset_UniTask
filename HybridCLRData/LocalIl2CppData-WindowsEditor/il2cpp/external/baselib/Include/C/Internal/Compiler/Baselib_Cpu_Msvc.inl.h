#include <intrin.h>

// Make sure arch is supported
#if !(defined(_M_IX86)      \
    || defined(_M_X64)      \
    || defined(_M_ARM64)    \
    || (defined(_M_ARM) && _M_ARM >= 7))
    #error "Unsupported CPU Architecture"
#endif

static FORCE_INLINE bool Baselib_Cpu_Hint_SpinLoop(void)
{
// https://en.wikipedia.org/w/index.php?title=X86_instruction_listings&oldid=1131392252#cite_ref-43
#if defined(_M_IX86) || defined(_M_X64)
    _mm_pause();
    return true;

// "sevl" intrinsic is currently not available for msvc. Enable when/if microsoft decide to include it.
// https://developer.arm.com/documentation/dui0489/i/arm-and-thumb-instructions/wfe
// #elif defined(_M_ARM64)
//     __sevl();
//     __wfe();
//     return true;

// https://github.com/rust-lang/rust/commit/c064b6560b7ce0adeb9bbf5d7dcf12b1acb0c807
#elif defined(_M_ARM64) || defined(_M_ARM) && _M_ARM >= 7
    __isb(0x0F /* ARM_BARRIER_SY */);
    return true;

#else
    #error Unknown spin loop relaxation instruction for CPU

#endif
}

static FORCE_INLINE Baselib_Cpu_Monitor Baselib_Cpu_Hint_MonitorInit(void)
{
#if defined(_M_X64)
    Baselib_Cpu_Monitor monitor = {Baselib_Cpu_HasFeature(Baselib_Cpu_Feature_MWaitX)};
#else
    Baselib_Cpu_Monitor monitor = {false};
#endif
    return monitor;
}

static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorAddress(const Baselib_Cpu_Monitor monitor, const void* address)
{
#if defined(_M_X64)
    if (monitor.supported)
    {
        _mm_monitorx(address, 0, 0);
        return true;
    }
#endif

    return false;
}

static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorWait(const Baselib_Cpu_Monitor monitor, const uint32_t timeout)
{
#if defined(_M_X64)
    if (monitor.supported)
    {
        _mm_mwaitx(0x02, 0, timeout);
        return true;
    }
#endif

    Baselib_Cpu_Hint_SpinLoop();
    return false;
}

static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorRelease(void)
{
    return false;
}
