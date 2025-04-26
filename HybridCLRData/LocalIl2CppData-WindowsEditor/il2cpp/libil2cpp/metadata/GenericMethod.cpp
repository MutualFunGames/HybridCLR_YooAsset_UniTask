#include "il2cpp-config.h"
#include "metadata/GenericMetadata.h"
#include "metadata/GenericMethod.h"
#include "metadata/GenericSharing.h"
#include "metadata/Il2CppGenericMethodCompare.h"
#include "metadata/Il2CppGenericMethodHash.h"
#include "os/Atomic.h"
#include "os/Mutex.h"
#include "utils/Memory.h"
#include "vm/Class.h"
#include "vm/Exception.h"
#include "vm/GenericClass.h"
#include "vm/MetadataAlloc.h"
#include "vm/MetadataCache.h"
#include "vm/MetadataLock.h"
#include "vm/Method.h"
#include "vm/Runtime.h"
#include "vm/Type.h"
#include "utils/Il2CppHashMap.h"
#include "utils/InitOnce.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-runtime-metadata.h"
#include "il2cpp-runtime-stats.h"
#include <string>

#include "hybridclr/metadata/MetadataUtil.h"
#include "hybridclr/metadata/MetadataModule.h"
#include "hybridclr/interpreter/InterpreterModule.h"

using il2cpp::metadata::GenericMetadata;
using il2cpp::metadata::GenericSharing;
using il2cpp::os::FastAutoLock;
using il2cpp::vm::Class;
using il2cpp::vm::GenericClass;
using il2cpp::vm::MetadataCalloc;
using il2cpp::vm::MetadataCache;
using il2cpp::vm::Method;
using il2cpp::vm::Runtime;
using il2cpp::vm::Type;

struct FullySharedGenericMethodInfo : public MethodInfo
{
    FullySharedGenericMethodInfo() { memset(this, 0, sizeof(*this)); }
    Il2CppMethodPointer rawVirtualMethodPointer;
    Il2CppMethodPointer rawDirectMethodPointer;
    InvokerMethod rawInvokerMethod;
};

static size_t SizeOfGenericMethodInfo(bool hasFullGenericSignature)
{
    if (hasFullGenericSignature)
        return sizeof(FullySharedGenericMethodInfo);
    return sizeof(MethodInfo);
}

static MethodInfo* AllocGenericMethodInfo(bool hasFullGenericSignature)
{
    return (MethodInfo*)MetadataCalloc(1, SizeOfGenericMethodInfo(hasFullGenericSignature));
}

static MethodInfo* AllocCopyGenericMethodInfo(const MethodInfo* sourceMethodInfo)
{
    MethodInfo* newMethodInfo = AllocGenericMethodInfo(sourceMethodInfo->has_full_generic_sharing_signature);
    memcpy(newMethodInfo, sourceMethodInfo, SizeOfGenericMethodInfo(sourceMethodInfo->has_full_generic_sharing_signature));
    return newMethodInfo;
}

static void FullySharedGenericInvokeRedirectHasAdjustorThunk(Il2CppMethodPointer methodPointer, const MethodInfo* method, void* obj, void** args, void* retVal)
{
    IL2CPP_ASSERT(Method::IsGenericInstance(method));
    IL2CPP_ASSERT(il2cpp::vm::Runtime::IsFullGenericSharingEnabled());
    IL2CPP_ASSERT(methodPointer == method->virtualMethodPointer || methodPointer == method->methodPointer);

    const FullySharedGenericMethodInfo* sharedMethodInfo = reinterpret_cast<const FullySharedGenericMethodInfo*>(method);
    IL2CPP_ASSERT(sharedMethodInfo->rawDirectMethodPointer != sharedMethodInfo->rawVirtualMethodPointer);

    if (methodPointer == sharedMethodInfo->virtualMethodPointer)
        sharedMethodInfo->rawInvokerMethod(sharedMethodInfo->rawVirtualMethodPointer, method, obj, args, retVal);
    else
        sharedMethodInfo->rawInvokerMethod(sharedMethodInfo->rawDirectMethodPointer, method, obj, args, retVal);
}

