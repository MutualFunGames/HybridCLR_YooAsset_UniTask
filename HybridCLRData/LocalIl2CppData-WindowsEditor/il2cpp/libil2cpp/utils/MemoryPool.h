#pragma once

#include <list>

namespace il2cpp
{
namespace utils
{
    class MemoryPool
    {
    public:
        MemoryPool();
        MemoryPool(size_t initialSize);
        ~MemoryPool();
        void* Malloc(size_t size);
        void* Calloc(size_t count, size_t size);

        static void SetRegionSize(size_t size);
        static size_t GetRegionSize();
        bool Contains(const void* data) const;
    private:
        struct Region;
        typedef std::list<Region*> RegionList;

        Region* AddRegion(size_t size);

        RegionList m_Regions;
    };
} /* namespace utils */
} /* namespace il2cpp */
