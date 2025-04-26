#include "il2cpp-config.h"
#include "il2cpp-class-internals.h"
#include "vm/Class.h"
#include "vm/GenericClass.h"
#include "vm/GlobalMetadata.h"
#include "vm/Type.h"
#include "vm/Field.h"
#include "metadata/FieldLayout.h"
#include <limits>
#include <algorithm>

using il2cpp::vm::Class;
using il2cpp::vm::GenericClass;
using il2cpp::vm::Type;

namespace il2cpp
{
namespace metadata
{
    typedef void* voidptr_t;
#define IL2CPP_ALIGN_STRUCT(type) struct type ## AlignStruct {uint8_t pad; type t; };

    IL2CPP_ALIGN_STRUCT(voidptr_t)
    IL2CPP_ALIGN_STRUCT(int8_t)
    IL2CPP_ALIGN_STRUCT(int16_t)
    IL2CPP_ALIGN_STRUCT(int32_t)
    IL2CPP_ALIGN_STRUCT(int64_t)
    IL2CPP_ALIGN_STRUCT(intptr_t)
    IL2CPP_ALIGN_STRUCT(float)
    IL2CPP_ALIGN_STRUCT(double)

#define IL2CPP_ALIGN_OF(type) ((size_t)offsetof(type ## AlignStruct, t))

    SizeAndAlignment FieldLayout::GetTypeSizeAndAlignment(const Il2CppType* type)
    {
        SizeAndAlignment sa = { 0 };
        if (type->byref)
        {
            sa.size = sizeof(voidptr_t);
            sa.alignment = IL2CPP_ALIGN_OF(voidptr_t);
            return sa;
        }

        switch (type->type)
        {
            case IL2CPP_TYPE_I1:
            case IL2CPP_TYPE_U1:
            case IL2CPP_TYPE_BOOLEAN:
                sa.size = sizeof(int8_t);
                sa.alignment = IL2CPP_ALIGN_OF(int8_t);
                return sa;
            case IL2CPP_TYPE_I2:
            case IL2CPP_TYPE_U2:
            case IL2CPP_TYPE_CHAR:
                sa.size = sizeof(int16_t);
                sa.alignment = IL2CPP_ALIGN_OF(int16_t);
                return sa;
            case IL2CPP_TYPE_I4:
            case IL2CPP_TYPE_U4:
                sa.size = sizeof(int32_t);
                sa.alignment = IL2CPP_ALIGN_OF(int32_t);
                return sa;
            case IL2CPP_TYPE_I8:
            case IL2CPP_TYPE_U8:
                sa.size = sizeof(int64_t);
                sa.alignment = IL2CPP_ALIGN_OF(int64_t);
                return sa;
            case IL2CPP_TYPE_I:
            case IL2CPP_TYPE_U:
                // TODO should we use pointer or size_t here?
                sa.size = sizeof(intptr_t);
                sa.alignment = IL2CPP_ALIGN_OF(intptr_t);
                return sa;
            case IL2CPP_TYPE_R4:
                sa.size = sizeof(float);
                sa.alignment = IL2CPP_ALIGN_OF(float);
                return sa;
            case IL2CPP_TYPE_R8:
                sa.size = sizeof(double);
                sa.alignment = IL2CPP_ALIGN_OF(double);
                return sa;
            case IL2CPP_TYPE_PTR:
            case IL2CPP_TYPE_FNPTR:
            case IL2CPP_TYPE_STRING:
            case IL2CPP_TYPE_SZARRAY:
            case IL2CPP_TYPE_ARRAY:
            case IL2CPP_TYPE_CLASS:
            case IL2CPP_TYPE_OBJECT:
            case IL2CPP_TYPE_VAR:
            case IL2CPP_TYPE_MVAR:
                sa.size = sizeof(voidptr_t);
                sa.alignment = IL2CPP_ALIGN_OF(voidptr_t);
                return sa;
            case IL2CPP_TYPE_VALUETYPE:
                if (Type::IsEnum(type))
                {
                    return GetTypeSizeAndAlignment(Class::GetEnumBaseType(Type::GetClass(type)));
                }
                else
                {
                    uint32_t alignment;
                    Il2CppClass* klass = Type::GetClass(type);
                    sa.size = Class::GetValueSize(klass, &alignment);
                    sa.alignment = alignment;
                    return sa;
                }
            case IL2CPP_TYPE_GENERICINST:
            {
                Il2CppGenericClass* gclass = type->data.generic_class;
                Il2CppClass* container_class = GenericClass::GetTypeDefinition(gclass);

                if (container_class != NULL && container_class->byval_arg.valuetype)
                {
                    if (container_class->enumtype)
                    {
                        return GetTypeSizeAndAlignment(Class::GetEnumBaseType(container_class));
                    }
                    else
                    {
                        uint32_t alignment;
                        sa.size = Class::GetValueSize(Class::FromIl2CppType(type), &alignment);
                        sa.alignment = alignment;
                        return sa;
                    }
                }
                else
                {
                    sa.size = sizeof(voidptr_t);
                    sa.alignment = IL2CPP_ALIGN_OF(voidptr_t);
                    return sa;
                }
            }
            default:
                IL2CPP_ASSERT(0);
                break;
        }
        return sa;
    }