static void FullySharedGenericInvokeRedirectNoAdjustorThunk(Il2CppMethodPointer methodPointer, const MethodInfo* method, void* obj, void** args, void* retVal)
{
    IL2CPP_ASSERT(Method::IsGenericInstance(method));
    IL2CPP_ASSERT(il2cpp::vm::Runtime::IsFullGenericSharingEnabled());
    IL2CPP_ASSERT(methodPointer == method->methodPointer || methodPointer == method->virtualMethodPointer);

    const FullySharedGenericMethodInfo* sharedMethodInfo = reinterpret_cast<const FullySharedGenericMethodInfo*>(method);
    IL2CPP_ASSERT(sharedMethodInfo->rawDirectMethodPointer == sharedMethodInfo->rawVirtualMethodPointer);

    sharedMethodInfo->rawInvokerMethod(sharedMethodInfo->rawDirectMethodPointer, method, obj, args, retVal);
}

namespace il2cpp
{
namespace metadata
{
    typedef Il2CppReaderWriterLockedHashMap<const Il2CppGenericMethod*, MethodInfo*, Il2CppGenericMethodHash, Il2CppGenericMethodCompare> Il2CppGenericMethodMap;
    static Il2CppGenericMethodMap s_GenericMethodMap;
    static Il2CppGenericMethodMap s_PendingGenericMethodMap;

    static bool HasFullGenericSharedParametersOrReturn(const MethodInfo* methodDefinition, const Il2CppType** inflatedParameterTypes)
    {
        // If a method has a variable sized return type, the FGS method will always
        // expect the return value to be passed as a by ref parameter
        if (Type::HasVariableRuntimeSizeWhenFullyShared(methodDefinition->return_type))
            return true;

        for (int i = 0; i < methodDefinition->parameters_count; i++)
        {
            // Value types are passed by ref, but reference types are passed normally, so if the inflated parameter is a
            // reference type, we don't have a signature difference.
            if (Type::IsValueType(inflatedParameterTypes[i]) && Type::HasVariableRuntimeSizeWhenFullyShared(methodDefinition->parameters[i]))
                return true;
        }

        return false;
    }

    static void AnUnresolvedCallStubWasNotFound()
    {
        vm::Exception::Raise(vm::Exception::GetExecutionEngineException("An unresolved indirect call lookup failed"));
    }

    // This method must have a different signature than AnUnresolvedCallStubWasNotFound to prevent identical COMDAT folding
    // FullySharedGenericInvokeRedirectHasAdjustorThunk relies on this method having a different address
    static void AnUnresolvedCallStubWasNotFoundValueType(void* obj)
    {
        vm::Exception::Raise(vm::Exception::GetExecutionEngineException("An unresolved indirect call to a value type failed"));
    }

    static void AGenericMethodWhichIsTooDeeplyNestedWasInvoked()
    {
        vm::Exception::Raise(vm::Exception::GetMaximumNestedGenericsException());
    }

    static void AGenericMethodWhichIsTooDeeplyNestedWasInvokedInvoker(Il2CppMethodPointer ptr, const MethodInfo* method, void* obj, void** args, void* ret)
    {
        AGenericMethodWhichIsTooDeeplyNestedWasInvoked();
    }

    static FullySharedGenericMethodInfo ambiguousMethodInfo;

    bool GenericMethod::IsGenericAmbiguousMethodInfo(const MethodInfo* method)
    {
        return method == &ambiguousMethodInfo;
    }

    const MethodInfo* GenericMethod::GetGenericVirtualMethod(const MethodInfo* vtableSlotMethod, const MethodInfo* genericVirtualMethod)
    {
        IL2CPP_NOT_IMPLEMENTED_NO_ASSERT(GetGenericVirtualMethod, "We should only do the following slow method lookup once and then cache on type itself.");

        const Il2CppGenericInst* classInst = NULL;
        if (vtableSlotMethod->is_inflated)
        {
            classInst = vtableSlotMethod->genericMethod->context.class_inst;
            vtableSlotMethod = vtableSlotMethod->genericMethod->methodDefinition;
        }

        return metadata::GenericMethod::GetMethod(vtableSlotMethod, classInst, genericVirtualMethod->genericMethod->context.method_inst);
    }

    const MethodInfo* GenericMethod::GetMethod(const MethodInfo* methodDefinition, const Il2CppGenericInst* classInst, const Il2CppGenericInst* methodInst)
    {
        Il2CppGenericMethod gmethod = { 0 };
        gmethod.methodDefinition = methodDefinition;
        gmethod.context.class_inst = classInst;
        gmethod.context.method_inst = methodInst;
        return GetMethod(&gmethod, true);
    }

