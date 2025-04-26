#pragma once

#include "../../../C/Baselib_Atomic.h"
#include "../../../C/Baselib_Atomic_Macros.h"
#include "Baselib_Atomic_Gcc_Patch.h"

#if COMPILER_GCC && ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#pragma message "GNUC: " PP_STRINGIZE(__GNUC__) " GNUC_MINOR: " PP_STRINGIZE(__GNUC_MINOR__)
#error "GCC is too old and/or missing compatible atomic built-in functions" PP_STRINGIZE(__GNUC__)
#endif

#define detail_intrinsic_relaxed __ATOMIC_RELAXED
#define detail_intrinsic_acquire __ATOMIC_ACQUIRE
#define detail_intrinsic_release __ATOMIC_RELEASE
#define detail_intrinsic_acq_rel __ATOMIC_ACQ_REL
// IMPORTANT !!!
// There has been some confusion about SEQ_CST memory order.
// It is NOT equal to having a full memory barrier in place.
// In fact, according to the specification, the sequential consistency is ONLY guaranteed
//   when used only together with other SEQ_CST calls.
// When SEQ_CST is intermixed with other memory order calls, then the sequential consistency is NOT guaranteed.
// Some compilers (mis)use this spec issue as they typically do with a UB,
//   by "downgrading" a SEQ_CST operation to a ACQ_REL.
// This is still compliant with the spec because
// - the sequential consistency is maintained between these "downgraded" ACQ_REL operations
// - if intermixed with other memory orders, consistency is NOT guaranteed, as per spec.
// SEQ_CST is actually broken from the design point of view.
//
// It can be checked in godbolt too - create two functions with ACQ_REL and SEQ_CST
// At the time of writing (December 2023):
// - for Arm64 on GCC and clang, the assembly is identical
// - MSVC for Arm64 adds a barrier
// - for Armv7, a barrier is added
// - on Intel, the instructions for ACQ_REL and SEQ_CST are different.
//
// If you want your SEQ_CST operation to have a full memory barrier
// (usually where you'd normally have a call to a __sync_* builtin),
// you should:
// - make sure you are confident in what you're doing
// - add a call to Baselib_atomic_thread_fence_seq_cst()
//
// Links to explore:
// - https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
// - https://arxiv.org/pdf/1611.01507.pdf
// - https://plv.mpi-sws.org/scfix/paper.pdf
// - https://community.arm.com/arm-community-blogs/b/tools-software-ides-blog/posts/armv8-sequential-consistency
#define detail_intrinsic_seq_cst __ATOMIC_SEQ_CST

// The GCC included with QNX considers a fail order equal or stronger than success order an invalid memory model for __atomic_compare_exchange,
// which otherwise is allowed by the c++ standard.
// We amend this by forcing the failure order to acquire under such circumstance.
#if defined(__QNX__)
#define detail_QNX_CMP_XCHG_BARRIER_ORDER2_PATCH(order1, order2) order2 < order1 ? order2 : order2 == detail_intrinsic_relaxed ? order2 : detail_intrinsic_acquire
#else
#define detail_QNX_CMP_XCHG_BARRIER_ORDER2_PATCH(order1, order2) order2
#endif

