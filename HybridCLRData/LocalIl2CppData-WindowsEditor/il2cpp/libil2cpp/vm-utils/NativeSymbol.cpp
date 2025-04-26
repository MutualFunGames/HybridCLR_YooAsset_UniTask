#include "il2cpp-config.h"

#include "il2cpp-class-internals.h"
#include "os/Environment.h"
#include "os/File.h"
#include "os/Image.h"
#include "os/Initialize.h"
#include "os/LibraryLoader.h"
#include "os/Locale.h"
#include "os/Path.h"
#include "utils/Collections.h"
#include "utils/PathUtils.h"
#include "utils/MemoryMappedFile.h"
#include "utils/Runtime.h"
#include "vm/Method.h"
#include "vm-utils/Debugger.h"
#include "vm-utils/DebugSymbolReader.h"
#include "vm-utils/MethodDefinitionKey.h"
#include "vm-utils/NativeSymbol.h"
#include <string>
#include <cstdlib>

namespace il2cpp
{
namespace utils
{
#if IL2CPP_ENABLE_NATIVE_STACKTRACES

    static Il2CppMethodPointer MaskSpareBits(const Il2CppMethodPointer method)
    {
        return (Il2CppMethodPointer)((size_t)method & ~IL2CPP_POINTER_SPARE_BITS);
    }

    struct MethodInfoToMethodPointerConverter
    {
        Il2CppMethodPointer operator()(const MethodDefinitionKey& methodInfo) const
        {
            return MaskSpareBits(methodInfo.method);
        }
    };

    typedef il2cpp::utils::collections::ArrayValueMap<Il2CppMethodPointer, MethodDefinitionKey, MethodInfoToMethodPointerConverter> NativeMethodMap;
    static NativeMethodMap s_NativeMethods;

    struct NativeSymbolMutator
    {
        void operator()(MethodDefinitionKey* method)
        {
            // So, when a function is marked as noreturn, some compilers emit a call to that function and then
            // put the next function immediately after call instruction which means the return address on the stack
            // will appear to point to the wrong function. This messes up our stack walking as we now are confused
            // which method is actually on the stack. To work around this, we add "1" to each of the function addresses,
            // so each function appears to start 1 byte later which means the address on the stack will appear as if
            // it is pointing to function that called the no return function. This is okay because no function will
            // ever return to the first byte of another function.
            method->method = reinterpret_cast<Il2CppMethodPointer>(reinterpret_cast<intptr_t>(method->method) + 1);
        }
    };

    void NativeSymbol::RegisterMethods(const std::vector<MethodDefinitionKey>& managedMethods)
    {
        s_NativeMethods.assign(managedMethods);

#if IL2CPP_MUTATE_METHOD_POINTERS
        NativeSymbolMutator mutator;
        s_NativeMethods.mutate(mutator);
#endif
    }

    static const int k_ProbableMaximumManagedMethodSizeInBytes = 5000;

    static bool IsInstructionPointerProbablyInManagedMethod(Il2CppMethodPointer managedMethodStart, Il2CppMethodPointer instructionPointer)
    {
        if (std::abs((intptr_t)managedMethodStart - (intptr_t)instructionPointer) < k_ProbableMaximumManagedMethodSizeInBytes)
            return true;

        return false;
    }

    const VmMethod* NativeSymbol::GetMethodFromNativeSymbol(Il2CppMethodPointer nativeMethod)
    {
        // Address has to be above our base address
        if ((void*)nativeMethod < il2cpp::os::Image::GetImageBase())
            return NULL;

        // And within the addresses we have our methods in.
        if (il2cpp::os::Image::ManagedSectionExists())
        {
            if (!il2cpp::os::Image::IsInManagedSection((void*)nativeMethod))
                return NULL;
        }

        // Get the first symbol greater than the one we want, because our instruction pointer
        // probably won't be at the start of the method.
        NativeMethodMap::iterator methodAfterNativeMethod = s_NativeMethods.upper_bound(nativeMethod);

        // If method are all of the managed methods are in the same custom section of the binary, then assume we
        // will find the proper method, so the end iterator means we found the last method in this list. If we
        // don't have custom sections, then we may have actually not found the method. In that case, let's not
        // return a method we are unsure of.
        if (!il2cpp::os::Image::ManagedSectionExists())
        {
            if (methodAfterNativeMethod == s_NativeMethods.end())
                return NULL;

            if (!IsInstructionPointerProbablyInManagedMethod(methodAfterNativeMethod->method, nativeMethod))
                return NULL;
        }

        // Go back one to get the method we actually want.
        if (methodAfterNativeMethod != s_NativeMethods.begin())
            methodAfterNativeMethod--;

        return il2cpp::vm::MetadataCache::GetMethodInfoFromMethodHandle(methodAfterNativeMethod->methodHandle);
    }

