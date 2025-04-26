// Make sure arch is supported
#if !(defined(__i386__)     \
    || defined(__x86_64__)  \
    || defined(__wasm__)    \
    || defined(__aarch64__) \
    || (defined(__ARM_ARCH) && __ARM_ARCH >= 7))
    #error "Unsupported CPU Architecture"
#endif

#if defined(__x86_64__) && !(COMPILER_GCC && (__GNUC__ < 6))
    #define DETAIL_BASELIB_MWAITX_CAPABLE
#endif

static FORCE_INLINE bool Baselib_Cpu_Hint_SpinLoop(void)
{
// https://en.wikipedia.org/w/index.php?title=X86_instruction_listings&oldid=1131392252#cite_ref-43
#if defined(__i386__) || defined(__x86_64__)
    asm volatile ("pause");
    return true;

// https://developer.arm.com/documentation/dui0489/i/arm-and-thumb-instructions/wfe
#elif defined(__aarch64__)
    asm volatile ("sevl; wfe");
    return true;

// https://github.com/rust-lang/rust/commit/c064b6560b7ce0adeb9bbf5d7dcf12b1acb0c807
#elif defined(__ARM_ARCH) && __ARM_ARCH >= 7
    asm volatile ("isb sy");
    return true;

// https://github.com/WebAssembly/threads/issues/15
#elif defined(__wasm__)
    return false;

#else
    #error Unknown spin loop relaxation instruction for CPU

#endif
}

static FORCE_INLINE Baselib_Cpu_Monitor Baselib_Cpu_Hint_MonitorInit(void)
{
#ifdef DETAIL_BASELIB_MWAITX_CAPABLE
    Baselib_Cpu_Monitor monitor = {Baselib_Cpu_HasFeature(Baselib_Cpu_Feature_MWaitX)};
#else
    Baselib_Cpu_Monitor monitor = {false};
#endif
    return monitor;
}

static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorAddress(const Baselib_Cpu_Monitor monitor, const void* address)
{
#ifdef DETAIL_BASELIB_MWAITX_CAPABLE
    if (monitor.supported)
    {
        asm volatile ("monitorx" : : "a" (address), "b" (0), "c" (0));
        return true;
    }
#endif

    return false;
}

static FORCE_INLINE bool Baselib_Cpu_Hint_MonitorWait(const Baselib_Cpu_Monitor monitor, const uint32_t timeout)
{
#ifdef DETAIL_BASELIB_MWAITX_CAPABLE
    if (monitor.supported)
    {
        asm volatile ("mwaitx" : : "a" (0), "b" (timeout), "c" (0x02));
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