    MethodInfo* GenericMethod::AllocateNewMethodInfo(const MethodInfo* methodDefinition, const Il2CppGenericInst* classInst, const Il2CppGenericInst* methodInst)
    {
        const MethodInfo* methodInfo = GetMethod(methodDefinition, classInst, methodInst);
        return AllocCopyGenericMethodInfo(methodInfo);
    }

    const MethodInfo* GenericMethod::GetMethod(const Il2CppGenericMethod* gmethod)
    {
        return GetMethod(gmethod, false);
    }

    const MethodInfo* GenericMethod::GetMethod(const Il2CppGenericMethod* gmethod, bool copyMethodPtr)
    {
        // This can be NULL only when we have hit the generic recursion depth limit.
        if (gmethod == NULL)
        {
            MethodInfo* newMethod = AllocGenericMethodInfo(il2cpp::vm::Runtime::IsFullGenericSharingEnabled());
            if (il2cpp::vm::Runtime::IsFullGenericSharingEnabled())
            {
                ((FullySharedGenericMethodInfo*)newMethod)->rawVirtualMethodPointer = AGenericMethodWhichIsTooDeeplyNestedWasInvoked;
                ((FullySharedGenericMethodInfo*)newMethod)->rawDirectMethodPointer = AGenericMethodWhichIsTooDeeplyNestedWasInvoked;
                ((FullySharedGenericMethodInfo*)newMethod)->rawInvokerMethod = AGenericMethodWhichIsTooDeeplyNestedWasInvokedInvoker;
            }

            newMethod->methodPointer = AGenericMethodWhichIsTooDeeplyNestedWasInvoked;
            newMethod->virtualMethodPointer = AGenericMethodWhichIsTooDeeplyNestedWasInvoked;
            newMethod->invoker_method = AGenericMethodWhichIsTooDeeplyNestedWasInvokedInvoker;
            return newMethod;
        }

        // First check for an already constructed generic method using the shared/reader lock
        MethodInfo* existingMethod;
        if (s_GenericMethodMap.TryGet(gmethod, &existingMethod))
            return existingMethod;

        if (Method::IsAmbiguousMethodInfo(gmethod->methodDefinition))
        {
            // is_inflated is used as an initialized check
            if (!ambiguousMethodInfo.is_inflated)
            {
                memcpy(&ambiguousMethodInfo, gmethod->methodDefinition, sizeof(MethodInfo));
                ambiguousMethodInfo.is_inflated = true;
                ambiguousMethodInfo.rawVirtualMethodPointer = gmethod->methodDefinition->virtualMethodPointer;
                ambiguousMethodInfo.rawDirectMethodPointer = gmethod->methodDefinition->methodPointer;
                ambiguousMethodInfo.invoker_method = gmethod->methodDefinition->invoker_method;
            }

            return &ambiguousMethodInfo;
        }

        return CreateMethodLocked(gmethod, copyMethodPtr);
    }

