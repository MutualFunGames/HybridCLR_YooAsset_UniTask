#include "il2cpp-config.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"
#include "il2cpp-tabledefs.h"
#include "mono-structs.h"
#include "gc/GCHandle.h"
#include "gc/WriteBarrier.h"
#include "metadata/CustomAttributeDataReader.h"
#include "metadata/CustomAttributeCreator.h"
#include "metadata/Il2CppTypeCompare.h"
#include "metadata/Il2CppTypeHash.h"
#include "vm/Array.h"
#include "vm/Class.h"
#include "vm/Event.h"
#include "vm/Exception.h"
#include "vm/Field.h"
#include "vm/Image.h"
#include "vm/MetadataCache.h"
#include "vm/Method.h"
#include "vm/Object.h"
#include "vm/Parameter.h"
#include "vm/Property.h"
#include "vm/Reflection.h"
#include "vm/String.h"
#include "vm/AssemblyName.h"
#include "utils/Il2CppHashMap.h"
#include "utils/StringUtils.h"
#include "utils/HashUtils.h"
#include "gc/AppendOnlyGCHashMap.h"


#include "gc/Allocator.h"

template<typename T>
struct ReflectionMapHash
{
    size_t operator()(const T& ea) const
    {
        return ((size_t)(intptr_t)(ea.first)) >> 3;
    }
};

template<typename T>
struct ReflectionMapLess
{
    bool operator()(const T& ea, const T& eb) const
    {
        if (ea.first < eb.first)
            return true;
        if (ea.second < eb.second)
            return true;
        return false;
    }
};


template<typename Key, typename Value>
struct ReflectionMap : public il2cpp::gc::AppendOnlyGCHashMap<Key, Value, ReflectionMapHash<Key> >
{
};

typedef ReflectionMap<std::pair<const Il2CppAssembly*, Il2CppClass*>, Il2CppReflectionAssembly*> AssemblyMap;
typedef ReflectionMap<std::pair<const FieldInfo*, Il2CppClass*>, Il2CppReflectionField*> FieldMap;
typedef ReflectionMap<std::pair<const PropertyInfo*, Il2CppClass*>, Il2CppReflectionProperty*> PropertyMap;
typedef ReflectionMap<std::pair<const EventInfo*, Il2CppClass*>, Il2CppReflectionEvent*> EventMap;
typedef ReflectionMap<std::pair<const MethodInfo*, Il2CppClass*>, Il2CppReflectionMethod*> MethodMap;
typedef ReflectionMap<std::pair<const Il2CppImage*, Il2CppClass*>, Il2CppReflectionModule*> ModuleMap;
typedef ReflectionMap<std::pair<const MethodInfo*, Il2CppClass*>, Il2CppArray*> ParametersMap;

typedef il2cpp::gc::AppendOnlyGCHashMap<const Il2CppType*, Il2CppReflectionType*, il2cpp::metadata::Il2CppTypeHash, il2cpp::metadata::Il2CppTypeEqualityComparer> TypeMap;

typedef Il2CppHashMap<Il2CppMetadataGenericParameterHandle, const MonoGenericParameterInfo*, il2cpp::utils::PassThroughHash<Il2CppMetadataGenericParameterHandle> > MonoGenericParameterMap;
typedef Il2CppHashMap<const  Il2CppAssembly*, const Il2CppMonoAssemblyName*, il2cpp::utils::PointerHash<const Il2CppAssembly> > MonoAssemblyNameMap;

// these needs to be pointers and allocated after GC is initialized since it uses GC Allocator
static AssemblyMap* s_AssemblyMap;
static FieldMap* s_FieldMap;
static PropertyMap* s_PropertyMap;
static EventMap* s_EventMap;
static MethodMap* s_MethodMap;
static ModuleMap* s_ModuleMap;
static ParametersMap* s_ParametersMap;
static TypeMap* s_TypeMap;
static MonoGenericParameterMap* s_MonoGenericParamterMap;
static MonoAssemblyNameMap* s_MonoAssemblyNameMap;

