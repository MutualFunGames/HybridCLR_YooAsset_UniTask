#include "il2cpp-config.h"
#include "il2cpp-class-internals.h"
#include "icalls/mscorlib/System.Runtime.InteropServices/GCHandle.h"
#include "gc/GCHandle.h"
#include "vm/Class.h"
#include "vm/Exception.h"
#include "vm/GenericClass.h"
#include "vm/Type.h"

namespace il2cpp
{
namespace icalls
{
namespace mscorlib
{
namespace System
{
namespace Runtime
{
namespace InteropServices
{
    bool GCHandle::CheckCurrentDomain(intptr_t handle)
    {
        return true; // il2cpp doesn't support multiple domains
    }

    void GCHandle::FreeHandle(intptr_t handle)
    {
        gc::GCHandle::Free((Il2CppGCHandle)handle);
    }

// Returns -2 if gchandle is not pinned
    intptr_t GCHandle::GetAddrOfPinnedObject(intptr_t handle)
    {
        gc::GCHandleType type = gc::GCHandle::GetHandleType((Il2CppGCHandle)handle);

        if (type != gc::HANDLE_PINNED)
            return reinterpret_cast<intptr_t>(reinterpret_cast<uint8_t*>(-2)); // mscorlib on managed land expects us to return "-2" as IntPtr if this condition occurs

        Il2CppObject* obj = gc::GCHandle::GetTarget((Il2CppGCHandle)handle);
        if (obj == NULL)
            return 0;

        ptrdiff_t offset;

        if (obj->klass->rank > 0)
        {
            // Pointer to first array element
            offset = kIl2CppSizeOfArray;
        }
        else if (obj->klass->byval_arg.type == IL2CPP_TYPE_STRING)
        {
            // Pointer to first character
            offset = offsetof(Il2CppString, chars);
        }
        else
        {
            // Pointer to struct in boxed object
            offset = sizeof(Il2CppObject);
        }

        return reinterpret_cast<intptr_t>((reinterpret_cast<uint8_t*>(obj) + offset));
    }

    Il2CppObject* GCHandle::GetTarget(intptr_t handle)
    {
        return gc::GCHandle::GetTarget((Il2CppGCHandle)handle);
    }

    static bool IsTypePinnable(Il2CppClass* klass)
    {
        // IL2CPP is matching the .NET Core behavior now, not .NET Framework.
        // Any type that does not have fields which are reference types can be
        // pinned.
        return !vm::Class::HasReferences(klass);
    }

    static inline bool IsObjectPinnable(Il2CppObject* obj)
    {
        if (obj == NULL)
            return true;

        return IsTypePinnable(obj->klass);
    }

    intptr_t GCHandle::GetTargetHandle(Il2CppObject* obj, intptr_t handle, int32_t type)
    {
        if (type == gc::HANDLE_PINNED && !IsObjectPinnable(obj))
        {
            Il2CppException* ex = vm::Exception::GetArgumentException(NULL, "Object contains references.");
            vm::Exception::Raise(ex);
        }

        auto targetHandle = gc::GCHandle::GetTargetHandle(obj, (Il2CppGCHandle)handle, type);
        vm::Exception::RaiseIfError(targetHandle.GetError());
        return (intptr_t)targetHandle.Get();
    }
} /* namespace InteropServices */
} /* namespace Runtime */
} /* namespace System */
} /* namespace mscorlib */
} /* namespace icalls */
} /* namespace il2cpp */