    static void GetMethodDebugInfoWithSymbols(const MethodInfo* method, intptr_t methodPointer, int32_t size, Il2CppMethodDebugInfo& methodDebugInfo)
    {
        methodDebugInfo.methodPointer = (Il2CppMethodPointer)methodPointer;
        methodDebugInfo.code_size = size;

        // Get the source file name for the method in case usym file is available.
#if !IL2CPP_MONO_DEBUGGER
        // Use usym file if debugging is not available
        il2cpp::utils::SourceLocation sourceLocation;
        if (il2cpp::utils::DebugSymbolReader::GetSourceLocation((void*)methodPointer, sourceLocation))
        {
            methodDebugInfo.file = sourceLocation.filePath;
            methodDebugInfo.line = sourceLocation.lineNumber;
        }
#else
        const Il2CppSequencePoint* sequencePoint = il2cpp::utils::Debugger::GetSequenceFirstSequencePoint(method);
        if (sequencePoint != nullptr)
        {
            const Il2CppSequencePointSourceFile* sequencePointSourceFile = method->klass->image->codeGenModule->debuggerMetadata->sequencePointSourceFiles + sequencePoint->sourceFileIndex;
            methodDebugInfo.file = sequencePointSourceFile->file;
            methodDebugInfo.line = sequencePoint->lineStart;
        }
#endif
        else
        {
            methodDebugInfo.file = NULL;
            methodDebugInfo.line = 0;
        }
    }

#define IL2CPP_DEBUG_DUMP_NATIVE_SYMBOL_DATA 0

#if IL2CPP_DEBUG_DUMP_NATIVE_SYMBOL_DATA
    static void DumpNativeSymbolData()
    {
        // You may want to change this to be a full path so it is easy to locate.
        FILE* dumpFile = fopen("nativeMethodsData.txt", "w");
        for (uint32_t i = 0; i < s_NativeMethods.size(); ++i)
        {
            const MethodDefinitionKey& methodKey = s_NativeMethods[i];

            void* address = methodKey.method;
            const MethodInfo* methodInfo = il2cpp::vm::MetadataCache::GetMethodInfoFromMethodHandle(methodKey.methodHandle);
            if (methodInfo != nullptr)
                fprintf(dumpFile, "%d\t[%p]:\t%s\n", i, address, vm::Method::GetFullName(methodInfo).c_str());
        }
        fclose(dumpFile);
    }

#endif

    void NativeSymbol::GetAllManagedMethodsWithDebugInfo(void(*func)(const MethodInfo* method, Il2CppMethodDebugInfo* methodDebugInfo, void* userData), void* userData)
    {
#if IL2CPP_DEBUG_DUMP_NATIVE_SYMBOL_DATA
        DumpNativeSymbolData();
#endif

        // If we have no data, return as the logic below requires at least one method in the array.
        if (s_NativeMethods.size() == 0)
            return;

        // Store the last image so we can detect boundaries between methods from different images.
        const MethodInfo* prevMethod = nullptr;
        intptr_t prevMethodAddress = 0;
        size_t i = 0;

        // Iterate over all native methods which are registered from imaged and generic instances
        do
        {
            const MethodDefinitionKey& methodInfo = s_NativeMethods[i];

            intptr_t methodAddress = (intptr_t)methodInfo.method;
            // Skip unresolved methods
            if (methodAddress != 0)
            {
                // Get the method info for the method handle.
                // Note this will allocate memory for classes and methods.
                const MethodInfo* method = il2cpp::vm::MetadataCache::GetMethodInfoFromMethodHandle(methodInfo.methodHandle);

                // Report previous method with complete information
                if (prevMethodAddress != 0 && prevMethod != nullptr)
                {
                    int32_t codeSize = (int32_t)(methodAddress - prevMethodAddress);
                    // If the previous method is too far from the current one - we can have other native methods inbetween.
                    // Use heuristic to estimate the size of the method - we expect managed method to be less than
                    // k_ProbableMaximumManagedMethodSizeInBytes, but it still may happen that we have a native method
                    // in the range even between methods in the same image.
                    codeSize = std::min(k_ProbableMaximumManagedMethodSizeInBytes, codeSize);

                    Il2CppMethodDebugInfo methodDebugInfo;
                    GetMethodDebugInfoWithSymbols(prevMethod, prevMethodAddress, codeSize, methodDebugInfo);
                    func(prevMethod, &methodDebugInfo, userData);
                }

                prevMethod = method;
                prevMethodAddress = methodAddress;
            }

            ++i;
        }
        while (i < s_NativeMethods.size());

        if (prevMethodAddress != 0 && prevMethod != nullptr)
        {
            // Report the last method using last method address.
            int32_t codeSize;
            // If there is a managed section we can use it to estimate the method size,
            // otherwise we use a heuristic.
            if (il2cpp::os::Image::ManagedSectionExists())
            {
                void *managedSectionStart, *managedSectionEnd;
                il2cpp::os::Image::GetManagedSectionStartAndEnd(managedSectionStart, managedSectionEnd);
                codeSize = std::min<int32_t>(k_ProbableMaximumManagedMethodSizeInBytes, (int32_t)((intptr_t)managedSectionEnd - prevMethodAddress));
            }
            else
            {
                codeSize = k_ProbableMaximumManagedMethodSizeInBytes;
            }

            Il2CppMethodDebugInfo methodDebugInfo;
            GetMethodDebugInfoWithSymbols(prevMethod, prevMethodAddress, codeSize, methodDebugInfo);
            func(prevMethod, &methodDebugInfo, userData);
        }
    }

    bool NativeSymbol::GetMethodDebugInfo(const MethodInfo* method, Il2CppMethodDebugInfo* methodDebugInfo)
    {
        Il2CppMethodPointer nativeMethod = method->virtualMethodPointer;

        if (il2cpp::os::Image::ManagedSectionExists())
        {
            if (!il2cpp::os::Image::IsInManagedSection((void*)nativeMethod))
                return false;
        }

        if (methodDebugInfo != NULL)
        {
            methodDebugInfo->methodPointer = method->virtualMethodPointer;
            methodDebugInfo->code_size = 0;
            methodDebugInfo->file = NULL;
        }

        return true;
    }

#endif
} /* namespace utils */
} /* namespace il2cpp */