namespace il2cpp
{
namespace vm
{
    static Il2CppClass *s_System_Reflection_Assembly;
    static Il2CppClass * s_System_Reflection_RuntimeFieldInfoKlass;
    static Il2CppClass *s_System_Reflection_Module;
    static Il2CppClass * s_System_Reflection_RuntimePropertyInfoKlass;
    static Il2CppClass * s_System_Reflection_RuntimeEventInfoKlass;
    static FieldInfo *s_DbNullValueField;
    static FieldInfo *s_ReflectionMissingField;
    static Il2CppClass *s_System_Reflection_ParameterInfo;
    static Il2CppClass *s_System_Reflection_ParameterInfo_array;
/*
 * We use the same C representation for methods and constructors, but the type
 * name in C# is different.
 */
    static Il2CppClass *s_System_Reflection_MethodInfo;
    static Il2CppClass *s_System_Reflection_ConstructorInfo;

    static il2cpp::metadata::CustomAttributeFilter GetFilter(Il2CppClass*& attributeClass)
    {
        if (attributeClass == NULL)
            return [](const MethodInfo*) { return true; };

        return [attributeClass](const MethodInfo* ctor) {
                Il2CppClass* klass = ctor->klass;
                return il2cpp::vm::Class::HasParent(klass, attributeClass) || (il2cpp::vm::Class::IsInterface(attributeClass) && il2cpp::vm::Class::IsAssignableFrom(attributeClass, klass));
        };
    }

    Il2CppReflectionAssembly* Reflection::GetAssemblyObject(const Il2CppAssembly *assembly)
    {
        Il2CppReflectionAssembly *res;

        AssemblyMap::key_type::wrapped_type key(assembly, (Il2CppClass*)NULL);
        AssemblyMap::data_type value = NULL;

        if (s_AssemblyMap->TryGetValue(key, &value))
            return value;

        res = (Il2CppReflectionAssembly*)Object::New(s_System_Reflection_Assembly);
        res->assembly = assembly;

        return s_AssemblyMap->GetOrAdd(key, res);
    }

    Il2CppReflectionAssemblyName* Reflection::GetAssemblyNameObject(const Il2CppAssemblyName *assemblyName)
    {
        IL2CPP_ASSERT(il2cpp_defaults.assembly_name_class != NULL);

        std::string fullAssemblyName = vm::AssemblyName::AssemblyNameToString(*assemblyName);
        Il2CppReflectionAssemblyName* reflectionAssemblyName = (Il2CppReflectionAssemblyName*)Object::New(il2cpp_defaults.assembly_name_class);
        vm::AssemblyName::ParseName(reflectionAssemblyName, fullAssemblyName);
        return reflectionAssemblyName;
    }

    Il2CppReflectionField* Reflection::GetFieldObject(Il2CppClass *klass, FieldInfo *field)
    {
        Il2CppReflectionField *res;

        FieldMap::key_type::wrapped_type key(field, klass);
        FieldMap::data_type value = NULL;

        if (s_FieldMap->TryGetValue(key, &value))
            return value;

        res = (Il2CppReflectionField*)Object::New(s_System_Reflection_RuntimeFieldInfoKlass);
        res->klass = klass;
        res->field = field;
        IL2CPP_OBJECT_SETREF(res, name, String::New(Field::GetName(field)));
        res->attrs = field->type->attrs;
        IL2CPP_OBJECT_SETREF(res, type, GetTypeObject(field->type));

        return s_FieldMap->GetOrAdd(key, res);
    }

    const FieldInfo* Reflection::GetField(const Il2CppReflectionField* field)
    {
        return field->field;
    }

    const MethodInfo* Reflection::GetMethod(const Il2CppReflectionMethod* method)
    {
        return method->method;
    }

    Il2CppReflectionMethod* Reflection::GetMethodObject(const MethodInfo *method, Il2CppClass *refclass)
    {
        Il2CppClass *klass;
        Il2CppReflectionMethod *ret;

        if (!refclass)
            refclass = method->klass;

        MethodMap::key_type::wrapped_type key(method, refclass);
        MethodMap::data_type value = NULL;

        if (s_MethodMap->TryGetValue(key, &value))
            return value;

        if (*method->name == '.' && (strcmp(method->name, ".ctor") == 0 || strcmp(method->name, ".cctor") == 0))
        {
            klass = s_System_Reflection_ConstructorInfo;
        }
        else
        {
            klass = s_System_Reflection_MethodInfo;
        }
        ret = (Il2CppReflectionMethod*)Object::New(klass);
        ret->method = method;
        IL2CPP_OBJECT_SETREF(ret, reftype, GetTypeObject(&refclass->byval_arg));

        return s_MethodMap->GetOrAdd(key, ret);
    }

