#include "il2cpp-config.h"
#include "utils/MemoryPool.h"
#include "utils/Memory.h"
#include <algorithm>
#include <limits>

namespace il2cpp
{
namespace utils
{
    // by making all allocations a multiple of this value, we ensure the next
    // allocation will always be aligned to this value
    const size_t kMemoryAlignment = 8;
    static size_t s_RegionSize = 64 * 1024;

    static inline size_t MakeMultipleOf(size_t size, size_t alignment)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    struct MemoryPool::Region
    {
        char* start;
        char* current;
        size_t size;
        size_t free;
    };

    void MemoryPool::SetRegionSize(size_t size)
    {
        s_RegionSize = size;
    }

    size_t MemoryPool::GetRegionSize()
    {
        return s_RegionSize;
    }

    MemoryPool::MemoryPool()
    {
        AddRegion(s_RegionSize);
    }

    MemoryPool::MemoryPool(size_t initialSize)
    {
        AddRegion(initialSize);
    }

    MemoryPool::~MemoryPool()
    {
        for (RegionList::iterator iter = m_Regions.begin(); iter != m_Regions.end(); ++iter)
        {
            IL2CPP_FREE((*iter)->start);
            IL2CPP_FREE(*iter);
        }

        m_Regions.clear();
    }

    void* MemoryPool::Malloc(size_t size)
    {
        size = MakeMultipleOf(size, kMemoryAlignment);

        Region* region = m_Regions.back();
        if (region->free < size)
            region = AddRegion(size);

        IL2CPP_ASSERT(region->free >= size);

        void* value = region->current;
        region->current += size;
        region->free -= size;

        return value;
    }

    void* MemoryPool::Calloc(size_t count, size_t size)
    {
        void* ret = Malloc(count * size);
        return ret;
    }

    bool MemoryPool::Contains(const void* data) const
    {
        for (RegionList::const_iterator iter = m_Regions.begin(); iter != m_Regions.end(); ++iter)
        {
			Region* region = *iter;
			if (data >= region->start && data < region->start + region->size)
				return true;
        }
        return false;
    }

    MemoryPool::Region* MemoryPool::AddRegion(size_t size)
    {
        Region* newRegion = (Region*)IL2CPP_MALLOC(sizeof(Region));
        Region* lastFreeRegion = m_Regions.size() > 0 ? m_Regions.back() : NULL;
        size_t allocationSize;

        // If we have more than 1/16th (4k by default) of the current region remaining free,
        // perform a one off allocation rather than losing that space to fragmentation.
        if (lastFreeRegion != NULL && lastFreeRegion->free >= (s_RegionSize / 16))
        {
            allocationSize = size;

            m_Regions.pop_back();
            m_Regions.push_back(newRegion);
            m_Regions.push_back(lastFreeRegion);
        }
        else
        {
            allocationSize = std::max(s_RegionSize, size);
            m_Regions.push_back(newRegion);
        }

        newRegion->start = newRegion->current = (char*)IL2CPP_MALLOC_ZERO(allocationSize);
        newRegion->size = newRegion->free = allocationSize;

        return newRegion;
    }
}
}
