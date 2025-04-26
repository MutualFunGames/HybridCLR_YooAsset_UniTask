#pragma once

#include "../Lock.h"
#include "../Algorithm.h"
#include "../Atomic.h"
#include <algorithm>
#include <type_traits>

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        namespace detail
        {
            // These flags represent user definable behaviour. The allocator default behaviour (zero/none) is the opposite of any of the below.
            enum chunked_allocator_flags
            {
                chunked_allocator_flags_paged_base_allocator_disable = 0x1,
                chunked_allocator_flags_evict_expired_blocks         = 0x2,
                chunked_allocator_flags_clamp_zero_size_disable      = 0x4
            };

            struct chunked_allocator_algorithm
            {
                static constexpr uint64_t Log2Base(uint64_t value, uint64_t offset) { return (value > 1) ? Log2Base(value >> (uint64_t)1, offset + 1) : offset; }
                static constexpr uint64_t Log2Base(uint64_t value) { return Log2Base(value, 0); }
                static constexpr uint64_t BitMask(uint64_t oneBits) { return (oneBits >= 64 ? -1 : ((uint64_t)1 << oneBits) - 1); }

                // Returns the optimal block count if originalSize is less than targetSize (or at least 2).
                static uint64_t OptimalBlockCount(size_t originalSize, size_t originalCount, size_t targetSize)
                {
                    return std::max((uint64_t)baselib::Algorithm::CeilAligned((originalSize * originalCount), targetSize) / targetSize, (uint64_t)2);
                }
            };

            ///
            /// block_allocator with and without chunked memory segment allocation
            ///
            template<class Allocator, uint32_t max_block_size_factor, bool chunked_block_layout>
            class alignas(sizeof(intptr_t) * 2)block_allocator
            {};

            //
            // Chunked memory segment block allocation (paged allocation)
            //
            template<class Allocator, uint32_t max_block_size_factor>
            class alignas(sizeof(intptr_t) * 2)block_allocator<Allocator, max_block_size_factor, false>
            {
                Allocator allocator;

            protected:
                static constexpr uint32_t alignment = Allocator::alignment;

                template<class ... Args>
                block_allocator(size_t blockSize, size_t blockCount, Args && ... args) :
                    allocator(std::forward<Args>(args)...),
                    m_BlockSizeMax(allocator.optimal_size(blockSize * max_block_size_factor)),
                    m_BlockCount(chunked_allocator_algorithm::OptimalBlockCount(blockSize, blockCount, allocator.optimal_size(blockSize))),
                    m_BlockSizeMaxLog2(chunked_allocator_algorithm::Log2Base(m_BlockSizeMax)),
                    m_MaxCapacity(m_BlockSizeMax * m_BlockCount),
                    m_BlockMemory((uintptr_t)allocator.allocate(static_cast<size_t>(m_MaxCapacity), baselib::Memory_PageState_Reserved))
                {
                }

                ~block_allocator()
                {
                    if (m_BlockMemory)
                        allocator.deallocate(reinterpret_cast<void*>(m_BlockMemory), static_cast<size_t>(m_MaxCapacity));
                }

                bool allocate_block(uint32_t block_index, size_t size)
                {
                    return allocator.set_page_state(memory_address_of(block_index, 0), size, baselib::Memory_PageState_ReadWrite);
                }

                bool resize_block(uint32_t block_index, size_t old_size, size_t new_size)
                {
                    if (new_size > old_size)
                    {
                        if (!allocator.set_page_state(memory_address_of(block_index, old_size), new_size - old_size, baselib::Memory_PageState_ReadWrite))
                            return false;
                    }
                    else if (new_size < old_size)
                    {
                        if (!allocator.set_page_state(memory_address_of(block_index, new_size), old_size - new_size, baselib::Memory_PageState_Reserved))
                            return false;
                    }
                    return true;
                }

                bool deallocate_block(uint32_t block_index,  size_t size)
                {
                    return allocator.set_page_state(memory_address_of(block_index, 0), size, baselib::Memory_PageState_Reserved);
                }

                FORCE_INLINE void* memory_address_of(uint32_t block_index, uint64_t offset) const
                {
                    return (void*)(m_BlockMemory + (static_cast<uint64_t>(block_index) << m_BlockSizeMaxLog2) + offset);
                }

                FORCE_INLINE uint32_t block_index_of(const void* ptr) const
                {
                    return static_cast<uint32_t>((reinterpret_cast<uintptr_t>(ptr) - m_BlockMemory) >> m_BlockSizeMaxLog2);
                }

                constexpr FORCE_INLINE size_t optimal_size(size_t size) const
                {
                    return allocator.optimal_size(size);
                }

                FORCE_INLINE bool owns(const void *ptr, size_t size) const
                {
                    if (m_BlockSizeMax < size)
                        return false;
                    return ptr == nullptr ? false : reinterpret_cast<const uintptr_t>(ptr) - m_BlockMemory < m_MaxCapacity;
                }

            protected:
                char _cachelineSpacer0[sizeof(Allocator) < PLATFORM_PROPERTY_CACHE_LINE_SIZE ? PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(Allocator) : sizeof(uint64_t)];
                const uint64_t  m_BlockSizeMax;
                const uint64_t  m_BlockCount;
                const uint64_t  m_BlockSizeMaxLog2;
                const uint64_t  m_MaxCapacity;
                const uintptr_t m_BlockMemory;
            };

            //
            // Non-chunked memory segment block allocation (heap type)
            //
            template<class Allocator, uint32_t max_block_size_factor>
            class alignas(sizeof(intptr_t) * 2)block_allocator<Allocator, max_block_size_factor, true>
            {
                Allocator allocator;

                struct Block
                {
                    uintptr_t ptr;
                    size_t size;
                };

            protected:
                static constexpr uint32_t alignment = Allocator::alignment;

                template<class ... Args>
                block_allocator(size_t blockSize, size_t blockCount, Args && ... args) :
                    allocator(std::forward<Args>(args)...),
                    m_Block{{0, 0}},
                m_BlockSizeMax(allocator.optimal_size(blockSize * max_block_size_factor)),
                m_BlockCount(chunked_allocator_algorithm::OptimalBlockCount(blockSize, blockCount, allocator.optimal_size(blockSize)))
                {
                }

                bool allocate_block(uint32_t block_index, size_t size)
                {
                    if (uintptr_t ptr = (uintptr_t)allocator.allocate(size))
                    {
                        m_Block[block_index] = { ptr, size };
                        return true;
                    }
                    return false;
                }

                bool resize_block(uint32_t block_index, size_t old_size, size_t new_size)
                {
                    if (uintptr_t ptr = (uintptr_t)allocator.allocate(new_size))
                    {
                        if (m_Block[block_index].ptr)
                            allocator.deallocate((void*)m_Block[block_index].ptr, old_size);
                        m_Block[block_index] = { ptr, new_size };
                        return true;
                    }
                    return false;
                }

                bool deallocate_block(uint32_t block_index, size_t size)
                {
                    allocator.deallocate((void*)m_Block[block_index].ptr, size);
                    m_Block[block_index] = { 0, 0 };
                    return true;
                }

                FORCE_INLINE void* memory_address_of(uint32_t block_index, uint64_t offset) const
                {
                    return (void*)(m_Block[block_index].ptr + offset);
                }

                FORCE_INLINE uint32_t block_index_of(const void* ptr) const
                {
                    uintptr_t ptrInt = reinterpret_cast<const uintptr_t>(ptr);
                    for (uint32_t i = 0; i < sizeof(m_Block) / sizeof(m_Block[0]); ++i)
                    {
                        if (!m_Block[i].ptr)
                            continue;
                        if (ptrInt - m_Block[i].ptr < m_Block[i].size)
                            return i;
                    }
                    return UINT32_MAX;
                }

                constexpr FORCE_INLINE size_t optimal_size(size_t size) const
                {
                    return allocator.optimal_size(size);
                }

                FORCE_INLINE bool owns(const void *ptr, size_t size) const
                {
                    if (m_BlockSizeMax < size)
                        return false;
                    return ptr == nullptr ? false : block_index_of(ptr) != UINT32_MAX;
                }

            protected:
                char _cachelineSpacer0[sizeof(Allocator) < PLATFORM_PROPERTY_CACHE_LINE_SIZE ? PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(Allocator) : sizeof(uint64_t)];
                Block          m_Block[64];
                const uint64_t  m_BlockSizeMax;
                const uint64_t  m_BlockCount;
            };

            //
            // chunked_allocator
            //
            template<class BaseAllocator, uint32_t flags, uint32_t concurrent_access_capacity, uint32_t max_block_size_factor>
            class chunked_allocator : protected block_allocator<BaseAllocator, max_block_size_factor, flags & chunked_allocator_flags_paged_base_allocator_disable>
            {
                friend class chunked_allocator_stats;

                using Impl = block_allocator<BaseAllocator, max_block_size_factor, flags & chunked_allocator_flags_paged_base_allocator_disable>;
                using Impl::m_BlockSizeMax;
                using Impl::m_BlockCount;

            public:
                static constexpr uint32_t alignment = Impl::alignment;

                template<class ... Args>
                chunked_allocator(size_t blockSize, size_t blockCount, Args&& ... args) :
                    Impl(blockSize, blockCount, std::forward<Args>(args)...),
                    m_BlockSize(Impl::optimal_size(blockSize)),
                    m_BlockSizeLog2(chunked_allocator_algorithm::Log2Base(m_BlockSize)),
                    m_BlockAllocationSizeArray{0},
                    m_BlockSizeFactorLog2Array{0},
                    m_Capacity(0),
                    m_FreeBlockMask(chunked_allocator_algorithm::BitMask(m_BlockCount)),
                    m_BlockGenerationIdArray{0},
                    m_BlockGenerationId(0),
                    m_ReservedBlockCount(0),
                    m_PositionLimit((EncodedPositionMask + 1) - (concurrent_access_capacity * m_BlockSizeMax)),
                    m_CapacityLock()
                {
                    m_EncodedBlockPosition = m_PositionLimit | EncodeBlockIndex(MaxBlocks);
                    BaselibAssert(blockCount >= 2 && "blockCount must be at least 2");
                    BaselibAssert(blockCount <= MaxBlocks && "blockCount must not be larger than 64");
                    BaselibAssert(blockSize < m_PositionLimit && "blockSize exceeded (reduce blockSize and/or blockCount).");
                    BaselibAssert(blockSize != 0 && "blockSize must not be zero");
                    BaselibAssert(baselib::Algorithm::IsPowerOfTwo(blockSize) && "blockSize must be a pow2 value");
                    BaselibAssert(baselib::Algorithm::IsPowerOfTwo(Impl::optimal_size(blockSize)) && "Allocator::alignment optimal_size of blockSize must be a pow2 value");
                    BaselibAssert(alignment <= blockSize && "Allocator::alignment must be less or equal to blockSize");
                }

                ~chunked_allocator()
                {
                    deallocate_all();
                }

                void* allocate(size_t requested_size)
                {
                    uint32_t blockIndex;
                    UNUSED(blockIndex);
                    return allocate(requested_size, blockIndex);
                }

                void* allocate(size_t requested_size, uint32_t& block_index)
                {
                    if (requested_size > m_BlockSizeMax)
                        return nullptr;
                    const uint64_t size = AlignSize(static_cast<uint64_t>(requested_size));

                    bool retry;
                    do
                    {
                        // Increment the position bits in the encoded block position, returning the current position
                        const uint64_t encodedBlockPos = baselib::atomic_fetch_add_explicit(m_EncodedBlockPosition, size, baselib::memory_order_relaxed);
                        const uint32_t blockIndex = DecodeBlockIndex(encodedBlockPos);
                        const uint64_t startPos = DecodeBlockPosition(encodedBlockPos);
                        // If current position + size fits in block, the allocation is successful, return address.
                        if ((startPos + size) <= (m_BlockSize << (DecodeBlockSizeFactorLog2(encodedBlockPos))))
                        {
                            block_index = blockIndex;
                            return Impl::memory_address_of(blockIndex, startPos);
                        }

                        // Early bail if allocator is exhausted, avoiding entering a lock if that is the case.
                        if (baselib::atomic_load_explicit(m_FreeBlockMask, baselib::memory_order_relaxed) != 0)
                        {
                            // Enter lock to avoid greedy allocation
                            retry = true;
                            m_CapacityLock.AcquireScoped([&] {
                                // If block index of m_EncodedBlockPosition has changed, some other thread has done the allocation (so, skip and retry)
                                if (blockIndex != DecodeBlockIndex(baselib::atomic_load_explicit(m_EncodedBlockPosition, baselib::memory_order_relaxed)))
                                    return;
                                uint32_t newBlockSizeFactorLog2;
                                uint32_t newBlockIndex = AllocateBlock(size, newBlockSizeFactorLog2);
                                retry = newBlockIndex != InvalidBlockIndex;
                                if (retry)
                                {
                                    // Swap the current encoded block position, updating with new block index and size factors, reseting position.
                                    // The release barrier is required for the following operations to guaranteed to happen after AllocateBlock (writes).
                                    const uint64_t oldPos = baselib::atomic_exchange_explicit(m_EncodedBlockPosition, EncodeBlockIndex(newBlockIndex) | EncodeBlockSizeFactorLog2(newBlockSizeFactorLog2), baselib::memory_order_release);

                                    // Offset (increment) the old block allocation size register with the returned position.
                                    // This is a "zero-sum game", where when the size reaches zero, all allocations in that block have been deallocated.
                                    // That is safe to do here, since the returned position included this allocation. The deallocation/inactivation of the
                                    // block will happen when the last deallocation is invoked (which can be the subsequent DeallocImpl call below)-
                                    OffsetBlockAllocationSize(blockIndex, DecodeBlockPosition(oldPos));

                                    // New block is set to used. This does not need to be in order with the operations following the release barrier above, since
                                    // no new block can be used before the end of this lock scope. The early bail check before the lock scope can potentially
                                    // result in a false positive, but that will in the rare event it happening resolve with the following changed block test.
                                    // (blockIndex != DecodeBlockIndex)
                                    SetBlockUsed(newBlockIndex);
                                }
                            });
                        }
                        else
                        {
                            // If allocator was exhausted, check if current block has changed and if so retry.
                            // Reload m_EncodedBlockPosition since we update that before m_FreeBlockMask (SetBlockUsed invocation) when swapping blocks above.
                            uint64_t encPos = baselib::atomic_load_explicit(m_EncodedBlockPosition, baselib::memory_order_relaxed);
                            uint64_t pos = DecodeBlockPosition(encPos);
                            // if block index has changed, we retry allocate (new block available)
                            retry = blockIndex != DecodeBlockIndex(encPos);
                            if ((!retry) && (pos >= m_PositionLimit))
                            {
                                // position limit has been reached for m_EncodedBlockPosition.
                                // Subtract size from what we incremented, unless block index has changed in which case we retry allocate.
                                // Note: In practice this only happens in case of a memory leak or no deallocation balance at all, but a safety net to protect
                                // m_EncodedBlockPosition position overflow (Ex: with 1GB block size and 1024 max concurrency, limit is 256 PebiBytes).
                                do
                                {
                                    if (baselib::atomic_compare_exchange_weak_explicit(m_EncodedBlockPosition, encPos, encPos - size, baselib::memory_order_relaxed, baselib::memory_order_relaxed))
                                        return nullptr;
                                    if (blockIndex != DecodeBlockIndex(encPos))
                                        retry = true;
                                }
                                while (!retry);
                            }
                        }
                        DeallocImpl(blockIndex, size);
                    }
                    while (retry);

                    return nullptr;
                }

                void* reallocate(void* ptr, size_t old_size, size_t new_size)
                {
                    uint32_t blockIndex;
                    UNUSED(blockIndex);
                    return reallocate(ptr, old_size, new_size, Impl::block_index_of(ptr), blockIndex);
                }

                void* reallocate(void* ptr, size_t old_size, size_t new_size, uint32_t old_block_index, uint32_t& new_block_index)
                {
                    if (ptr == nullptr)
                        return allocate(new_size, new_block_index);

                    old_size = static_cast<size_t>(AlignSize(old_size));
                    if (new_size <= old_size)
                    {
                        // Reuse the same memory, invoke DeallocImpl which subtracts the reduced size alternatively deallocates if applicable.
                        new_block_index = old_block_index;
                        new_size = static_cast<size_t>(AlignSize(new_size));
                        if (new_size == old_size)
                            return ptr;
                        DeallocImpl(old_block_index, old_size - new_size);
                        return new_size == 0 ? nullptr : ptr;
                    }
                    if (void *newPtr = allocate(new_size, new_block_index))
                    {
                        std::memcpy(newPtr, ptr, old_size);
                        DeallocImpl(old_block_index, old_size);
                        return newPtr;
                    }
                    return nullptr;
                }

                bool deallocate(void* ptr, size_t size)
                {
                    if (ptr == nullptr)
                        return false;
                    DeallocImpl(Impl::block_index_of(ptr), AlignSize(size));
                    return true;
                }

                bool deallocate(void* ptr, size_t size, uint32_t block_index)
                {
                    if (ptr == nullptr)
                        return false;
                    DeallocImpl(block_index, AlignSize(size));
                    return true;
                }

                void deallocate_all()
                {
                    if (capacity() == 0)
                        return;
                    for (uint32_t i = 0; i < m_BlockCount; ++i)
                    {
                        if (IsBlockUsed(i) || IsBlockReserved(i))
                            Impl::deallocate_block(i, static_cast<size_t>(BlockSize(i)));
                    }
                    std::memset(&m_BlockAllocationSizeArray, 0, sizeof(m_BlockAllocationSizeArray));
                    std::memset(&m_BlockSizeFactorLog2Array, 0, sizeof(m_BlockSizeFactorLog2Array));
                    m_FreeBlockMask = chunked_allocator_algorithm::BitMask(m_BlockCount);
                    m_Capacity = 0;
                    m_ReservedBlockCount = 0;
                    baselib::atomic_store_explicit(m_EncodedBlockPosition, m_PositionLimit | EncodeBlockIndex(MaxBlocks), baselib::memory_order_release);
                }

                bool reserve(size_t capacity)
                {
                    // regardless of growable blocks used or not, we do not support (API) for reserving more than default block size * count
                    if (capacity > (m_BlockSize * m_BlockCount))
                        return false;

                    bool result = true;
                    uint32_t blockCount = static_cast<uint32_t>(Impl::optimal_size(baselib::Algorithm::CeilAligned(capacity, m_BlockSize)) >> m_BlockSizeLog2);
                    m_CapacityLock.AcquireScoped([&] {
                        uint64_t reservedSize = 0;
                        // Check size of currently reserved blocks (including if any reserved block has grown). Then allocate nr of blocks needed (if any)
                        // to cover the requested reserved capacity.
                        for (uint32_t block_index = 0; block_index < m_ReservedBlockCount; ++block_index)
                            reservedSize += BlockSize(block_index);
                        if (reservedSize < (blockCount * m_BlockSize))
                        {
                            if (blockCount > m_ReservedBlockCount)
                            {
                                uint32_t blocksReserved = 0;
                                for (uint32_t block_index = m_ReservedBlockCount; block_index < blockCount; ++block_index)
                                {
                                    // Only allocate blocks not in use. Blocks already in use are included in current capacity and will no be marked reserved.
                                    if (!IsBlockUsed(block_index))
                                    {
                                        if (!Impl::allocate_block(block_index, static_cast<size_t>(m_BlockSize)))
                                        {
                                            blockCount = block_index;
                                            result = false;
                                            break;
                                        }
                                        ++blocksReserved;
                                    }
                                }
                                // atomic op since `capacity` can access m_Capacity concurrently
                                baselib::atomic_fetch_add_explicit(m_Capacity, blocksReserved << m_BlockSizeLog2, baselib::memory_order_relaxed);
                                m_ReservedBlockCount = blockCount;
                            }
                        }
                    });
                    return result;
                }

                size_t capacity() const
                {
                    return static_cast<size_t>(baselib::atomic_load_explicit(m_Capacity, baselib::memory_order_relaxed));
                }

                static constexpr size_t optimal_size(size_t size)
                {
                    return baselib::Algorithm::CeilAligned(size, alignment);
                }

                bool owns(const void *ptr, size_t size) const
                {
                    return Impl::owns(ptr, size);
                }

            private:
                // m_EncodedBlockPosition consists of the following bits MSB to LSB (bitcount) [block index:7][block size factor:0-3][position (offset):54-57]
                // Block index bits representation
                static constexpr uint32_t InvalidBlockIndex = 0xffffffff;
                static constexpr uint64_t MaxBlocks = 64;
                static constexpr uint64_t EncodedBlockIndexBits = chunked_allocator_algorithm::Log2Base(MaxBlocks) + 1; // One extra bit for initial setup
                static constexpr uint32_t EncodedBlockIndexBitsOffset = std::numeric_limits<uint64_t>::digits - EncodedBlockIndexBits;
                // max_block_size_factor bits representation. EncodedBlockSizeFactorBits is nr of bits needed to represent max_block_size_factor size-shift.
                // which is the range of 0 (1x) to 7 (128x) so 3 the range is from 0-3.
                static constexpr uint32_t MaxBlockSizeFactorLog2 = static_cast<uint32_t>(chunked_allocator_algorithm::Log2Base(max_block_size_factor));
                static constexpr uint64_t EncodedBlockSizeFactorBits = chunked_allocator_algorithm::Log2Base(baselib::Algorithm::CeilPowerOfTwo(MaxBlockSizeFactorLog2 + 1));
                static constexpr uint64_t EncodedBlockSizeFactorBitsOffset = EncodedBlockIndexBitsOffset - EncodedBlockSizeFactorBits;
                static constexpr uint64_t EncodedBlockSizeFactorMask = (1 << EncodedBlockSizeFactorBits) - 1;
                // Remaining nr of bits (54-57) are used for the Position.
                static constexpr uint64_t EncodedPositionMask = ((uint64_t)1 << (std::numeric_limits<uint64_t>::digits - (EncodedBlockIndexBits + EncodedBlockSizeFactorBits))) - (uint64_t)1;

                static constexpr uint64_t EncodeBlockIndex(uint32_t index) { return static_cast<uint64_t>(index) << EncodedBlockIndexBitsOffset; }
                static constexpr uint32_t DecodeBlockIndex(uint64_t encoded_position) { return encoded_position >> EncodedBlockIndexBitsOffset; }
                static constexpr uint64_t DecodeBlockPosition(uint64_t encoded_position) { return encoded_position & EncodedPositionMask; }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(enable), int>::type = 0>
                FORCE_INLINE uint64_t BlockSize(uint32_t) const { return m_BlockSize; }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(!enable), int>::type = 0>
                FORCE_INLINE uint64_t BlockSize(uint32_t block_index) const { return m_BlockSize << m_BlockSizeFactorLog2Array[block_index]; }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(enable), int>::type = 0>
                static constexpr uint64_t EncodeBlockSizeFactorLog2(uint32_t) { return 0; }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(!enable), int>::type = 0>
                static constexpr uint64_t EncodeBlockSizeFactorLog2(uint32_t factor) { return static_cast<uint64_t>(factor) << EncodedBlockSizeFactorBitsOffset; }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(enable), int>::type = 0>
                static constexpr uint32_t DecodeBlockSizeFactorLog2(uint64_t) { return 0; }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(!enable), int>::type = 0>
                static constexpr uint32_t DecodeBlockSizeFactorLog2(uint64_t encoded_position) { return ((encoded_position >> EncodedBlockSizeFactorBitsOffset) & EncodedBlockSizeFactorMask); }

                // Returns the log2 factor required for pow2 of size rounded up to block size.
                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(enable), int>::type = 0>
                uint8_t SizeToBlockSizeFactorLog2(uint64_t) const { return 0; }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(!enable), int>::type = 0>
                FORCE_INLINE uint8_t SizeToBlockSizeFactorLog2(uint64_t size) const
                {
                    return (uint8_t)((baselib::Algorithm::HighestBit((uint64_t)baselib::Algorithm::CeilAligned(size, m_BlockSize) - 1) + 1) - m_BlockSizeLog2);
                }

                FORCE_INLINE uint32_t GetFirstFreeBlockIndex() const { return static_cast<uint32_t>(baselib::Algorithm::LowestBit(m_FreeBlockMask)); }
                FORCE_INLINE bool IsBlockUsed(uint32_t block_index) const { return !(m_FreeBlockMask & ((uint64_t)1 << block_index)); }
                FORCE_INLINE void SetBlockUsed(uint32_t block_index) { m_FreeBlockMask = m_FreeBlockMask & ~((uint64_t)1 << block_index); }
                FORCE_INLINE void SetBlockUnused(uint32_t block_index) { m_FreeBlockMask = m_FreeBlockMask | ((uint64_t)1 << block_index); }
                FORCE_INLINE bool IsBlockReserved(uint32_t block_index) const { return (block_index  < m_ReservedBlockCount); }
                FORCE_INLINE void UpdateBlockGenerationId(uint32_t block_index) { m_BlockGenerationIdArray[block_index] = m_BlockGenerationId; }
                FORCE_INLINE void OffsetBlockAllocationSize(uint32_t block_index, uint64_t size)
                {
                    baselib::atomic_fetch_add_explicit(m_BlockAllocationSizeArray[block_index], size, baselib::memory_order_relaxed);
                }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(enable), int>::type = 0>
                uint32_t AllocateBlock(uint64_t, uint32_t&)
                {
                    uint32_t firstFreeBlockIndex = static_cast<uint32_t>(baselib::Algorithm::LowestBit(m_FreeBlockMask));
                    if (firstFreeBlockIndex == 0xffffffff)
                        return InvalidBlockIndex;
                    if (!IsBlockReserved(firstFreeBlockIndex))
                    {
                        if (!Impl::allocate_block(firstFreeBlockIndex, static_cast<size_t>(m_BlockSize)))
                            return InvalidBlockIndex;
                        baselib::atomic_fetch_add_explicit(m_Capacity, m_BlockSize, baselib::memory_order_relaxed);
                        // "auto-reserve" allocated blocks, preventing them from being deallocated (unless deallocate_all is called).
                        m_ReservedBlockCount += (flags & chunked_allocator_flags_evict_expired_blocks) ? 0 : 1;
                    }
                    UpdateBlockGenerationId(firstFreeBlockIndex);
                    return firstFreeBlockIndex;
                }

                template<bool enable = max_block_size_factor == 1, typename std::enable_if<(!enable), int>::type = 0>
                uint32_t AllocateBlock(uint64_t size, uint32_t& blockSizeFactorLog2)
                {
                    uint32_t firstFreeBlockIndex = static_cast<uint32_t>(baselib::Algorithm::LowestBit(m_FreeBlockMask));
                    if (firstFreeBlockIndex == 0xffffffff)
                        return InvalidBlockIndex;
                    uint64_t capacityOffset;
                    const uint8_t sizeFactorLog2 = SizeToBlockSizeFactorLog2(size);
                    if (IsBlockReserved(firstFreeBlockIndex))
                    {
                        // If any reserved block is large enough to hold size, use that.
                        for (uint32_t i = firstFreeBlockIndex; i < m_ReservedBlockCount; ++i)
                        {
                            if (IsBlockUsed(i) || (m_BlockSizeFactorLog2Array[i] < sizeFactorLog2))
                                continue;
                            blockSizeFactorLog2 = m_BlockSizeFactorLog2Array[i];
                            UpdateBlockGenerationId(i);
                            return i;
                        }
                        // Expand existing block
                        uint64_t oldSize = BlockSize(firstFreeBlockIndex);
                        if (!Impl::resize_block(firstFreeBlockIndex, static_cast<size_t>(oldSize), static_cast<size_t>(m_BlockSize << sizeFactorLog2)))
                            return InvalidBlockIndex;
                        capacityOffset = (m_BlockSize << sizeFactorLog2) - oldSize;
                    }
                    else
                    {
                        if (!Impl::allocate_block(firstFreeBlockIndex, static_cast<size_t>(m_BlockSize << sizeFactorLog2)))
                            return InvalidBlockIndex;
                        // If chunked_allocator_flags_evict_expired_blocks is false (default) "auto-reserve" allocated blocks,
                        // preventing them from being deallocated (unless deallocate_all is called).
                        m_ReservedBlockCount += (flags & chunked_allocator_flags_evict_expired_blocks) ? 0 : 1;
                        capacityOffset = m_BlockSize << sizeFactorLog2;
                    }
                    baselib::atomic_fetch_add_explicit(m_Capacity, capacityOffset, baselib::memory_order_relaxed);
                    m_BlockSizeFactorLog2Array[firstFreeBlockIndex] = blockSizeFactorLog2 = sizeFactorLog2;
                    UpdateBlockGenerationId(firstFreeBlockIndex);
                    return firstFreeBlockIndex;
                }

                FORCE_INLINE void DeallocateBlock(uint32_t block_index)
                {
                    if (!IsBlockReserved(block_index))
                    {
                        // The last block index is reserved for internal use, it is never allocated so never free it (only ever happens once or after deallocate_all).
                        if (OPTIMIZER_UNLIKELY(block_index >= m_BlockCount))
                            return;
                        // Capacity is not required to be synchronized at this point (order independent).
                        Impl::deallocate_block(block_index, static_cast<size_t>(BlockSize(block_index)));
                        baselib::atomic_fetch_sub_explicit(m_Capacity, BlockSize(block_index), baselib::memory_order_relaxed);
                        m_BlockSizeFactorLog2Array[block_index] = 0;
                    }
                    // DeallocateBlock is called within the capacity lock so m_FreeBlockMask is not required to be synchronized at this point.
                    SetBlockUnused(block_index);
                }

                FORCE_INLINE void DeallocImpl(uint32_t block_index, uint64_t size)
                {
                    // If subtraction result is equal to size, it is the last allocation meaning block can be deallocated.
                    // A release fence is used to guarantee writes are not reordered past block release (if it happens on another thread)
                    if (baselib::atomic_fetch_sub_explicit(m_BlockAllocationSizeArray[block_index], size, baselib::memory_order_release) == size)
                    {
                        m_CapacityLock.AcquireScoped([&] {
                            DeallocateBlock(block_index);
                        });
                    }
                }

                template<bool disable = (flags & chunked_allocator_flags_clamp_zero_size_disable) != 0, typename std::enable_if<(disable), int>::type = 0>
                static constexpr uint64_t AlignSize(uint64_t value) { return (value + alignment - 1) & ~(static_cast<uint64_t>(alignment) - 1); }

                template<bool disable = (flags & chunked_allocator_flags_clamp_zero_size_disable) != 0, typename std::enable_if<(!disable), int>::type = 0>
                static constexpr uint64_t AlignSize(uint64_t value) { return (((value - 1) >> 63) + value + alignment - 1) & ~(static_cast<uint64_t>(alignment) - 1); }

                // 1-2x frequent read only access for allocate. Shared with block_allocator constants
                const uint64_t  m_BlockSize;
                const uint64_t  m_BlockSizeLog2;

                // 1x frequent read/write access for allocate (+2x infrequent, for new block).
                char _cachelineSpacer0[PLATFORM_PROPERTY_CACHE_LINE_SIZE - (sizeof(uint64_t) * 2)];
                uint64_t m_EncodedBlockPosition;   // nr bits reserved MSB to LSB [block index:7][block size factor:0-3][position (offset):54-57]
                char _cachelineSpacer1[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(uint64_t)];

                // 1x frequent access for deallocate (infrequent for allocate). Last element reserved for initial allocator setup
                uint64_t        m_BlockAllocationSizeArray[MaxBlocks + 1];

                // Other, infrequence access - separate from above high frequency access
                uint8_t         m_BlockSizeFactorLog2Array[MaxBlocks];  // read/modified when swapping and evicting or blocks
                uint64_t        m_Capacity;                             // read only by `capacity, `reserve` and swapping blocks
                uint64_t        m_FreeBlockMask;                        // read/modified when swapping blocks
                uint8_t         m_BlockGenerationIdArray[MaxBlocks];    // write when swapping blocks, read by `chunked_allocator_stats::stat`
                uint8_t         m_BlockGenerationId;                    // read when swapping blocks
                uint32_t        m_ReservedBlockCount;                   // read when swapping blocks, write in `reserve` call increments when expanding
                const uint64_t  m_PositionLimit;                        // read only by deallocate_all and ctor
                baselib::Lock   m_CapacityLock;
            };

            class chunked_allocator_stats
            {
            protected:
                struct block_stat
                {
                    friend class chunked_allocator_stats;
                protected:
                    void*   m_Memory;
                    size_t  m_Size;
                    size_t  m_Capacity;
                    uint8_t m_Index;
                    uint8_t m_GenerationId;
                };

                template<class Allocator>
                static uint32_t block_stats(const Allocator& allocator, block_stat block_stat[])
                {
                    uint32_t blocks = 0;
                    for (uint32_t i = 0; i < static_cast<uint32_t>(allocator.m_BlockCount); ++i)
                    {
                        if (!allocator.IsBlockUsed(i))
                            continue;

                        if (void *p = allocator.memory_address_of(i, 0))
                            block_stat[blocks].m_Memory = p;
                        else
                            continue;

                        int64_t size = static_cast<int64_t>(baselib::atomic_load_explicit(allocator.m_BlockAllocationSizeArray[i], baselib::memory_order_relaxed));
                        // If m_BlockAllocationSizeArray is <= 0, it is the current active block we are accessing.
                        if (size <= 0)
                        {
                            uint64_t encodedBlockPos = baselib::atomic_load_explicit(allocator.m_EncodedBlockPosition, baselib::memory_order_relaxed);
                            if (allocator.DecodeBlockIndex(encodedBlockPos) == i)
                            {
                                // The m_BlockAllocationSizeArray and m_EncodedBlockPosition index are equal, it is valid to use encoded position as current size.
                                size += allocator.DecodeBlockPosition(encodedBlockPos);
                            }
                            else
                            {
                                // There was a block swap, so it is now safe to reload m_BlockAllocationSizeArray and use as size.
                                size = static_cast<int64_t>(baselib::atomic_load_explicit(allocator.m_BlockAllocationSizeArray[i], baselib::memory_order_relaxed));
                            }
                        }
                        block_stat[blocks].m_Size = static_cast<size_t>(size);

                        block_stat[blocks].m_Capacity = static_cast<size_t>(allocator.BlockSize(i));
                        block_stat[blocks].m_Index = i;
                        block_stat[blocks].m_GenerationId = allocator.m_BlockGenerationIdArray[i];
                        ++blocks;
                    }
                    return blocks;
                }

                template<class Allocator>
                static void set_block_generation_id(Allocator& allocator, uint8_t generationId)
                {
                    allocator.m_BlockGenerationId = generationId;
                    // Update current active block to new generation id. Mask with max blocks in case block index >= allocator.MaxBlocks. This can only happen
                    // if this function is called before the first block switch (In that case, zero is valid (discarded)).
                    uint64_t encodedBlockPos = baselib::atomic_load_explicit(allocator.m_EncodedBlockPosition, baselib::memory_order_relaxed);
                    allocator.m_BlockGenerationIdArray[allocator.DecodeBlockIndex(encodedBlockPos) & (allocator.MaxBlocks - 1)] = generationId;
                }
            };
        }
    }
}