    const MethodInfo* GenericMethod::CreateMethodLocked(const Il2CppGenericMethod* gmethod, bool copyMethodPtr)
    {
        // We need to inflate a new generic method, take the metadata mutex
        // All code below this point can and does assume mutual exclusion
        FastAutoLock lock(&il2cpp::vm::g_MetadataLock);

        // Recheck the s_GenericMethodMap in case there was a race to add this generic method
        MethodInfo* existingMethod;
        if (s_GenericMethodMap.TryGet(gmethod, &existingMethod))
            return existingMethod;

        // GetMethodLocked may be called recursively, we keep tracking of pending inflations
        if (s_PendingGenericMethodMap.TryGet(gmethod, &existingMethod))
            return existingMethod;

        if (copyMethodPtr)
            gmethod = MetadataCache::GetGenericMethod(gmethod->methodDefinition, gmethod->context.class_inst, gmethod->context.method_inst);

        const MethodInfo* methodDefinition = gmethod->methodDefinition;
        Il2CppClass* declaringClass = methodDefinition->klass;
        if (gmethod->context.class_inst)
        {
            Il2CppGenericClass* genericClassDeclaringType = GenericMetadata::GetGenericClass(methodDefinition->klass, gmethod->context.class_inst);
            declaringClass = GenericClass::GetClass(genericClassDeclaringType);

            // we may fail if we cannot construct generic type
            if (!declaringClass)
                return NULL;
        }

        const Il2CppType** parameters = GenericMetadata::InflateParameters(methodDefinition->parameters, methodDefinition->parameters_count, &gmethod->context, true);
        il2cpp::vm::Il2CppGenericMethodPointers methodPointers = MetadataCache::GetGenericMethodPointers(methodDefinition, &gmethod->context);
        bool hasFullGenericSharingSignature = methodPointers.isFullGenericShared && HasFullGenericSharedParametersOrReturn(gmethod->methodDefinition, parameters);

        MethodInfo* newMethod = AllocGenericMethodInfo(hasFullGenericSharingSignature);

        // we set the pending generic method map here because the initialization may recurse and try to retrieve the same generic method
        // this is safe because we *always* take the lock when retrieving the MethodInfo from a generic method.
        // if we move lock to only if MethodInfo needs constructed then we need to revisit this since we could return a partially initialized MethodInfo
        s_PendingGenericMethodMap.Add(gmethod, newMethod);

        newMethod->klass = declaringClass;
        newMethod->flags = methodDefinition->flags;
        newMethod->iflags = methodDefinition->iflags;
        newMethod->slot = methodDefinition->slot;
        newMethod->name = methodDefinition->name;
        newMethod->is_generic = false;
        newMethod->is_inflated = true;
        newMethod->token = methodDefinition->token;

        newMethod->return_type = GenericMetadata::InflateIfNeeded(methodDefinition->return_type, &gmethod->context, true);

        newMethod->parameters_count = methodDefinition->parameters_count;
        newMethod->parameters = parameters;

        newMethod->genericMethod = gmethod;

        if (!gmethod->context.method_inst)
        {
            if (methodDefinition->is_generic)
                newMethod->is_generic = true;

            if (!declaringClass->generic_class)
            {
                newMethod->genericContainerHandle = methodDefinition->genericContainerHandle;
            }

            newMethod->methodMetadataHandle = methodDefinition->methodMetadataHandle;
        }
        else if (!il2cpp::vm::Runtime::IsLazyRGCTXInflationEnabled() && !il2cpp::metadata::GenericMetadata::ContainsGenericParameters(newMethod))
        {
            // we only need RGCTX for generic instance methods
            newMethod->rgctx_data = InflateRGCTXLocked(gmethod, lock);
        }

        newMethod->virtualMethodPointer = methodPointers.virtualMethodPointer;
        newMethod->methodPointer = methodPointers.methodPointer;
        if (methodPointers.methodPointer)
        {
            newMethod->invoker_method = methodPointers.invoker_method;
        }
        else
        {
            newMethod->invoker_method = Runtime::GetMissingMethodInvoker();
            il2cpp::vm::Il2CppUnresolvedCallStubs stubs = MetadataCache::GetUnresovledCallStubs(newMethod);
            newMethod->methodPointer = stubs.methodPointer;
            newMethod->virtualMethodPointer = stubs.virtualMethodPointer;
        }

        newMethod->has_full_generic_sharing_signature = hasFullGenericSharingSignature;
        bool isInterpMethod = hybridclr::metadata::IsInterpreterMethod(newMethod);
        if (!isInterpMethod)
        {
            newMethod->has_full_generic_sharing_signature = hasFullGenericSharingSignature;
        }

        ++il2cpp_runtime_stats.inflated_method_count;

        bool indirectCallViaInvokers = il2cpp::vm::Method::HasFullGenericSharingSignature(newMethod);
        if (indirectCallViaInvokers)
        {
            // The method has a full generic sharing signature - that is it a fully shared method an has any fully shared parameter types or return type,
            // then its signature doesn't match the expected signature
            // e.g. If List<T>::Insert(T t) is fully shared then for List<int>::Insert(int), the C++ fully shared instance would be List::Insert(void*) and require an int* to be passed in.
            // So in that case we use the unresolved call stubs to find a matching standard signature to wrap any indirect/virtual calls

            FullySharedGenericMethodInfo* sharedMethodInfo = reinterpret_cast<FullySharedGenericMethodInfo*>(newMethod);
            sharedMethodInfo->rawVirtualMethodPointer = newMethod->virtualMethodPointer;
            sharedMethodInfo->rawDirectMethodPointer = newMethod->methodPointer;
            sharedMethodInfo->rawInvokerMethod = newMethod->invoker_method;

            bool hasAdjustorThunk = newMethod->methodPointer != newMethod->virtualMethodPointer;
            if (hasAdjustorThunk)
                newMethod->invoker_method = FullySharedGenericInvokeRedirectHasAdjustorThunk;
            else
                newMethod->invoker_method = FullySharedGenericInvokeRedirectNoAdjustorThunk;

            il2cpp::vm::Il2CppUnresolvedCallStubs stubs = MetadataCache::GetUnresovledCallStubs(newMethod);
            if (stubs.stubsFound)
            {
                newMethod->methodPointer = stubs.methodPointer;
                newMethod->virtualMethodPointer = stubs.virtualMethodPointer;
            }
            else
            {
                newMethod->methodPointer = AnUnresolvedCallStubWasNotFound;
                newMethod->virtualMethodPointer = AnUnresolvedCallStubWasNotFound;

                if (hasAdjustorThunk)
                {
                    // The FullySharedGenericInvokeRedirectHasAdjustorThunk requires that methodPointer and virtualMethodPointer be different
                    // so it can tell which raw* method it should call even though it doesn't directly call them
                    IL2CPP_ASSERT(reinterpret_cast<Il2CppMethodPointer>(AnUnresolvedCallStubWasNotFoundValueType) != AnUnresolvedCallStubWasNotFound);
                    if (reinterpret_cast<Il2CppMethodPointer>(AnUnresolvedCallStubWasNotFoundValueType) != AnUnresolvedCallStubWasNotFound)
                    {
                        newMethod->methodPointer = reinterpret_cast<Il2CppMethodPointer>(AnUnresolvedCallStubWasNotFoundValueType);
                    }
                    else
                    {
                        // If we got hit by COMDAT folding (but in DEBUG, which is the most likely way it would happen)
                        // Ensure that are methodPointers are definitely different
                        // We'll get an less specific error message, but it's better than corruption
                        // Make the change on methodPointer because we're most likely to be called
                        newMethod->methodPointer = il2cpp::vm::Method::GetEntryPointNotFoundMethodInfo()->methodPointer;
                    }
                }
            }
        }
        else
        {
            newMethod->methodPointerCallByInterp = newMethod->methodPointer;
            newMethod->virtualMethodPointerCallByInterp = newMethod->virtualMethodPointer;
        }

        bool isAotImplByInterp = hybridclr::metadata::MetadataModule::IsImplementedByInterpreter(newMethod);
        bool isAdjustorThunkMethod = IS_CLASS_VALUE_TYPE(newMethod->klass) && hybridclr::metadata::IsInstanceMethod(newMethod);
        if (isInterpMethod || (isAotImplByInterp && (newMethod->methodPointer == nullptr || newMethod->methodPointer == AnUnresolvedCallStubWasNotFound || newMethod->methodPointer == (Il2CppMethodPointer)AnUnresolvedCallStubWasNotFoundValueType)))
        {
            newMethod->invoker_method = hybridclr::interpreter::InterpreterModule::GetMethodInvoker(newMethod);
            newMethod->methodPointer = newMethod->methodPointerCallByInterp = hybridclr::interpreter::InterpreterModule::GetMethodPointer(newMethod);
            newMethod->virtualMethodPointer = newMethod->virtualMethodPointerCallByInterp = isAdjustorThunkMethod ?
                hybridclr::interpreter::InterpreterModule::GetAdjustThunkMethodPointer(newMethod) :
                (newMethod->methodPointerCallByInterp != hybridclr::interpreter::InterpreterModule::NotSupportNative2Managed ?
                    newMethod->methodPointerCallByInterp : hybridclr::interpreter::InterpreterModule::NotSupportAdjustorThunk);
            if (indirectCallViaInvokers)
            {
                FullySharedGenericMethodInfo* sharedMethodInfo = reinterpret_cast<FullySharedGenericMethodInfo*>(newMethod);
                if (!hybridclr::interpreter::InterpreterModule::HasImplementCallNative2Managed(newMethod))
                {
                    newMethod->methodPointer = newMethod->methodPointerCallByInterp = sharedMethodInfo->rawDirectMethodPointer;
                }
                if (!hybridclr::interpreter::InterpreterModule::HasImplementCallVirtualNative2Managed(newMethod))
                {
                    newMethod->virtualMethodPointer = newMethod->virtualMethodPointerCallByInterp = sharedMethodInfo->rawVirtualMethodPointer;
                }
            }
            newMethod->initInterpCallMethodPointer = true;
            newMethod->isInterpterImpl = true;
        }
        else if (newMethod->methodPointer != AnUnresolvedCallStubWasNotFound && newMethod->methodPointer != (Il2CppMethodPointer)AnUnresolvedCallStubWasNotFoundValueType)
        {
            newMethod->methodPointerCallByInterp = newMethod->methodPointer;
            newMethod->virtualMethodPointerCallByInterp = newMethod->virtualMethodPointer;
        }
        // IL2CPP_ASSERT(!indirectCallViaInvokers || !isAdjustorThunkMethod || newMethod->methodPointerCallByInterp != newMethod->virtualMethodPointerCallByInterp);

        // If we are a default interface method on a generic instance interface we need to ensure that the interfaces rgctx is inflated
        if (Method::IsDefaultInterfaceMethodOnGenericInstance(newMethod))
            vm::Class::InitLocked(declaringClass, lock);

        // The generic method is fully created,
        // Update the generic method map, this needs to take an exclusive lock
        // **** This must happen with the metadata lock held and be released before the metalock is released ****
        // **** This prevents deadlocks and ensures that there is no race condition
        // **** creating a new method adding it to s_GenericMethodMap and removing it from s_PendingGenericMethodMap
        s_GenericMethodMap.Add(gmethod, newMethod);

        // Remove the method from the pending table
        s_PendingGenericMethodMap.Remove(gmethod);

        return newMethod;
    }