    Il2CppReflectionModule* Reflection::GetModuleObject(const Il2CppImage *image)
    {
        Il2CppReflectionModule *res;
        //char* basename;

        ModuleMap::key_type::wrapped_type key(image, (Il2CppClass*)NULL);
        ModuleMap::data_type value = NULL;

        if (s_ModuleMap->TryGetValue(key, &value))
            return value;

        res = (Il2CppReflectionModule*)Object::New(s_System_Reflection_Module);

        res->image = image;
        IL2CPP_OBJECT_SETREF(res, assembly, Reflection::GetAssemblyObject(image->assembly));

        IL2CPP_OBJECT_SETREF(res, fqname, String::New(image->name));
        IL2CPP_NOT_IMPLEMENTED_ICALL_NO_ASSERT(Reflection::GetModuleObject, "Missing Module fields need set");
        //basename = g_path_get_basename (image->name);
        //IL2CPP_OBJECT_SETREF (res, name, String::New (basename));
        IL2CPP_OBJECT_SETREF(res, name, String::New(image->name));
        IL2CPP_OBJECT_SETREF(res, scopename, String::New(image->nameNoExt));

        //g_free (basename);

        /*if (image->assembly->image == image) {
            res->token = mono_metadata_make_token (MONO_TABLE_MODULE, 1);
        } else {
            int i;
            res->token = 0;
            if (image->assembly->image->modules) {
                for (i = 0; i < image->assembly->image->module_count; i++) {
                    if (image->assembly->image->modules [i] == image)
                        res->token = mono_metadata_make_token (MONO_TABLE_MODULEREF, i + 1);
                }
                IL2CPP_ASSERT(res->token);
            }
        }*/

        return s_ModuleMap->GetOrAdd(key, res);
    }

    Il2CppReflectionProperty* Reflection::GetPropertyObject(Il2CppClass *klass, const PropertyInfo *property)
    {
        Il2CppReflectionProperty *res;

        PropertyMap::key_type::wrapped_type key(property, klass);
        PropertyMap::data_type value = NULL;

        if (s_PropertyMap->TryGetValue(key, &value))
            return value;

        res = (Il2CppReflectionProperty*)Object::New(s_System_Reflection_RuntimePropertyInfoKlass);
        res->klass = klass;
        res->property = property;

        return s_PropertyMap->GetOrAdd(key, res);
    }

    Il2CppReflectionEvent* Reflection::GetEventObject(Il2CppClass* klass, const EventInfo* event)
    {
        Il2CppReflectionEvent* result;

        EventMap::key_type::wrapped_type key(event, klass);
        EventMap::data_type value = NULL;

        if (s_EventMap->TryGetValue(key, &value))
            return value;

        Il2CppReflectionMonoEvent* monoEvent = reinterpret_cast<Il2CppReflectionMonoEvent*>(Object::New(s_System_Reflection_RuntimeEventInfoKlass));
        monoEvent->eventInfo = event;
        monoEvent->reflectedType = Reflection::GetTypeObject(&klass->byval_arg);
        result = reinterpret_cast<Il2CppReflectionEvent*>(monoEvent);

        return s_EventMap->GetOrAdd(key, result);
    }

    Il2CppReflectionType* Reflection::GetTypeObject(const Il2CppType *type)
    {
        Il2CppReflectionType* object = NULL;

        if (s_TypeMap->TryGetValue(type, &object))
            return object;

        Il2CppReflectionType* typeObject = (Il2CppReflectionType*)Object::New(il2cpp_defaults.runtimetype_class);

        typeObject->type = type;

        return s_TypeMap->GetOrAdd(type, typeObject);
    }

    Il2CppObject* Reflection::GetDBNullObject()
    {
        Il2CppObject* valueFieldValue;

        if (!s_DbNullValueField)
        {
            s_DbNullValueField = Class::GetFieldFromName(il2cpp_defaults.dbnull_class, "Value");
            IL2CPP_ASSERT(s_DbNullValueField);
        }

        valueFieldValue = Field::GetValueObject(s_DbNullValueField, NULL);
        IL2CPP_ASSERT(valueFieldValue);
        return valueFieldValue;
    }

