#pragma once

#if PLATFORM_REQUIRES_GCC_ATOMIC_CMPXCHG128_PATCH

// Patch for GCC (version 7.0 and later). 128-bit__atomic_compare_exchange intrinsic producing code calling libatomic
// __atomic_compare_exchange_16. Libatomic is not used by baselib due to known performance issues and we instead use inline assembly.
// Ref: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878

#if defined(__aarch64__)
#define detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ld_instr, st_instr, barrier_instr) \
{                                                                                             \
    register bool result asm ("w0");                                        \
    asm volatile                                                            \
    (                                                                       \
        "   ldp     x12, x13, [%x4]         /* load expected */         \n" \
        "   ldp     x10, x11, [%x5]         /* load value */            \n" \
        "   " #ld_instr "  x9, x8, [%x3]    /* load obj */              \n" \
        "   eor     x13, x8, x13            /* compare to expected */   \n" \
        "   eor     x12, x9, x12                                        \n" \
        "   orr     x12, x12, x13                                       \n" \
        "   cbnz    x12, 0f                 /* not equal = no store */  \n" \
        "   " #st_instr "   w12, x10, x11, [%x0] /* try store */        \n" \
        "   cbnz    w12, 1f                                             \n" \
        "   orr w0, wzr, #0x1               /* success, result in w0 */ \n" \
        "   b   2f                                                      \n" \
        "0:                                 /* no store */              \n" \
        "   clrex                                                       \n" \
        "1:                                 /* failed store */          \n" \
        "   movz    w0, #0                                              \n" \
        "2:                                 /* store expected, fail */  \n" \
        "   tbnz    w0, #0, 3f                                          \n" \
        "   stp     x9, x8, [%x1]                                       \n" \
        "3:                                                             \n" \
        "   " #barrier_instr "                                          \n" \
                                                                            \
        : "+r" (obj), "+r" (expected), "=r" (result)                        \
        : "r" (obj), "r" (expected), "r" (value)                            \
        : "x8", "x9", "x10", "x11", "x12", "x13", "cc", "memory");          \
                                                                            \
    return result != 0;                                                     \
}

#define detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ld_instr, st_instr, barrier_instr)   \
{                                                                                                 \
    register bool result asm ("w0");                                                \
    asm volatile                                                                    \
    (                                                                               \
        "   ldp     x10, x11, [%x4]       /* load expected */                   \n" \
        "   ldp     x12, x13, [%x5]       /* load value */                      \n" \
        "0:                                                                     \n" \
        "   " #ld_instr "  x9, x8, [%x3]  /* load obj (ldxp/ldaxp) */           \n" \
        "   eor     x14, x8, x11          /* compare to expected */             \n" \
        "   eor     x15, x9, x10                                                \n" \
        "   orr     x14, x15, x14                                               \n" \
        "   cbnz    x14, 1f               /* not equal = no store */            \n" \
        "   " #st_instr "   w14, x12, x13, [%x0] /* try store (stxp/stlxp) */   \n" \
        "   cbnz    w14, 0b               /* retry or store result in w0 */     \n" \
        "   orr w0, wzr, #0x1                                                   \n" \
        "   b   2f                                                              \n" \
        "1:                               /* no store */                        \n" \
        "   movz    w0, #0                                                      \n" \
        "   clrex                                                               \n" \
        "2:                               /* store expected on fail */          \n" \
        "   tbnz    w0, #0, 3f                                                  \n" \
        "   stp     x9, x8, [%x1]                                               \n" \
        "3:                                                                     \n" \
        "   " #barrier_instr "                                                  \n" \
                                                                                    \
        : "+r" (obj), "+r" (expected), "=r" (result)                                \
        : "r" (obj), "r" (expected), "r" (value)                                    \
        : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "cc", "memory");    \
                                                                                    \
    return result != 0;                                                             \
}

#else // (__aarch64__)

