#pragma once

#include <vector>
#include <stdint.h>
#include "il2cpp-config.h"
#include "il2cpp-metadata.h"
#include "metadata/CustomAttributeDataReader.h"

struct Il2CppString;
struct Il2CppArray;
struct Il2CppReflectionAssembly;
struct Il2CppReflectionAssemblyName;
struct Il2CppReflectionField;
struct Il2CppReflectionMethod;
struct Il2CppReflectionModule;
struct Il2CppReflectionProperty;
struct Il2CppReflectionEvent;
struct Il2CppReflectionType;
struct Il2CppReflectionParameter;
struct Il2CppClass;
struct FieldInfo;
struct MethodInfo;
struct PropertyInfo;
struct EventInfo;
struct Il2CppClass;
struct Il2CppArray;
struct CustomAttributeTypeCache;
struct Il2CppAssembly;
struct Il2CppAssemblyName;
struct Il2CppImage;
struct Il2CppType;
struct Il2CppObject;
struct MonoGenericParameterInfo;
struct Il2CppMonoAssemblyName;

namespace il2cpp
{
namespace vm
{

    struct ReflectionObjInfo
    {
        const Il2CppImage* image;
        uint32_t token;
    };

    class LIBIL2CPP_CODEGEN_API Reflection
    {
// exported
    public:
        static Il2CppReflectionAssembly* GetAssemblyObject(const Il2CppAssembly *assembly);
        static Il2CppReflectionAssemblyName* GetAssemblyNameObject(const Il2CppAssemblyName *assemblyName);
        static Il2CppReflectionField* GetFieldObject(Il2CppClass *klass, FieldInfo *field);
        static const FieldInfo* GetField(const Il2CppReflectionField* field);
        static Il2CppReflectionProperty* GetPropertyObject(Il2CppClass *klass, const PropertyInfo *property);
        static Il2CppReflectionEvent* GetEventObject(Il2CppClass *klass, const EventInfo *event);
        static Il2CppReflectionMethod* GetMethodObject(const MethodInfo *method, Il2CppClass *refclass);
        static const MethodInfo* GetMethod(const Il2CppReflectionMethod* method);
        static Il2CppReflectionModule* GetModuleObject(const Il2CppImage *image);
        static Il2CppReflectionType* GetTypeObject(const Il2CppType *type);
        static Il2CppArray* GetParamObjects(const MethodInfo *method, Il2CppClass *refclass);
        static Il2CppArray* GetCustomAttrsInfo(Il2CppObject *obj, Il2CppClass* attributeClass);
        static metadata::CustomAttributeDataReader GetCustomAttrsDataReader(Il2CppObject *obj);
        static const MonoGenericParameterInfo* GetMonoGenericParameterInfo(Il2CppMetadataGenericParameterHandle param);
        static void SetMonoGenericParameterInfo(Il2CppMetadataGenericParameterHandle param, const MonoGenericParameterInfo *monoParam);
        static const Il2CppMonoAssemblyName* GetMonoAssemblyName(const Il2CppAssembly *assembly);
        static void SetMonoAssemblyName(const Il2CppAssembly *assembly, const Il2CppMonoAssemblyName *aname);
        static int GetMetadataToken(Il2CppObject* obj);

        static bool HasAttribute(Il2CppObject *obj, Il2CppClass *attributeClass);
        static bool HasAttribute(FieldInfo *field, Il2CppClass *attributeClass);
        static bool HasAttribute(const MethodInfo *method, Il2CppClass *attributeClass);
        static bool HasAttribute(Il2CppClass *klass, Il2CppClass *attributeClass);
        static bool HasAttribute(Il2CppMetadataCustomAttributeHandle handle, Il2CppClass *attributeClass);

        static bool IsType(Il2CppObject *obj);
        static bool IsField(Il2CppObject *obj);
        static bool IsAnyMethod(Il2CppObject *obj);
        static bool IsProperty(Il2CppObject *obj);
        static bool IsEvent(Il2CppObject *obj);

        static void ClearStatics();

        static ReflectionObjInfo GetImageOfReflectionObject(Il2CppObject* obj);
// internal
    public:
        static void Initialize();
        static Il2CppClass* TypeGetHandle(Il2CppReflectionType* ref);
        static Il2CppObject* GetDBNullObject();

        static Il2CppObject* GetCustomAttribute(Il2CppMetadataCustomAttributeHandle token, Il2CppClass* attributeClass);
        static Il2CppArray* ConstructCustomAttributes(Il2CppMetadataCustomAttributeHandle token);

    private:
        static bool HasAttribute(Il2CppReflectionParameter *parameter, Il2CppClass* attributeClass);
        static bool HasAttribute(const il2cpp::metadata::CustomAttributeDataReader& reader, Il2CppClass* attributeClass);
        static Il2CppArray* GetCustomAttrsInfo(const il2cpp::metadata::CustomAttributeDataReader& reader, Il2CppClass* attributeClass);
    };
} /* namespace vm */
} /* namespace il2cpp */