    static Il2CppObject* GetReflectionMissingObject()
    {
        Il2CppObject* valueFieldValue;

        if (!s_ReflectionMissingField)
        {
            Il2CppClass* klass = Image::ClassFromName(il2cpp_defaults.corlib, "System.Reflection", "Missing");
            Class::Init(klass);
            s_ReflectionMissingField = Class::GetFieldFromName(klass, "Value");
            IL2CPP_ASSERT(s_ReflectionMissingField);
        }

        valueFieldValue = Field::GetValueObject(s_ReflectionMissingField, NULL);
        IL2CPP_ASSERT(valueFieldValue);
        return valueFieldValue;
    }

    static Il2CppObject* GetObjectForMissingDefaultValue(uint32_t parameterAttributes)
    {
        if (parameterAttributes & PARAM_ATTRIBUTE_OPTIONAL)
            return GetReflectionMissingObject();
        else
            return Reflection::GetDBNullObject();
    }

    Il2CppArray* Reflection::GetParamObjects(const MethodInfo *method, Il2CppClass *refclass)
    {
        Il2CppArray *res = NULL;
        Il2CppReflectionMethod *member = NULL;

        IL2CPP_NOT_IMPLEMENTED_NO_ASSERT(Reflection::GetParamObjects, "Work in progress!");

        if (!method->parameters_count)
            return Array::NewSpecific(s_System_Reflection_ParameterInfo_array, 0);

        // Mono caches based on the address of the method pointer in the MethodInfo
        // since they put everything in one cache and the MethodInfo is already used as key for GetMethodObject caching
        // However, since we have distinct maps for the different types we can use MethodInfo as the key again

        ParametersMap::key_type::wrapped_type key(method, refclass);
        ParametersMap::data_type value;

        if (s_ParametersMap->TryGetValue(key, &value))
            return value;

        member = GetMethodObject(method, refclass);
        res = Array::NewSpecific(s_System_Reflection_ParameterInfo_array, method->parameters_count);
        for (int i = 0; i < method->parameters_count; ++i)
        {
            Il2CppReflectionParameter* param = (Il2CppReflectionParameter*)Object::New(s_System_Reflection_ParameterInfo);
            IL2CPP_OBJECT_SETREF(param, ClassImpl, GetTypeObject(method->parameters[i]));
            IL2CPP_OBJECT_SETREF(param, MemberImpl, (Il2CppObject*)member);
            const char* parameter_name = Method::GetParamName(method, i);
            IL2CPP_OBJECT_SETREF(param, NameImpl, parameter_name ? String::New(parameter_name) : NULL);
            param->PositionImpl = i;
            param->AttrsImpl = method->parameters[i]->attrs;

            Il2CppObject* defaultValue = NULL;
            if (param->AttrsImpl & PARAM_ATTRIBUTE_HAS_DEFAULT)
            {
                bool isExplicitySetNullDefaultValue = false;
                defaultValue = Parameter::GetDefaultParameterValueObject(method, i, &isExplicitySetNullDefaultValue);
                if (defaultValue == NULL && !isExplicitySetNullDefaultValue)
                    defaultValue = GetObjectForMissingDefaultValue(param->AttrsImpl);
            }
            else
            {
                defaultValue = GetObjectForMissingDefaultValue(param->AttrsImpl);
            }

            IL2CPP_OBJECT_SETREF(param, DefaultValueImpl, defaultValue);

            il2cpp_array_setref(res, i, param);
        }

        return s_ParametersMap->GetOrAdd(key, res);
    }

// TODO: move this somewhere else
    bool Reflection::IsType(Il2CppObject *obj)
    {
        return (obj->klass == il2cpp_defaults.runtimetype_class);
    }

    static bool IsMethod(Il2CppObject *obj)
    {
        return obj->klass == s_System_Reflection_MethodInfo;
    }

    static bool IsCMethod(Il2CppObject *obj)
    {
        return obj->klass == s_System_Reflection_ConstructorInfo;
    }

    bool Reflection::IsAnyMethod(Il2CppObject *obj)
    {
        return IsMethod(obj) || IsCMethod(obj);
    }

