#pragma once

#include "heap_allocator.h"
#include "page_allocator.h"
#include "Internal/chunked_allocator.inl.h"

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        // chunked_allocator_flags
        // Behaviour flags used with `chunked_allocator` parameter `flags`.
        //
        enum chunked_allocator_flags
        {
            // Disable page based allocation for base allocator. Allocate/deallocate invocation per block instead of page state modification.
            chunked_allocator_flags_paged_base_allocator_disable = detail::chunked_allocator_flags_paged_base_allocator_disable,
            // Evict (deallocate) expired blocks, i.e. when all allocations within a block have been deallocated (Note: reserved blocks does not expire).
            chunked_allocator_flags_evict_expired_blocks = detail::chunked_allocator_flags_evict_expired_blocks,
            // Disable clamping zero size to 1. Optional optimization if requested size of an re/allocation is guaranteed to be non-zero.
            chunked_allocator_flags_clamp_zero_size_disable = detail::chunked_allocator_flags_clamp_zero_size_disable
        };

        // chunked_allocator
        // Lockless chunked memory allocator capable of handling multiple concurrent allocations and deallocations (unless otherwise stated).
        // The allocator internally contain blocks of data, who's size and max number can be set when creating the allocator.
        // Allocating will bump an offset in the currently active block. When an allocation no longer fits in the current block, a new block is selected.
        // When all allocations within a block memory range have been deallocated by the user, the block becomes expired and is available for allocation again.
        // The exception for this is when the `chunked_allocator_flags_evict_expired_blocks` is set in which case the block is deallocated and must be allocated
        // if required to be used again (i.e. a tradeoff between performance and memory footprint).
        // Note that blocks that have been reserved can only be deallocated by invoking `deallocated_all`.
        //
        // Allocating memory is lockless with the cost of O(1) except when the current active block capacity is exhausted and
        // swapping to a new block is required, in which case a lock is taken to prevent greedy allocation.
        // When a new block is required, it is allocated if there are no expired blocks available.
        //
        //
        // Notes on size and alignment:
        //
        // - Alignment is defined by the alignment of `Allocator`. An alignment of 1 will optimise away alignment instructions when compiler optimisation is
        //   enabled i.e. an alignment size of 1 should be used if an inherited allocator is responsible for alignment.
        //
        // - Zero size allocations are by default allowed and will return a unique memory address. If size is guaranteed to be non-zero, this behaviour can be
        //   disabled saving the extra instructions (typically shr-add or sub-shr-add) using the `chunked_allocator_flags_clamp_zero_size_disable` flag.
        //
        // Notes on performance/memory requirements:
        //
        // - Allocating and deallocating synchronisation are decoupled (no false sharing). Allocating does in terms of concurrency execute only one relaxed atomic
        //   fetch_add operation when the allocator isn't exhausted. Deallocating will always emit a release barrier.
        //
        // - Except potential alignment, no memory overhead per allocation (no header information).
        //
        // - Max block count does not have any more memory overhead unless block memory is actually required (allocated).
        //
        // - Allocation only increment blocks internal offset when allocating. Allocating large blocks may lead to "holes" at the end of a block if they do not
        //   fit into the remaining block free space. Be careful to balance block size to frequent allocations of large sizes (use allocator composition to amend).
        //
        // - Reallocating will require a new allocation unless the new size is less than the current size or fit within the alignment padding of the old allocation.
        //   If a new memory alloction is required, a memory copy required and the old allocation space is lost until the block is expired.
        //
        // - allocate, reallocate and deallocate provides indexed alternatives, which can perform better when using many blocks and the flag
        //   chunked_allocator_flags_paged_base_allocator_disable. It has no effect on paged (default) base allocators which does not gain from this optimisation.
        //   Note that the `owns` method does not provide and indexed alternative and has the max cost of O(max_block_count) as it will need to check
        //   if the memory address is within range of any active buffer.
        //
        // Example use-cases:
        // When utilizing deallocation, allocations with a defined life-time over a certain time span (frames), such as render queues, input data or task processing.
        // When not utilizing deallocation (reset by using `deallocate_all`), allocations that has equally defined lifetime such as scoped resource loading.
        //
        // Parameters:
        // block_size                   - size of a block used for allocating a chunk of memory. Must be a pow2 value. Must be zero if constructor alternative is used.
        // max_block_count              - max number of blocks used. Must be in the range 2-64. Must be zero if constructor alternative is used.
        // Allocator                    - backing (base) memory allocator. Defaults to baselib `page_allocator`. Alignment of allocations are inherited from this allocator.
        // flags                        - behavior flags (see `chunked_allocator_flags`). Default is the value zero, no flags.
        // concurrent_access_capacity   - max amount of concurrent access to `allocate` or `reallocate`. Must be a pow2 value. Defaults to 1024.
        //                                Concurrent access above this limit is not allowed and may corrupt the allocator.
        // max_block_size_factor        - size factor of `block_size` to which the allocator can grow blocks for allocations of size larger than `block_size`.
        //                                Must be a pow2 value in the range 1-128. Defaults to 1.
        //
        template<size_t block_size, size_t max_block_count, class Allocator = baselib::page_allocator<8, baselib::Memory_PageState_Reserved>, uint32_t flags = 0, uint32_t concurrent_access_capacity = 1024, uint32_t max_block_size_factor = 1>
        class chunked_allocator : protected detail::chunked_allocator<Allocator, flags, concurrent_access_capacity, max_block_size_factor>
        {
            friend class detail::chunked_allocator_stats;

            using Impl = detail::chunked_allocator<Allocator, flags, concurrent_access_capacity, max_block_size_factor>;
            static constexpr bool ctorConfig = (block_size == 0 && max_block_count == 0);

            static_assert((block_size == 0) == (max_block_count == 0), "block_size and max_block_count must both be zero or non-zero");
            static_assert(baselib::Algorithm::IsPowerOfTwo(block_size), "block_size must be a pow2 value");
            static_assert(ctorConfig ? true : Allocator::alignment <= block_size, "Allocator::alignment must be less or equal to block_size");
            static_assert(Allocator::alignment != 0, "Allocator::alignment must not be zero");
            static_assert(baselib::Algorithm::IsPowerOfTwo(Allocator::alignment), "Allocator::alignment must be a pow2 value");
            static_assert(max_block_size_factor != 0, "max_block_size_factor must not be zero");
            static_assert(max_block_size_factor <= 128, "max_block_size_factor must be less or equal to 128");
            static_assert(baselib::Algorithm::IsPowerOfTwo(max_block_size_factor), "max_block_size_factor must be a pow2 value");
            static_assert(concurrent_access_capacity != 0, "concurrent_access_capacity must not be zero");
            static_assert(baselib::Algorithm::IsPowerOfTwo(concurrent_access_capacity), "concurrent_access_capacity must be a pow2 value");

        public:
            // non-copyable
            chunked_allocator(const chunked_allocator& other) = delete;
            chunked_allocator& operator=(const chunked_allocator& other) = delete;

            // non-movable (strictly speaking not needed but listed to signal intent)
            chunked_allocator(chunked_allocator && other) = delete;
            chunked_allocator& operator=(chunked_allocator&& other) = delete;

            // Allocated memory is guaranteed to always be aligned to at least the value of `alignment`.
            static constexpr uint32_t alignment = Impl::alignment;

            // Creates a new instance. `args` are optional parameters forwarded to the `Allocator` constructor.
            template<class ... Args, bool value = ctorConfig, typename std::enable_if<(!value), bool>::type = 0>
            chunked_allocator(Args&& ... args) : Impl(block_size, max_block_count, std::forward<Args>(args)...)
            {
                atomic_thread_fence(memory_order_seq_cst);
            }

            // Creates a new instance using run-time constructor parameters for block size and count. The same restrictions on parameter values apply.
            // Template parameters `block_size` and `max_block_count` must be both zero when this constructor is used.
            // `args` are optional parameters forwarded to the `Allocator` constructor.
            template<class ... Args, bool value = ctorConfig, typename std::enable_if<(value), bool>::type = 0>
            chunked_allocator(size_t blockSize, size_t blockCount, Args&& ... args) : Impl(blockSize, blockCount, std::forward<Args>(args)...)
            {
                BaselibAssert(blockSize != 0);
                BaselibAssert(blockCount != 0);
                BaselibAssert(baselib::Algorithm::IsPowerOfTwo(blockSize));
                atomic_thread_fence(memory_order_seq_cst);
            }

            // Destroy allocator, deallocates any memory allocated.
            //
            // If there are other threads currently accessing the allocator behavior is undefined.
            ~chunked_allocator() {}

            // Allocates a memory block large enough to hold `size` number of bytes.
            // `size` must less or equal to `block_size` multiplied by `max_block_size_factor`.
            //
            // \returns Address to memory block of allocated memory or nullptr if failed.
            void* allocate(size_t size)
            {
                return Impl::allocate(size);
            }

            // Allocates a memory block large enough to hold `size` number of bytes.
            // `size` must less or equal to `block_size` multiplied by `max_block_size_factor`.
            // If operation is successful `block_index` contains the internal block index of the allocation, to be used with subsequent indexed method calls.
            //
            // \returns Address to memory block of allocated memory or nullptr if failed.
            void* allocate(size_t size, uint32_t &block_index)
            {
                return Impl::allocate(size, block_index);
            }

            // Reallocates previously allocated or reallocated memory pointed to by `ptr` from `old_size` to `new_size` number of bytes.
            // `new_size` must be less or equal to `block_size` multiplied by `max_block_size_factor`.
            // Passing `nullptr` in `ptr` yield the same result as calling `allocate`.
            //
            // \returns Address to memory block of reallocated memory or nullptr if failed.
            void* reallocate(void* ptr, size_t old_size, size_t new_size)
            {
                return Impl::reallocate(ptr, old_size, new_size);
            }

            // Reallocates previously allocated or reallocated memory pointed to by `ptr` from `old_size` to `new_size` number of bytes.
            // `new_size` must be less or equal to `block_size` multiplied by `max_block_size_factor`.
            // `block_index` is the internal block index resulting from a previous call to `allocate` or `reallocate`.
            // If operation is successful `new_block_index` contains the internal block index of the reallocated memory.
            // Passing `nullptr` in `ptr` yield the same result as calling `allocate`.
            //
            // \returns Address to memory block of reallocated memory or nullptr if failed.
            void* reallocate(void* ptr, size_t old_size, size_t new_size, uint32_t block_index, uint32_t& new_block_index)
            {
                return Impl::reallocate(ptr, old_size, new_size, block_index, new_block_index);
            }

            // Deallocates memory block previously allocated or reallocated with `size` pointed to by `ptr`.
            // Passing nullptr in `ptr` result in a no-op.
            //
            // \returns Always returns true unless `ptr` is nullptr.
            bool deallocate(void* ptr, size_t size)
            {
                return Impl::deallocate(ptr, size);
            }

            // Deallocates memory block previously allocated or reallocated with `size` pointed to by `ptr`.
            // `block_index` is the internal block index resulting from a previous call to `allocate` or `reallocate` using block index methods.
            // Passing nullptr in `ptr` result in a no-op.
            // This function is guaranteed to emit an release barrier.
            //
            // \returns Always returns true unless `ptr` is nullptr.
            bool deallocate(void* ptr, size_t size, uint32_t block_index)
            {
                return Impl::deallocate(ptr, size, block_index);
            }

            // Release all resources and set capacity to zero
            //
            // Calling this function invalidates any currently allocated memory
            // If there are other threads currently accessing the allocator behavior is undefined.
            void deallocate_all()
            {
                Impl::deallocate_all();
            }

            // Request that the allocator capacity be at least enough to contain `capacity`.
            //
            // If `capacity` is less or equal to current capacity, the capacity is not affected.
            // Note that internally, `capacity` is rounded up to `block_size` which in turn is aligned to optimal allocation size of `Allocator`.
            //
            // \returns true if successful.
            bool reserve(size_t capacity)
            {
                return Impl::reserve(capacity);
            }

            // Get the current capacity.
            size_t capacity() const
            {
                return Impl::capacity();
            }

            // Calculate optimal allocation size given `size`.
            //
            // \returns Optimal size when allocating memory given `size` or zero if `size` is larger than `block_size` multiplied by `max_block_size_factor`.
            static constexpr size_t optimal_size(size_t size)
            {
                return Impl::optimal_size(size);
            }

            // Checks for the ownership allocation given `ptr` and `size`
            // If `size` is valid `ptr` is checked to be in range of allocator memory pool.
            // Note that this function is not O(1) if the `chunked_allocator_flags_paged_base_allocator_disable` is used. See header documentation for details.
            //
            // \returns True if the allocator owns the allocation.
            bool owns(const void *ptr, size_t size) const
            {
                return Impl::owns(ptr, size);
            }
        };

        // chunked_allocator_stats
        // Retrieve current state of an allocator.
        //
        class chunked_allocator_stats : protected detail::chunked_allocator_stats
        {
            using Impl = detail::chunked_allocator_stats;
        public:
            // Output data structure used by `block_stats`.
            struct block_stat : Impl::block_stat
            {
                FORCE_INLINE void*   memory() { return m_Memory; }                  // Block memory address
                FORCE_INLINE size_t  size() { return m_Size; }                      // Allocated bytes (including allocation alignment padding)
                FORCE_INLINE size_t  capacity() { return m_Capacity; }              // Capacity of block
                FORCE_INLINE uint8_t index() { return m_Index; }                    // Internal index of block (range zero to max block count of allocator)
                FORCE_INLINE uint8_t generation_id() { return m_GenerationId; }     // Generation id of the block (see `set_block_generation_id`)
            };

            // Retrieve an array current state of each active block in `allocator` into `block_stat` given `block_stats_flags`. `block_stat` must be at
            // least the size of the allocator max block count (64).
            // This is a lock-less operation, internally invoking atomic operations.
            // If there are other threads currently accessing the allocator the results of `block_stat` memory, size and capacity functions are approximations.
            // This method is intended for memory profiling, debugging and statistics.
            //
            // \returns Number of active blocks, i.e. the valid size of `block_stat`.
            template<class Allocator>
            static uint32_t block_stats(const Allocator& allocator, block_stat block_stat[]) { return Impl::block_stats(allocator, block_stat); }

            // Set the generation id for currently active and subsequent blocks used by `allocator`.
            // Default (initial) value is zero. Valid range is 0-255.
            template<class Allocator>
            static void set_block_generation_id(Allocator& allocator, uint8_t generationId) { Impl::set_block_generation_id(allocator, generationId); }
        };
    }
}