#define detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ld_instr, st_instr, barrier_instr)   \
{                                                                                                                           \
    bool result = false;                                                                                                    \
    asm volatile (                                                                                                          \
                  "    lock cmpxchg16b %1 \n"                                                                               \
                  "    setz %0 \n"                                                                                          \
                  : "=q" (result), "+m" (*(__int128*)obj), "+d" (((uint64_t*)expected)[1]), "+a" (((uint64_t*)expected)[0]) \
                  : "c" (((uint64_t*)value)[1]), "b" (((uint64_t*)value)[0])                                                \
                  : "memory", "cc" );                                                                                       \
    return result;                                                                                                          \
}

#define detail_GCC_CMP_XCHG_WEAK_128 detail_GCC_CMP_XCHG_STRONG_128

#endif

#define detail_GCC_CMP_XCHG_STRONG_128_relaxed_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldxp,  stxp, )
#define detail_GCC_CMP_XCHG_STRONG_128_acquire_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldaxp, stxp, )
#define detail_GCC_CMP_XCHG_STRONG_128_acquire_acquire(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldaxp, stxp, )
#define detail_GCC_CMP_XCHG_STRONG_128_release_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldxp,  stlxp, )
#define detail_GCC_CMP_XCHG_STRONG_128_acq_rel_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldaxp, stlxp, )
#define detail_GCC_CMP_XCHG_STRONG_128_acq_rel_acquire(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldaxp, stlxp, )
#define detail_GCC_CMP_XCHG_STRONG_128_seq_cst_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldaxp, stlxp, dmb ish)
#define detail_GCC_CMP_XCHG_STRONG_128_seq_cst_acquire(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldaxp, stlxp, dmb ish)
#define detail_GCC_CMP_XCHG_STRONG_128_seq_cst_seq_cst(obj, expected, value) detail_GCC_CMP_XCHG_STRONG_128(obj, expected, value, ldaxp, stlxp, dmb ish)

#define detail_GCC_CMP_XCHG_WEAK_128_relaxed_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldxp,  stxp, )
#define detail_GCC_CMP_XCHG_WEAK_128_acquire_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldaxp, stxp, )
#define detail_GCC_CMP_XCHG_WEAK_128_acquire_acquire(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldaxp, stxp, )
#define detail_GCC_CMP_XCHG_WEAK_128_release_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldxp,  stlxp, )
#define detail_GCC_CMP_XCHG_WEAK_128_acq_rel_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldaxp, stlxp, )
#define detail_GCC_CMP_XCHG_WEAK_128_acq_rel_acquire(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldaxp, stlxp, )
#define detail_GCC_CMP_XCHG_WEAK_128_seq_cst_relaxed(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldaxp, stlxp, dmb ish)
#define detail_GCC_CMP_XCHG_WEAK_128_seq_cst_acquire(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldaxp, stlxp, dmb ish)
#define detail_GCC_CMP_XCHG_WEAK_128_seq_cst_seq_cst(obj, expected, value) detail_GCC_CMP_XCHG_WEAK_128(obj, expected, value, ldaxp, stlxp, dmb ish)

#define detail_GCC_CMP_XCHG_128_WEAK_QNX_PATCH(order1, order2, int_type, obj, expected, value) \
    if(sizeof(int_type) == 16) \
        detail_GCC_CMP_XCHG_WEAK_128_##order1##_##order2(obj, expected, value);

#define detail_GCC_CMP_XCHG_128_STRONG_QNX_PATCH(order1, order2, int_type, obj, expected, value) \
    if(sizeof(int_type) == 16) \
        detail_GCC_CMP_XCHG_STRONG_128_##order1##_##order2(obj, expected, value);

#else // PLATFORM_USE_QNX_ATOMIC_CMPXCHG_128_PATCH

#define detail_GCC_CMP_XCHG_128_WEAK_QNX_PATCH(...)
#define detail_GCC_CMP_XCHG_128_STRONG_QNX_PATCH(...)

#endif