    bool Reflection::IsField(Il2CppObject *obj)
    {
        return obj->klass == s_System_Reflection_RuntimeFieldInfoKlass;
    }

    bool Reflection::IsProperty(Il2CppObject *obj)
    {
        return obj->klass == s_System_Reflection_RuntimePropertyInfoKlass;
    }

    bool Reflection::IsEvent(Il2CppObject *obj)
    {
        return obj->klass == s_System_Reflection_RuntimeEventInfoKlass;
    }

    static bool IsParameter(Il2CppObject *obj)
    {
        return obj->klass == il2cpp_defaults.parameter_info_class;
    }

    static bool IsAssembly(Il2CppObject *obj)
    {
        return obj->klass == s_System_Reflection_Assembly->klass;
    }

    static std::tuple<uint32_t, const Il2CppImage*> GetMetadataTokenFromReflectionType(Il2CppObject* obj, bool throwOnError = true)
    {
        if (vm::Reflection::IsField(obj))
        {
            Il2CppReflectionField* field = (Il2CppReflectionField*)obj;
            return std::make_tuple(vm::Field::GetToken(field->field), field->field->parent->image);
        }
        if (vm::Reflection::IsAnyMethod(obj))
        {
            Il2CppReflectionMethod* method = (Il2CppReflectionMethod*)obj;
            return std::make_tuple(vm::Method::GetToken(method->method), method->method->klass->image);
        }
        if (vm::Reflection::IsProperty(obj))
        {
            Il2CppReflectionProperty* prop = (Il2CppReflectionProperty*)obj;
            return std::make_tuple(vm::Property::GetToken(prop->property), prop->property->parent->image);
        }
        if (vm::Reflection::IsEvent(obj))
        {
            Il2CppReflectionMonoEvent* eventInfo = (Il2CppReflectionMonoEvent*)obj;
            return std::make_tuple(vm::Event::GetToken(eventInfo->eventInfo), eventInfo->eventInfo->parent->image);
        }
        if (vm::Reflection::IsType(obj))
        {
            Il2CppReflectionType* type = (Il2CppReflectionType*)obj;
            Il2CppClass* klass = vm::Class::FromIl2CppType(type->type);
            return std::make_tuple(klass->token, klass->image);
        }
        if (IsParameter(obj))
        {
            Il2CppReflectionParameter* parameter = (Il2CppReflectionParameter*)obj;
            Il2CppReflectionMethod* method = (Il2CppReflectionMethod*)parameter->MemberImpl;
            const Il2CppImage* image = method->method->klass->image;
#if !SUPPORT_METHOD_RETURN_TYPE_CUSTOM_ATTRIBUTE
            if (parameter->PositionImpl == -1)
                return std::make_tuple(0x8000000, method->method->klass->image); // This is what mono returns as a fixed value.

#endif
            return std::make_tuple(vm::Method::GetParameterToken(method->method, parameter->PositionImpl), method->method->klass->image);
        }
        if (IsAssembly(obj))
        {
            Il2CppReflectionAssembly* assembly = (Il2CppReflectionAssembly*)obj;
            return std::make_tuple(assembly->assembly->token, assembly->assembly->image);
        }

        if (throwOnError)
            NOT_SUPPORTED_IL2CPP(MemberInfo::get_MetadataToken, "This icall is not supported by il2cpp.");
        return std::make_tuple(0, (const Il2CppImage*)nullptr);
    }

    int Reflection::GetMetadataToken(Il2CppObject* obj)
    {
        uint32_t token;
        const Il2CppImage* image;
        std::tie(token, image) = GetMetadataTokenFromReflectionType(obj);
        return (int)token;
    }

    bool Reflection::HasAttribute(Il2CppReflectionParameter *parameter, Il2CppClass* attributeClass)
    {
        Il2CppReflectionMethod* method = (Il2CppReflectionMethod*)parameter->MemberImpl;

        if (method->method->parameters == NULL)
            return false;

        IL2CPP_NOT_IMPLEMENTED_NO_ASSERT(Reflection::GetCustomAttributeTypeCacheFor, "-1 represents the return value. Need to emit custom attribute information for that.")
        if (parameter->PositionImpl == -1)
            return false;

        const MethodInfo* methodWithParameterAttributeInformation = method->method;
        if (method->method->is_inflated)
            methodWithParameterAttributeInformation = method->method->genericMethod->methodDefinition;

        auto reader = il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(methodWithParameterAttributeInformation->klass->image, Method::GetParameterToken(method->method, parameter->PositionImpl));
        return HasAttribute(reader, attributeClass);
    }

