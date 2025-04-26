#pragma once

#include <stdint.h>
#include <functional>
#include "il2cpp-object-internals.h"
#include "gc/Allocator.h"

namespace il2cpp
{
namespace metadata
{
    // This union is large enough to store any type
    // that can be serialized by an attribute argument
    // bool, byte, char, double, float, int, long, sbyte, short, string, uint, ulong, ushort, System.Object, System.Type, or an enum
    // Or an szarray/vector of the previous types
    union CustomAttributeDataStorage
    {
        Il2CppObject* obj;
        int64_t i;
        double d;
    };

    typedef std::function<bool(const MethodInfo*)> CustomAttributeFilter;

    struct CustomAttributeArgument
    {
        Il2CppClass* klass;
        CustomAttributeDataStorage data;
    };

    struct CustomAttributeFieldArgument
    {
        CustomAttributeArgument arg;
        const FieldInfo* field;
    };

    struct CustomAttributePropertyArgument
    {
        CustomAttributeArgument arg;
        const PropertyInfo* prop;
    };

    struct LazyCustomAttributeData
    {
        const MethodInfo* ctor;
        const void* dataStart;
        uint32_t dataLength;
    };

    struct CustomAttributeData
    {
        const MethodInfo* ctor;
    };

    class CustomAttributeReaderVisitor
    {
    public:
        // This Visitor methods will be called in the defined order
        virtual void MoveNext(const MethodInfo* ctor) {}
        virtual void VisitArgumentSizes(uint32_t argumentCount, uint32_t fieldCount, uint32_t propertyCount) {}
        virtual void VisitArgument(const CustomAttributeArgument& argument, uint32_t index) {}
        virtual void VisitCtor(const MethodInfo* ctor, CustomAttributeArgument args[], uint32_t argumentCount) {}
        virtual void VisitField(const CustomAttributeFieldArgument& field, uint32_t index) {}
        virtual void VisitProperty(const CustomAttributePropertyArgument& prop, uint32_t index) {}
    };


    class CustomAttributeDataIterator;

    class CustomAttributeCtorIterator
    {
    private:
        CustomAttributeCtorIterator(const char* ctorBuffer, const CustomAttributeFilter& filter) : ctorBuffer(ctorBuffer), filter(filter)
        {}
        CustomAttributeCtorIterator(const char* ctorBuffer) : ctorBuffer(ctorBuffer), filter(TrueFilter)
        {}

        const char* ctorBuffer;
        const CustomAttributeFilter filter;

        friend class CustomAttributeDataReader;
        friend class CustomAttributeDataIterator;

        static bool TrueFilter(const MethodInfo* ctor)
        {
            return true;
        }
    };

    class CustomAttributeDataIterator
    {
    private:
        CustomAttributeDataIterator(const char* ctorBuffer, const char* dataBuffer) : dataBuffer(dataBuffer), ctorBuffer(ctorBuffer), filter(CustomAttributeCtorIterator::TrueFilter)
        {
        }

        CustomAttributeDataIterator(const char* ctorBuffer, const char* dataBuffer, const CustomAttributeFilter& filter) : dataBuffer(dataBuffer), ctorBuffer(ctorBuffer), filter(filter)
        {}

        const char* ctorBuffer;
        const char* dataBuffer;
        const CustomAttributeFilter filter;

        friend class CustomAttributeDataReader;
    };

    class CustomAttributeDataReader
    {
    public:

        // Creates a CustomAttributeDataReader pointing into the metadata buffer start and end
        // This range must be for a single metadata member
        CustomAttributeDataReader(const Il2CppImage* image, const void* buffer, const void* bufferEnd);

        static CustomAttributeDataReader Empty()
        {
            return CustomAttributeDataReader();
        }

        // Returns the number of custom attributes stored for the member
        uint32_t GetCount() const
        {
            return count;
        }

        uint32_t GetCount(const CustomAttributeFilter& filter) const;

        // Iterate through all of the custom attribute constructors
        // Call GetCtorIterator to get the iterator and call this method until it returns false
        bool IterateAttributeCtors(const MethodInfo** attributeCtor, CustomAttributeCtorIterator* iter) const;

        // Iterate through all of the custom attribute data, but only return the attribute type and data range.
        // This method does not allocate
        // On each call LazyCustomAttributeData will be filled with new custom attribute data
        // Call GetDataIterator to get the iterator and call this method until it returns false
        // Call the static VisitCustomAttributeData function to get the attribute arguments from the LazyCustomAttributeData
        // If this function returns false *exc may be non-null if an exception occurred
        bool ReadLazyCustomAttributeData(LazyCustomAttributeData* data, CustomAttributeDataIterator* iter, Il2CppException** exc) const;

        // Iterate through all of the custom attribute data and get all of the custom attribute ctor arguments, fields, and parameter info
        // On each call the CustomAttributeReaderVisitor will be called with the information for the current custom attribute
        // If any of the arguments are managed types (e.g. string, object, arrays) this method will allocate them on the GC heap
        // Call GetDataIterator to get the iterator and call this function until it returns false
        // If this function returns false *exc may be non-null if an exception occurred
        bool VisitCustomAttributeData(CustomAttributeDataIterator* iter, CustomAttributeReaderVisitor* visitor, Il2CppException** exc) const;

        // Get the custom attribute ctor arguments, fields, and parameter info for a single custom attribute
        // The CustomAttributeReaderVisitor will be called with the information for this custom attribute
        // Call ReadLazyCustomAttributeData to get the dataStart & dataLength parameters
        // If any of the arguments are managed types (e.g. string, object, arrays) this method will allocate them on the GC heap
        // This method returns false on error
        // If this function returns false *exc may be non-null if an exception occurred
        static bool VisitCustomAttributeData(const Il2CppImage* image, const MethodInfo* ctor, const void* dataStart, uint32_t dataLength, CustomAttributeReaderVisitor* visitor, Il2CppException** exc);

        CustomAttributeCtorIterator GetCtorIterator() const;
        CustomAttributeCtorIterator GetCtorIterator(const CustomAttributeFilter& filter) const;
        CustomAttributeDataIterator GetDataIterator() const;
        CustomAttributeDataIterator GetDataIterator(const CustomAttributeFilter& filter) const;

    private:

        CustomAttributeDataReader() : image(nullptr), bufferStart(nullptr), bufferEnd(nullptr), count(0) {}
        CustomAttributeDataReader(const Il2CppImage* image, const char* dataStart, uint32_t dataLength);

        const char* GetDataBufferStart() const;

        bool IterateAttributeCtorsImpl(const MethodInfo** attributeCtor, const char** ctorBuffer) const;
        bool ReadPastCustomAttribute(const MethodInfo* ctor, CustomAttributeDataIterator* iter, Il2CppException** exc) const;
        bool ReadAndVisitCustomAttributeData(const MethodInfo* ctor, CustomAttributeDataIterator* iter, CustomAttributeReaderVisitor* visitor, Il2CppException** exc) const;
        bool ReadAndVisitCustomAttributeImpl(const MethodInfo* ctor, CustomAttributeDataIterator* iter, CustomAttributeReaderVisitor* visitor, Il2CppException** exc, bool deserializedManagedObjects) const;

        const Il2CppImage* image;
        const char* bufferStart;
        const char* bufferEnd;
        uint32_t count;
    };
}         /* namespace vm */
} /* namespace il2cpp */
