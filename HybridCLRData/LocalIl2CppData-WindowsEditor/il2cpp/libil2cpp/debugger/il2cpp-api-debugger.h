#pragma once

#include "il2cpp-config.h"

// This header defines an extension to the IL2CPP embedding API that the debugger code requires.
// It should not include any Mono headers.

#include "il2cpp-api.h"

// These macros were used to build the debugger agent for the now unused Mono runtime backend.
// We don't have tha backend any more, but we will keep these macros to avoid code churn.
#define VM_DOMAIN_GET_AGENT_INFO(domain) il2cpp_domain_get_agent_info(domain)
#define VM_DOMAIN_SET_AGENT_INFO(domain, value) il2cpp_domain_set_agent_info(domain, value)
#define VM_DOMAIN_GET_FRIENDLY_NAME(domain) il2cpp_domain_get_friendly_name(domain)
#define VM_METHOD_IS_STRING_CTOR(method) il2cpp_method_is_string_ctor(method)
#define VM_INFLATED_METHOD_GET_DECLARING(imethod) il2cpp_method_get_generic_definition(imethod)
#define VM_INFLATED_METHOD_GET_CLASS_INST(imethod) il2cpp_method_get_generic_class_inst(imethod)
#define VM_OBJECT_GET_DOMAIN(object) mono_domain_get()
#define VM_GENERIC_CLASS_GET_CONTAINER_CLASS(gklass) il2cpp_generic_class_get_container_class(gklass)
#define VM_DEFAULTS_OBJECT_CLASS il2cpp_defaults_object_class()
#define VM_DEFAULTS_EXCEPTION_CLASS il2cpp_defaults_exception_class()
#define VM_DEFAULTS_CORLIB_IMAGE il2cpp_defaults_corlib_image()
#define VM_DEFAULTS_VOID_CLASS il2cpp_defaults_void_class()
#define VM_DEFAULTS_RUNTIMETYPE_CLASS il2cpp_defaults_runtimetype_class()
#define VM_GENERIC_INST_TYPE_ARGC(inst) il2cpp_generic_inst_get_argc(inst)
#define VM_GENERIC_INST_TYPE_ARGV(inst, index) il2cpp_generic_inst_get_argv(inst, index)
#define VM_IMAGE_GET_MODULE_NAME(image) il2cpp_image_get_name(image)
#define VM_ASSEMBLY_IS_DYNAMIC(assembly) 0
#define VM_CLASS_IS_INTERFACE(c) ((il2cpp_class_get_flags (c) & TYPE_ATTRIBUTE_INTERFACE) || mono_type_is_generic_parameter (il2cpp_class_get_byval_arg (c)))

// These types are used in debugger-agent.c for field access
// (not via an API), so we need to map them to the IL2CPP types
// so that the field layout is correct.
#define MonoArray Il2CppArraySize
#define MonoAssembly Il2CppAssembly
#define MonoGenericContainer Il2CppGenericContainer
#define MonoInternalThread Il2CppInternalThread
#define MonoMethod MethodInfo
#define MonoObject Il2CppObject
#define MonoThread Il2CppThread
#define MonoType Il2CppType

typedef struct Il2CppInternalThread Il2CppInternalThread;

// These defines map objects that the debugger-agent.c code uses directly to there
// IL2CPP counterparts.
#define debug_options il2cpp_mono_debug_options
#define mono_defaults il2cpp_defaults

// These are some defines from Mono that the debugger-agent.c code uses.
#define MONO_MAX_IREGS 1
#define NOT_IMPLEMENTED do { g_assert_not_reached (); } while (0)

#if IL2CPP_COMPILER_MSVC
typedef int32_t (*MonoSuspendThreadCallback) (MonoThreadInfo *info, void* user_data);
#else
#if defined(__cplusplus)
enum SuspendThreadResult : int32_t;
#endif
typedef SuspendThreadResult (*MonoSuspendThreadCallback) (MonoThreadInfo *info, void* user_data);
#endif

// This much match the layout of Mono's CattrNamedArg
typedef struct
{
    const Il2CppType *type;
    const FieldInfo *field;
    const PropertyInfo *prop;
} Il2CppCattrNamedArg;

typedef struct
{
    const MonoMethod* ctor;
    Il2CppArray* typedArgs;
    Il2CppArray* namedArgs;
    Il2CppCattrNamedArg* argInfo;
} Il2CppCustomAttributeData;

typedef struct
{
    int32_t numberOfAttributes;
    Il2CppCustomAttributeData attributeData[];
} Il2CppCustomAttributeDataList;