    Il2CppArray* Reflection::GetCustomAttrsInfo(Il2CppObject* obj, Il2CppClass* attributeClass)
    {
        il2cpp::metadata::CustomAttributeDataReader reader = GetCustomAttrsDataReader(obj);
        return GetCustomAttrsInfo(reader, attributeClass);
    }

    Il2CppArray* Reflection::GetCustomAttrsInfo(const il2cpp::metadata::CustomAttributeDataReader& reader, Il2CppClass* attributeClass)
    {
        if (reader.GetCount() == 0)
            return NULL;

        auto filter = GetFilter(attributeClass);

        uint32_t attributeCount = reader.GetCount(filter);

        if (attributeCount == 0)
            return NULL;

        Il2CppArray* attrArray = il2cpp::vm::Array::New(il2cpp_defaults.attribute_class, attributeCount);

        il2cpp::metadata::CustomAttributeDataIterator iter = reader.GetDataIterator(filter);
        for (uint32_t i = 0; i < attributeCount; i++)
        {
            Il2CppException* exc = NULL;
            il2cpp::metadata::CustomAttributeCreator creator;
            if (reader.VisitCustomAttributeData(&iter, &creator, &exc))
            {
                il2cpp_array_setref(attrArray, i, creator.GetAttribute(&exc));
                if (exc != NULL)
                    il2cpp::vm::Exception::Raise(exc);
            }

            if (exc != NULL)
                il2cpp::vm::Exception::Raise(exc);
        }

        return attrArray;
    }

    il2cpp::metadata::CustomAttributeDataReader Reflection::GetCustomAttrsDataReader(Il2CppObject* obj)
    {
        uint32_t token;
        const Il2CppImage* image;
        std::tie(token, image) = GetMetadataTokenFromReflectionType(obj, false);
        if (token == 0)
        {
            // obj is not a reflection type, reflect on obj's class
            token = obj->klass->token;
            image = obj->klass->image;
        }

        if ((token & IL2CPP_TOKEN_PARAM_DEF) == IL2CPP_TOKEN_PARAM_DEF)
        {
            Il2CppReflectionParameter* parameter = (Il2CppReflectionParameter*)obj;
            Il2CppReflectionMethod* method = (Il2CppReflectionMethod*)parameter->MemberImpl;

            if (method->method->parameters == NULL)
                return il2cpp::metadata::CustomAttributeDataReader::Empty();
#if !SUPPORT_METHOD_RETURN_TYPE_CUSTOM_ATTRIBUTE
            IL2CPP_NOT_IMPLEMENTED_NO_ASSERT(Reflection::GetCustomAttributeReaderFor, "-1 represents the return value. Need to emit custom attribute information for that.")
            if (parameter->PositionImpl == -1)
                return il2cpp::metadata::CustomAttributeDataReader::Empty();
#endif
        }

        return il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(image, token);
    }

    ReflectionObjInfo Reflection::GetImageOfReflectionObject(Il2CppObject* obj)
    {
        if (il2cpp::vm::Reflection::IsAnyMethod(obj))
        {
            const MethodInfo* method = ((Il2CppReflectionMethod*)obj)->method;
            return { method->klass->image, method->token };
        }
        else if (il2cpp::vm::Reflection::IsProperty(obj))
        {
            const PropertyInfo* prop = ((Il2CppReflectionProperty*)obj)->property;
            return { prop->parent->image, prop->token };
        }
        else if (il2cpp::vm::Reflection::IsField(obj))
        {
            FieldInfo* field = ((Il2CppReflectionField*)obj)->field;
            return { field->parent->image, field->token };
        }
        else if (il2cpp::vm::Reflection::IsEvent(obj))
        {
            const EventInfo* eventInfo = ((Il2CppReflectionMonoEvent*)obj)->eventInfo;
            return { eventInfo->parent->image, eventInfo->token };
        }
        else if (IsParameter(obj))
        {
            Il2CppReflectionParameter* parameter = (Il2CppReflectionParameter*)obj;
            Il2CppReflectionMethod* method = (Il2CppReflectionMethod*)parameter->MemberImpl;
            return { method->method->klass->image, il2cpp::vm::Method::GetParameterToken(method->method, parameter->PositionImpl) };
        }
        else if (IsAssembly(obj))
        {
            const Il2CppAssembly* assembly = ((Il2CppReflectionAssembly*)obj)->assembly;
            return { assembly->image, assembly->token };
        }
        else
        {
            Il2CppClass* klass = il2cpp::vm::Reflection::IsType(obj)
                ? il2cpp::vm::Class::FromSystemType((Il2CppReflectionType*)obj)
                : obj->klass;

            return { klass->image, klass->token };
        }
    }