#define detail_THREAD_FENCE(order, ...)                                                                                     \
static FORCE_INLINE void Baselib_atomic_thread_fence_##order(void)                                                          \
{                                                                                                                           \
    __extension__({__atomic_thread_fence (detail_intrinsic_##order); });                                                    \
}                                                                                                                           \

#define detail_LOAD(op, order, id , bits, int_type, ...)                                                                    \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(const void* obj, void* result)                            \
{                                                                                                                           \
    typedef int_type aligned_int_type COMPILER_ALIGN_AS(sizeof(int_type));                                                  \
    __extension__({ __atomic_load((aligned_int_type*)obj, (int_type*)result, detail_intrinsic_##order); });                 \
}

#define detail_LOAD_NOT_CONST(op, order, id , bits, int_type, ...)                                                          \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(void* obj, void* result)                                  \
{                                                                                                                           \
    typedef int_type aligned_int_type COMPILER_ALIGN_AS(sizeof(int_type));                                                  \
    __extension__({ __atomic_load((aligned_int_type*)obj, (int_type*)result, detail_intrinsic_##order); });                 \
}

#define detail_STORE(op, order, id , bits, int_type, ...)                                                                   \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(void* obj, const void* value)                             \
{                                                                                                                           \
    typedef int_type aligned_int_type COMPILER_ALIGN_AS(sizeof(int_type));                                                  \
    __extension__({ __atomic_store((aligned_int_type*)obj, (int_type*)value, detail_intrinsic_##order); });                 \
}

#define detail_ALU(op, order, id , bits, int_type, ...)                                                                     \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(void* obj, const void* value, void* result)               \
{                                                                                                                           \
    typedef int_type aligned_int_type COMPILER_ALIGN_AS(sizeof(int_type));                                                  \
    *(aligned_int_type*)result = __extension__({ __atomic_##op((aligned_int_type*)obj, *(int_type*)value, detail_intrinsic_##order); });\
}

#define detail_XCHG(op, order, id , bits, int_type, ...)                                                                    \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(void* obj, const void* value, void* result)               \
{                                                                                                                           \
    typedef int_type aligned_int_type COMPILER_ALIGN_AS(sizeof(int_type));                                                  \
    __extension__({ __atomic_exchange((aligned_int_type*)obj, (int_type*)value, (int_type*)result, detail_intrinsic_##order); });\
}

#define detail_CMP_XCHG_WEAK(op, order1, order2, id , bits, int_type, ...)                                                  \
static FORCE_INLINE bool Baselib_atomic_##op##_##id##_##order1##_##order2##_v(void* obj, void* expected, const void* value) \
{                                                                                                                           \
    typedef int_type aligned_int_type COMPILER_ALIGN_AS(sizeof(int_type));                                                  \
    detail_GCC_CMP_XCHG_128_WEAK_QNX_PATCH(order1, order2, int_type, obj, expected, value);                                 \
    return __extension__({ __atomic_compare_exchange(                                                                       \
        (aligned_int_type*)obj,                                                                                             \
        (int_type*)expected,                                                                                                \
        (int_type*)value,                                                                                                   \
        1,                                                                                                                  \
        detail_intrinsic_##order1,                                                                                          \
        detail_QNX_CMP_XCHG_BARRIER_ORDER2_PATCH(detail_intrinsic_##order1, detail_intrinsic_##order2));                    \
    });                                                                                                                     \
}

#define detail_CMP_XCHG_STRONG(op, order1, order2, id , bits, int_type, ...)                                                \
static FORCE_INLINE bool Baselib_atomic_##op##_##id##_##order1##_##order2##_v(void* obj, void* expected, const void* value) \
{                                                                                                                           \
    typedef int_type aligned_int_type COMPILER_ALIGN_AS(sizeof(int_type));                                                  \
    detail_GCC_CMP_XCHG_128_STRONG_QNX_PATCH(order1, order2, int_type, obj, expected, value);                               \
    return  __extension__ ({ __atomic_compare_exchange(                                                                     \
        (aligned_int_type*)obj,                                                                                             \
        (int_type*)expected,                                                                                                \
        (int_type*)value,                                                                                                   \
        0,                                                                                                                  \
        detail_intrinsic_##order1,                                                                                          \
        detail_QNX_CMP_XCHG_BARRIER_ORDER2_PATCH(detail_intrinsic_##order1, detail_intrinsic_##order2));                    \
    });                                                                                                                     \
}

#define detail_NOT_SUPPORTED(...)

Baselib_Atomic_FOR_EACH_MEMORY_ORDER(
    detail_THREAD_FENCE
)

Baselib_Atomic_FOR_EACH_ATOMIC_OP_MEMORY_ORDER_AND_TYPE(
    detail_LOAD,            // load
    detail_STORE,           // store
    detail_ALU,             // add
    detail_ALU,             // and
    detail_ALU,             // or
    detail_ALU,             // xor
    detail_XCHG,            // exchange
    detail_CMP_XCHG_WEAK,   // compare_exchange_weak
    detail_CMP_XCHG_STRONG, // compare_exchange_strong
)

#if PLATFORM_ARCH_64

// 128-bit implementation
// GCC 7.0 and higher does not provide __atomic_load, store or xchg 16b, so we fallback to cmpxchg for those atomic ops.
// For QNX we do this for GCC version 5.0 and higher (incorrect versioning?)
#if PLATFORM_REQUIRES_GCC_ATOMIC_CMPXCHG128_PATCH

// QNX GCC < 7.1 erraneously reports uninitialized memory
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"

#define detail_LOAD_NOT_CONST128(op, order, id, ...)                                                            \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(void* obj, void* result)                      \
{                                                                                                               \
    *((__int128*)result) = 0;                                                                                   \
    Baselib_atomic_compare_exchange_weak_128_##order##_##order##_v((void*)obj, result, result);                 \
}

#define detail_STORE128(op, order, id, ...)                                                                     \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(void* obj, const void* value)                 \
{                                                                                                               \
    __int128 comparand = *((volatile __int128*)obj);                                                            \
    while(!Baselib_atomic_compare_exchange_weak_128_##order##_relaxed_v(obj, &comparand, value));               \
}

#define detail_XCHG128(op, order, id, ...)                                                                      \
static FORCE_INLINE void Baselib_atomic_##op##_##id##_##order##_v(void* obj, const void* value, void* result)   \
{                                                                                                               \
    *((__int128*)result) = *((volatile __int128*)obj);                                                          \
    while(!Baselib_atomic_compare_exchange_weak_128_##order##_relaxed_v(obj, result, value));                   \
}

#pragma GCC diagnostic pop

Baselib_Atomic_FOR_EACH_ATOMIC_OP_AND_MEMORY_ORDER(
    detail_LOAD_NOT_CONST128,   // load
    detail_STORE128,            // store
    detail_NOT_SUPPORTED,       // add
    detail_NOT_SUPPORTED,       // and
    detail_NOT_SUPPORTED,       // or
    detail_NOT_SUPPORTED,       // xor
    detail_XCHG128,             // exchange
    detail_CMP_XCHG_WEAK,       // compare_exchange_weak
    detail_CMP_XCHG_STRONG,     // compare_exchange_strong
    128, 128, __int128          // type information
)

Baselib_Atomic_FOR_EACH_ATOMIC_OP_AND_MEMORY_ORDER(
    detail_LOAD_NOT_CONST128,   // load
    detail_STORE128,            // store
    detail_NOT_SUPPORTED,       // add
    detail_NOT_SUPPORTED,       // and
    detail_NOT_SUPPORTED,       // or
    detail_NOT_SUPPORTED,       // xor
    detail_XCHG128,             // exchange
    detail_CMP_XCHG_WEAK,       // compare_exchange_weak
    detail_CMP_XCHG_STRONG,     // compare_exchange_strong
    ptr2x, 128, __int128        // type information
)

#undef detail_LOAD_NOT_CONST128
#undef detail_STORE128
#undef detail_XCHG128

#else

Baselib_Atomic_FOR_EACH_ATOMIC_OP_AND_MEMORY_ORDER(
    detail_LOAD_NOT_CONST,      // load
    detail_STORE,               // store
    detail_NOT_SUPPORTED,       // add
    detail_NOT_SUPPORTED,       // and
    detail_NOT_SUPPORTED,       // or
    detail_NOT_SUPPORTED,       // xor
    detail_XCHG,                // exchange
    detail_CMP_XCHG_WEAK,       // compare_exchange_weak
    detail_CMP_XCHG_STRONG,     // compare_exchange_strong
    128, 128, __int128          // type information
)

Baselib_Atomic_FOR_EACH_ATOMIC_OP_AND_MEMORY_ORDER(
    detail_LOAD_NOT_CONST,      // load
    detail_STORE,               // store
    detail_NOT_SUPPORTED,       // add
    detail_NOT_SUPPORTED,       // and
    detail_NOT_SUPPORTED,       // or
    detail_NOT_SUPPORTED,       // xor
    detail_XCHG,                // exchange
    detail_CMP_XCHG_WEAK,       // compare_exchange_weak
    detail_CMP_XCHG_STRONG,     // compare_exchange_strong
    ptr2x, 128, __int128        // type information
)

#endif // PLATFORM_REQUIRES_GCC_ATOMIC_CMPXCHG128_PATCH

#else

Baselib_Atomic_FOR_EACH_ATOMIC_OP_AND_MEMORY_ORDER(
    detail_LOAD_NOT_CONST,      // load
    detail_STORE,               // store
    detail_NOT_SUPPORTED,       // add
    detail_NOT_SUPPORTED,       // and
    detail_NOT_SUPPORTED,       // or
    detail_NOT_SUPPORTED,       // xor
    detail_XCHG,                // exchange
    detail_CMP_XCHG_WEAK,       // compare_exchange_weak
    detail_CMP_XCHG_STRONG,     // compare_exchange_strong
    ptr2x, 64, int64_t          // type information
)

#endif // PLATFORM_ARCH_64

#undef detail_intrinsic_relaxed
#undef detail_intrinsic_acquire
#undef detail_intrinsic_release
#undef detail_intrinsic_acq_rel
#undef detail_intrinsic_seq_cst

#undef detail_THREAD_FENCE
#undef detail_LOAD
#undef detail_LOAD_NOT_CONST
#undef detail_STORE
#undef detail_ALU
#undef detail_XCHG
#undef detail_CMP_XCHG_WEAK
#undef detail_CMP_XCHG_STRONG
#undef detail_NOT_SUPPORTED

#include "Baselib_Atomic_Gcc_Patch_PostInclude.h"