    static size_t AlignTo(size_t size, size_t alignment)
    {
        if (size & (alignment - 1))
        {
            size += alignment - 1;
            size &= ~(alignment - 1);
        }

        return size;
    }

    void FieldLayout::LayoutInstanceFields(const Il2CppClass* klass, size_t parentSize, size_t actualParentSize, size_t parentAlignment, uint8_t packing, FieldLayoutData& data)
    {
        LayoutFields(klass, vm::Field::IsInstance, parentSize, actualParentSize, parentAlignment, packing, klass->flags & TYPE_ATTRIBUTE_EXPLICIT_LAYOUT && klass->generic_class == NULL, data);
    }

    void FieldLayout::LayoutStaticFields(const Il2CppClass* klass, FieldLayoutData& data)
    {
        LayoutFields(klass, vm::Field::IsNormalStatic, 0, 0, 1, 0, false, data);
    }

    void FieldLayout::LayoutThreadStaticFields(const Il2CppClass* klass, FieldLayoutData& data)
    {
        LayoutFields(klass, vm::Field::IsThreadStatic, 0, 0, 1, 0, false, data);
    }

    void FieldLayout::LayoutFields(const Il2CppClass* klass, FieldInfoFilter filter, size_t parentSize, size_t actualParentSize, size_t parentAlignment, uint8_t packing, bool isExplicitLayout, FieldLayoutData& data)
    {
        data.classSize = parentSize;
        data.actualClassSize = actualParentSize;
        IL2CPP_ASSERT(parentAlignment <= std::numeric_limits<uint8_t>::max());
        data.minimumAlignment = static_cast<uint8_t>(parentAlignment);
        for (uint16_t i = 0; i < klass->field_count; i++)
        {
            if (!filter(klass->fields + i))
                continue;

            SizeAndAlignment sa = GetTypeSizeAndAlignment(klass->fields[i].type);
            uint8_t alignment = sa.alignment;
            if (packing > 0)
                alignment = std::min(sa.alignment, packing);
            size_t offset = isExplicitLayout ? klass->fields[i].offset : data.actualClassSize;

            offset += alignment - 1;
            offset &= ~(alignment - 1);

            data.FieldOffsets.push_back(offset);
            data.actualClassSize = std::max(data.actualClassSize, offset + std::max(sa.size, (size_t)1));
            data.minimumAlignment = std::max(data.minimumAlignment, alignment);
        }

        data.classSize = AlignTo(data.actualClassSize, data.minimumAlignment);

        // C++ ABI difference between MS and Clang
#if IL2CPP_CXX_ABI_MSVC
        data.actualClassSize = data.classSize;
#endif
    }
} /* namespace vm */
} /* namespace il2cpp */