    bool Reflection::HasAttribute(Il2CppObject *obj, Il2CppClass* attributeClass)
    {
        il2cpp::metadata::CustomAttributeDataReader reader = GetCustomAttrsDataReader(obj);
        return HasAttribute(reader, attributeClass);
    }

    bool Reflection::HasAttribute(const il2cpp::metadata::CustomAttributeDataReader& reader, Il2CppClass* attributeClass)
    {
        if (reader.GetCount() == 0)
            return false;

        auto filter = GetFilter(attributeClass);
        auto ctorIter = reader.GetCtorIterator(filter);
        const MethodInfo* ctor;
        return reader.IterateAttributeCtors(&ctor, &ctorIter);
    }

    Il2CppObject* Reflection::GetCustomAttribute(Il2CppMetadataCustomAttributeHandle token, Il2CppClass* attributeClass)
    {
        auto reader = il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(token);

        if (reader.GetCount() == 0)
            return NULL;

        auto filter = GetFilter(attributeClass);

        if (reader.GetCount(filter) == 0)
            return NULL;

        Il2CppObject* attr = NULL;
        il2cpp::metadata::CustomAttributeDataIterator iter = reader.GetDataIterator(filter);
        Il2CppException* exc = NULL;
        il2cpp::metadata::CustomAttributeCreator creator;
        if (reader.VisitCustomAttributeData(&iter, &creator, &exc))
        {
            if (exc == NULL)
                attr = creator.GetAttribute(&exc);
        }

        if (exc != NULL)
            il2cpp::vm::Exception::Raise(exc);

        return attr;
    }

    Il2CppArray* Reflection::ConstructCustomAttributes(Il2CppMetadataCustomAttributeHandle token)
    {
        auto reader = il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(token);
        return GetCustomAttrsInfo(reader, NULL);
    }

    void Reflection::Initialize()
    {
        s_AssemblyMap = new AssemblyMap();
        s_FieldMap = new FieldMap();
        s_PropertyMap = new PropertyMap();
        s_EventMap = new EventMap();
        s_MethodMap = new MethodMap();
        s_ModuleMap = new ModuleMap();
        s_ParametersMap = new ParametersMap();
        s_TypeMap = new TypeMap();
        s_MonoGenericParamterMap = new MonoGenericParameterMap();
        s_MonoAssemblyNameMap = new MonoAssemblyNameMap();

        s_System_Reflection_Assembly = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimeAssembly");
        IL2CPP_ASSERT(s_System_Reflection_Assembly != NULL);
        s_System_Reflection_Module = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimeModule");
        IL2CPP_ASSERT(s_System_Reflection_Module != NULL);

        s_System_Reflection_ConstructorInfo = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimeConstructorInfo");
        IL2CPP_ASSERT(s_System_Reflection_ConstructorInfo != NULL);
        s_System_Reflection_MethodInfo = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimeMethodInfo");
        IL2CPP_ASSERT(s_System_Reflection_MethodInfo != NULL);
        s_System_Reflection_ParameterInfo = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimeParameterInfo");
        IL2CPP_ASSERT(s_System_Reflection_ParameterInfo != NULL);
        s_System_Reflection_ParameterInfo_array = Class::GetArrayClass(s_System_Reflection_ParameterInfo, 1);
        IL2CPP_ASSERT(s_System_Reflection_ParameterInfo_array != NULL);

        s_System_Reflection_RuntimeFieldInfoKlass = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimeFieldInfo");
        IL2CPP_ASSERT(s_System_Reflection_RuntimeFieldInfoKlass != NULL);
        s_System_Reflection_RuntimeEventInfoKlass = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimeEventInfo");
        IL2CPP_ASSERT(s_System_Reflection_RuntimeEventInfoKlass != NULL);
        s_System_Reflection_RuntimePropertyInfoKlass = Class::FromName(il2cpp_defaults.corlib, "System.Reflection", "RuntimePropertyInfo");
        IL2CPP_ASSERT(s_System_Reflection_RuntimePropertyInfoKlass != NULL);
    }

