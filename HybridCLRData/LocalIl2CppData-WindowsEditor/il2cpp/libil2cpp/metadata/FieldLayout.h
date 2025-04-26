#pragma once

#include <stdint.h>
#include <vector>

typedef bool (FieldInfoFilter)(FieldInfo*);

namespace il2cpp
{
namespace metadata
{
    struct SizeAndAlignment
    {
        size_t size;
        uint8_t alignment;
    };

    class FieldLayout
    {
    public:
        struct FieldLayoutData
        {
            std::vector<size_t> FieldOffsets;
            size_t classSize;
            size_t actualClassSize;
            uint8_t minimumAlignment;
        };

        static void LayoutInstanceFields(const Il2CppClass* klass, size_t parentSize, size_t actualParentSize, size_t parentAlignment, uint8_t packing, FieldLayoutData& data);
        static void LayoutStaticFields(const Il2CppClass* klass, FieldLayoutData& data);
        static void LayoutThreadStaticFields(const Il2CppClass* klass, FieldLayoutData& data);

        static SizeAndAlignment GetTypeSizeAndAlignment(const Il2CppType* type);

    private:
        static void LayoutFields(const Il2CppClass* klass, FieldInfoFilter filter, size_t parentSize, size_t actualParentSize, size_t parentAlignment, uint8_t packing, bool isExplicitLayout, FieldLayoutData& data);
    };
} /* namespace metadata */
} /* namespace il2cpp */