    const Il2CppRGCTXData* GenericMethod::InflateRGCTX(const MethodInfo* method)
    {
        IL2CPP_ASSERT(method->is_inflated);
        IL2CPP_ASSERT(method->genericMethod);
        IL2CPP_ASSERT(method->genericMethod->context.method_inst);

        return il2cpp::utils::InitOnce(const_cast<Il2CppRGCTXData**>(&method->rgctx_data), &il2cpp::vm::g_MetadataLock, [method](const il2cpp::os::FastAutoLock& lock) {
            return const_cast<Il2CppRGCTXData*>(GenericMethod::InflateRGCTXLocked(method->genericMethod, lock));
        });
    }

    const Il2CppRGCTXData* GenericMethod::InflateRGCTXLocked(const Il2CppGenericMethod* gmethod, const il2cpp::os::FastAutoLock &lock)
    {
        return GenericMetadata::InflateRGCTXLocked(gmethod->methodDefinition->klass->image, gmethod->methodDefinition->token, &gmethod->context, lock);
    }

    const Il2CppGenericContext* GenericMethod::GetContext(const Il2CppGenericMethod* gmethod)
    {
        return &gmethod->context;
    }

    static std::string FormatGenericArguments(const Il2CppGenericInst* inst)
    {
        std::string output;
        if (inst)
        {
            output.append("<");
            for (size_t i = 0; i < inst->type_argc; ++i)
            {
                if (i != 0)
                    output.append(", ");
                output.append(Type::GetName(inst->type_argv[i], IL2CPP_TYPE_NAME_FORMAT_FULL_NAME));
            }
            output.append(">");
        }

        return output;
    }

    std::string GenericMethod::GetFullName(const Il2CppGenericMethod* gmethod)
    {
        const MethodInfo* method = gmethod->methodDefinition;
        std::string output;
        output.append(Type::GetName(&gmethod->methodDefinition->klass->byval_arg, IL2CPP_TYPE_NAME_FORMAT_FULL_NAME));
        output.append(FormatGenericArguments(gmethod->context.class_inst));
        output.append("::");
        output.append(Method::GetName(method));
        output.append(FormatGenericArguments(gmethod->context.method_inst));

        return output;
    }

    void GenericMethod::ClearStatics()
    {
        s_GenericMethodMap.Clear();
    }
} /* namespace vm */
} /* namespace il2cpp */