    bool Reflection::HasAttribute(FieldInfo *field, Il2CppClass *attributeClass)
    {
        auto reader = il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(field->parent->image, Field::GetToken(field));
        return HasAttribute(reader, attributeClass);
    }

    bool Reflection::HasAttribute(const MethodInfo *method, Il2CppClass *attributeClass)
    {
        auto reader = il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(method->klass->image, Method::GetToken(method));
        return HasAttribute(reader, attributeClass);
    }

    bool Reflection::HasAttribute(Il2CppClass *klass, Il2CppClass *attributeClass)
    {
        auto reader = il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(klass->image, klass->token);
        return HasAttribute(reader, attributeClass);
    }

    bool Reflection::HasAttribute(Il2CppMetadataCustomAttributeHandle handle, Il2CppClass *attributeClass)
    {
        auto reader = il2cpp::vm::MetadataCache::GetCustomAttributeDataReader(handle);
        return HasAttribute(reader, attributeClass);
    }

    Il2CppClass* Reflection::TypeGetHandle(Il2CppReflectionType* ref)
    {
        if (!ref)
            return NULL;

        return Class::FromSystemType(ref);
    }

    const MonoGenericParameterInfo* Reflection::GetMonoGenericParameterInfo(Il2CppMetadataGenericParameterHandle param)
    {
        MonoGenericParameterMap::const_iterator it = s_MonoGenericParamterMap->find(param);
        if (it == s_MonoGenericParamterMap->end())
            return NULL;

        return it->second;
    }

    void Reflection::SetMonoGenericParameterInfo(Il2CppMetadataGenericParameterHandle param, const MonoGenericParameterInfo *monoParam)
    {
        s_MonoGenericParamterMap->insert(std::make_pair(param, monoParam));
    }

    const Il2CppMonoAssemblyName* Reflection::GetMonoAssemblyName(const Il2CppAssembly *assembly)
    {
        MonoAssemblyNameMap::const_iterator it = s_MonoAssemblyNameMap->find(assembly);
        if (it == s_MonoAssemblyNameMap->end())
            return NULL;

        return it->second;
    }

    void Reflection::SetMonoAssemblyName(const Il2CppAssembly *assembly, const Il2CppMonoAssemblyName *aname)
    {
        s_MonoAssemblyNameMap->insert(std::make_pair(assembly, aname));
    }

    void Reflection::ClearStatics()
    {
        delete s_AssemblyMap;
        s_AssemblyMap = NULL;
        delete s_FieldMap;
        s_FieldMap = NULL;
        delete s_PropertyMap;
        s_PropertyMap = NULL;
        delete s_EventMap;
        s_EventMap = NULL;
        delete s_MethodMap;
        s_MethodMap = NULL;
        delete s_ModuleMap;
        s_ModuleMap = NULL;
        delete s_ParametersMap;
        s_ParametersMap = NULL;
        delete s_TypeMap;
        s_TypeMap = NULL;
        delete s_MonoGenericParamterMap;
        s_MonoGenericParamterMap = NULL;
        delete s_MonoAssemblyNameMap;
        s_MonoAssemblyNameMap = NULL;

        s_System_Reflection_Assembly = NULL;
        s_System_Reflection_RuntimeFieldInfoKlass = NULL;
        s_System_Reflection_Module = NULL;
        s_System_Reflection_RuntimePropertyInfoKlass = NULL;
        s_System_Reflection_RuntimeEventInfoKlass = NULL;
        s_DbNullValueField = NULL;
        s_ReflectionMissingField = NULL;
        s_System_Reflection_ParameterInfo = NULL;
        s_System_Reflection_ParameterInfo_array = NULL;

        s_System_Reflection_MethodInfo = NULL;
        s_System_Reflection_ConstructorInfo = NULL;
    }
} /* namespace vm */
} /* namespace il2cpp */