#if defined(__cplusplus)
extern "C" {
#endif
// These functions expose the IL2CPP C++ API to C for the debugger using Mono's types.
void il2cpp_start_debugger_thread();
Il2CppSequencePoint* il2cpp_get_method_sequence_points(MonoMethod* method, void* *iter);
MonoClass* il2cpp_defaults_object_class();
uint8_t* il2cpp_field_get_address(MonoObject *obj, MonoClassField *monoField);
MonoClass* il2cpp_defaults_exception_class();
MonoImage* il2cpp_defaults_corlib_image();
bool il2cpp_method_is_string_ctor(const MonoMethod * method);
MonoClass* il2cpp_defaults_void_class();
MonoClass* il2cpp_defaults_runtimetype_class();
MonoMethod* il2cpp_get_interface_method(MonoClass* klass, MonoClass* itf, int slot);
int32_t il2cpp_field_is_deleted(MonoClassField *field);
MonoClass* il2cpp_iterate_loaded_classes(void* *iter);
const char** il2cpp_get_source_files_for_type(MonoClass *klass, int *count);
MonoMethod* il2cpp_method_get_generic_definition(MonoMethodInflated *imethod);
MonoGenericInst* il2cpp_method_get_generic_class_inst(MonoMethodInflated *imethod);
MonoClass* il2cpp_generic_class_get_container_class(MonoGenericClass *gclass);
Il2CppSequencePoint* il2cpp_get_sequence_point(MonoImage* image, int id);
char* il2cpp_assembly_get_full_name(MonoAssembly *assembly);
const MonoMethod* il2cpp_get_seq_point_method(Il2CppSequencePoint *seqPoint);
const MonoClass* il2cpp_get_class_from_index(int index);
const MonoType* il2cpp_get_type_from_method_context(MonoType* type, const MonoMethod* method);
const MonoType* il2cpp_type_inflate(MonoType* type, const MonoGenericContext* context);
void il2cpp_debugger_get_method_execution_context_and_header_info(const MonoMethod* method, uint32_t* executionContextInfoCount, const Il2CppMethodExecutionContextInfo **executionContextInfo, const Il2CppMethodHeaderInfo **headerInfo, const Il2CppMethodScope **scopes);
Il2CppThreadUnwindState* il2cpp_debugger_get_thread_context();
const MonoAssembly* il2cpp_m_method_get_assembly(MonoMethod* method);
Il2CppSequencePointSourceFile* il2cpp_debug_get_source_file(MonoImage* image, int index);
Il2CppCatchPoint* il2cpp_get_method_catch_points(MonoMethod* method, void* *iter);
Il2CppSequencePoint* il2cpp_get_seq_point_from_catch_point(Il2CppCatchPoint *cp);
MonoMethod* il2cpp_get_generic_method_definition(MonoMethod* method);
bool il2cpp_class_is_initialized(MonoClass* klass);
void* il2cpp_domain_get_agent_info(MonoAppDomain* domain);
void il2cpp_domain_set_agent_info(MonoAppDomain* domain, void* agentInfo);
const char* il2cpp_domain_get_friendly_name(MonoAppDomain* domain);
int il2cpp_generic_inst_get_argc(MonoGenericInst * inst);
MonoType* il2cpp_generic_inst_get_argv(MonoGenericInst * inst, int index);
MonoObject* il2cpp_assembly_get_object(MonoDomain * domain, MonoAssembly * assembly, MonoError * error);
const MonoType* il2cpp_get_type_from_index(int index);
void il2cpp_thread_info_safe_suspend_and_run(size_t id, int32_t interrupt_kernel, MonoSuspendThreadCallback callback, void* user_data);
MonoGenericParam* il2cpp_generic_container_get_param(MonoGenericContainer * gc, int i);
int32_t il2cpp_mono_methods_match(MonoMethod* left, MonoMethod* right);
void il2cpp_field_static_get_value_checked(MonoVTable * vt, MonoClassField * field, void* value, MonoError * error);
void il2cpp_field_static_get_value_for_thread(MonoInternalThread * thread, MonoVTable * vt, MonoClassField * field, void* value, MonoError * error);
const Il2CppCustomAttributeDataList* il2cpp_get_custom_attribute_data_list(MonoClass* attr_klass, MonoCustomAttrInfo* cinfo, MonoImage* image);
void il2cpp_free_custom_attribute_data_list(Il2CppCustomAttributeDataList* data);
int32_t il2cpp_field_get_fixed_array_size(MonoClassField* field);
MonoType* il2cpp_class_get_byval_arg(MonoClass* klass);
#if defined(__cplusplus)
}
#endif
