#include "il2cpp-config.h"
#include "vm/Property.h"
#include "il2cpp-class-internals.h"

namespace il2cpp
{
namespace vm
{
    uint32_t Property::GetFlags(const PropertyInfo* prop)
    {
        return prop->attrs;
    }

    const MethodInfo* Property::GetGetMethod(const PropertyInfo* prop)
    {
        return prop->get;
    }

    const MethodInfo* Property::GetSetMethod(const PropertyInfo* prop)
    {
        return prop->set;
    }

    const char* Property::GetName(const PropertyInfo* prop)
    {
        return prop->name;
    }

    Il2CppClass* Property::GetParent(const PropertyInfo* prop)
    {
        return prop->parent;
    }

    uint32_t Property::GetToken(const PropertyInfo* prop)
    {
        return prop->token;
    }

    const Il2CppType* Property::GetType(const PropertyInfo* prop)
    {
        if (prop->get)
            return prop->get->return_type;

        return prop->set->parameters[prop->set->parameters_count - 1];
    }
} /* namespace vm */
} /* namespace il2cpp */
