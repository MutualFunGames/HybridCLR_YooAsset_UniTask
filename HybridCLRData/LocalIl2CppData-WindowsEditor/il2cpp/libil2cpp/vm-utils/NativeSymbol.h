#pragma once

#include "vm/MetadataCache.h"
#include "vm/StackTrace.h"
#include "vm-utils/MethodDefinitionKey.h"
#include "vm-utils/VmMethod.h"

#include <stdint.h>
#include <vector>

namespace il2cpp
{
namespace utils
{
    class NativeSymbol
    {
    public:
#if IL2CPP_ENABLE_NATIVE_STACKTRACES
        static void RegisterMethods(const std::vector<MethodDefinitionKey>& managedMethods);
        static const VmMethod* GetMethodFromNativeSymbol(Il2CppMethodPointer nativeMethod);
        static void GetAllManagedMethodsWithDebugInfo(void(*func)(const MethodInfo* method, Il2CppMethodDebugInfo* methodDebugInfo, void* userData), void* userData);
        static bool GetMethodDebugInfo(const MethodInfo* method, Il2CppMethodDebugInfo* methodDebugInfo);
#endif
    };
} /* namespace vm */
} /* namespace mono */
