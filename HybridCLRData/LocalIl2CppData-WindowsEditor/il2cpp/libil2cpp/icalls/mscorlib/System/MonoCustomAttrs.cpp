#include "il2cpp-config.h"
#include <memory>
#include "icalls/mscorlib/System/MonoCustomAttrs.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"
#include "gc/GarbageCollector.h"
#include "metadata/CustomAttributeDataReader.h"
#include "vm/Array.h"
#include "vm/Class.h"
#include "vm/Exception.h"
#include "vm/Object.h"
#include "vm/Reflection.h"
#include "vm/Runtime.h"
#include "vm/Exception.h"
#include "vm/MetadataCache.h"

#include "hybridclr/metadata/MetadataModule.h"

namespace il2cpp
{
namespace icalls
{
namespace mscorlib
{
namespace System
{
    Il2CppArray * MonoCustomAttrs::GetCustomAttributesInternal(Il2CppObject* obj, Il2CppReflectionType* type, bool pseudoAttrs)
    {
        IL2CPP_ASSERT(pseudoAttrs == false && "System_MonoCustomAttrs_GetCustomAttributesInternal_icall with pseudoAttrs == true has not been implemented yet");

        Il2CppClass* attributeClass = type != NULL ? vm::Class::FromIl2CppType(type->type) : NULL;

        Il2CppArray *result = il2cpp::vm::Reflection::GetCustomAttrsInfo(obj, attributeClass);

        if (!result)
            return il2cpp::vm::Array::New(il2cpp_defaults.attribute_class, 0);

        return result;
    }

    bool MonoCustomAttrs::IsDefinedInternal(Il2CppObject *obj, Il2CppReflectionType *attr_type)
    {
        return il2cpp::vm::Reflection::HasAttribute(obj, vm::Class::FromIl2CppType(attr_type->type));
    }

    Il2CppObject* MonoCustomAttrs::CreateCustomAttributeData(const Il2CppAssembly* assembly, const il2cpp::metadata::LazyCustomAttributeData& data)
    {
        static const MethodInfo* customAttributeDataConstructor;
        if (!customAttributeDataConstructor)
        {
            customAttributeDataConstructor = vm::Class::GetMethodFromName(il2cpp_defaults.customattribute_data_class, ".ctor", 4);
            if (customAttributeDataConstructor == NULL)
                IL2CPP_NOT_IMPLEMENTED_ICALL(MonoCustomAttrs::GetCustomAttributesDataInternal);
        }

        Il2CppObject* customAttributeData = vm::Object::New(il2cpp_defaults.customattribute_data_class);
        void* params[] =
        {
            vm::Reflection::GetMethodObject(data.ctor, data.ctor->klass),
            vm::Reflection::GetAssemblyObject(assembly),
            (void*)&data.dataStart,
            (void*)&data.dataLength
        };
        vm::Runtime::Invoke(customAttributeDataConstructor, customAttributeData, params, NULL);
        return customAttributeData;
    }

    Il2CppArray* MonoCustomAttrs::GetCustomAttributesDataInternal(Il2CppObject* obj)
    {
        il2cpp::vm::ReflectionObjInfo refInfo = il2cpp::vm::Reflection::GetImageOfReflectionObject(obj);
        metadata::CustomAttributeDataReader reader = il2cpp::vm::Reflection::GetCustomAttrsDataReader(obj);

        uint32_t count = reader.GetCount();

        Il2CppArray* result = il2cpp::vm::Array::New(il2cpp_defaults.customattribute_data_class, reader.GetCount());

        if (count == 0)
            return result;

        uint32_t i = 0;

        bool hasError = false;
        il2cpp::metadata::LazyCustomAttributeData data;
        Il2CppException* exc = NULL;
        il2cpp::metadata::CustomAttributeDataIterator iter = reader.GetDataIterator();
        while (reader.ReadLazyCustomAttributeData(&data, &iter, &exc))
        {
            IL2CPP_ASSERT(i < reader.GetCount());
            Il2CppObject* attributeData = CreateCustomAttributeData(obj->klass->image->assembly, data);
            il2cpp_array_setref(result, i, attributeData);
            i++;
        }

        if (exc != NULL)
            vm::Exception::Raise(exc);

        return result;
    }
} /* namespace System */
} /* namespace mscorlib */
} /* namespace icalls */
} /* namespace il2cpp */
