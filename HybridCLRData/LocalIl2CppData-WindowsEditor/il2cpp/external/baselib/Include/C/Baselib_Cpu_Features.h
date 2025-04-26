#pragma once

#ifdef __cplusplus
BASELIB_C_INTERFACE
{
#endif

typedef enum
{
    Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000000,
    Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000001,
    Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000007,
    Baselib_Cpu_FeatureData_Scheme_CpuId_0x80000001,
    Detail_Baselib_Cpu_FeatureData_CpuIdArrayLength = Baselib_Cpu_FeatureData_Scheme_CpuId_0x80000001 + 1,
} Baselib_Cpu_FeatureData_Scheme;

typedef enum
{
    Baselib_Cpu_FeatureData_Register_AX = 0,
    Baselib_Cpu_FeatureData_Register_BX = 1,
    Baselib_Cpu_FeatureData_Register_CX = 2,
    Baselib_Cpu_FeatureData_Register_DX = 3
} Baselib_Cpu_FeatureData_Register;

// Feature struct
//
// Specifies which feature to check for using what scheme
//
typedef struct
{
    Baselib_Cpu_FeatureData_Scheme      scheme;
    Baselib_Cpu_FeatureData_Register    reg;
    int8_t                              bit;
} Baselib_Cpu_Feature;

//
// https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
//

static const Baselib_Cpu_Feature Baselib_Cpu_Feature_SSE2       = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000001, Baselib_Cpu_FeatureData_Register_DX, 26 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_SSE3       = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000001, Baselib_Cpu_FeatureData_Register_CX,  0 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_SSSE3      = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000001, Baselib_Cpu_FeatureData_Register_CX,  9 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_SSE41      = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000001, Baselib_Cpu_FeatureData_Register_CX, 19 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_SSE42      = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000001, Baselib_Cpu_FeatureData_Register_CX, 20 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_AVX        = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000001, Baselib_Cpu_FeatureData_Register_CX, 28 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_AVX2       = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000007, Baselib_Cpu_FeatureData_Register_BX,  5 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_AVX512F    = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x00000007, Baselib_Cpu_FeatureData_Register_BX, 16 };
static const Baselib_Cpu_Feature Baselib_Cpu_Feature_MWaitX     = { Baselib_Cpu_FeatureData_Scheme_CpuId_0x80000001, Baselib_Cpu_FeatureData_Register_CX, 29 };

typedef struct { int32_t regs[4]; } Detail_Baselib_Cpu_FeatureData_CpuId;
extern BASELIB_API const Detail_Baselib_Cpu_FeatureData_CpuId Detail_Baselib_Cpu_FeatureData_CpuIdArray[Detail_Baselib_Cpu_FeatureData_CpuIdArrayLength];

// Check if the CPU supports a certain feature or instruction.
//
// At the moment this function is limited to checking features using CPUID on x86 based platforms.
//
/// \return true if support for the feature was detected.
static FORCE_INLINE bool Baselib_Cpu_HasFeature(const Baselib_Cpu_Feature feature)
{
    return (Detail_Baselib_Cpu_FeatureData_CpuIdArray[feature.scheme].regs[feature.reg] >> feature.bit) & 1;
}

#ifdef __cplusplus
}
#endif
