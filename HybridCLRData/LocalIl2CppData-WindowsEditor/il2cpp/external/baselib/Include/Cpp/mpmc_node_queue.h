#pragma once

#include "../C/Baselib_Memory.h"
#include "../C/Baselib_Atomic_LLSC.h"
#include "mpmc_node.h"

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        // In computer science, a queue is a collection in which the entities in the collection are kept in order and the principal (or only) operations on the
        // collection are the addition of entities to the rear terminal position, known as enqueue, and removal of entities from the front terminal position, known
        // as dequeue. This makes the queue a First-In-First-Out (FIFO) data structure. In a FIFO data structure, the first element added to the queue will be the
        // first one to be removed. This is equivalent to the requirement that once a new element is added, all elements that were added before have to be removed
        // before the new element can be removed. Often a peek or front operation is also entered, returning the value of the front element without dequeuing it.
        // A queue is an example of a linear data structure, or more abstractly a sequential collection.
        //
        // "Queue (abstract data type)", Wikipedia: The Free Encyclopedia
        // https://en.wikipedia.org/w/index.php?title=Queue_(abstract_data_type)&oldid=878671332
        //

        // This implementation is a lockless node queue capable of handling multiple concurrent producers and consumers
        //
        // Node types are required to inherit the mpmc_node class. No data from the inherited class is modified/copied, so no restrictions apply.
        // The node memory is allocated and destroyed by the user (user owned).
        // Dequeued nodes may be overwritten/discarded and/or reused.
        // Dequeued nodes may not be deleted (released from user space memory) while any consumer thread is in the scope of a deque call.
        //
        // Notes consumer threads:
        //  While dequeued nodes may be reused and/or overwritten they must however remain in application readable memory (user space memory) until it can be
        //  guaranteed no consumer thread is still processing the node i.e. not within the scope of a dequeue call.
        //  Even though the value is ignored (discarded by version check) any consumer thread may still read the node link information.
        //  Consumer threads are concurrently attempting to dequeue the front in a DCAS loop and the first to succeed will update the queue front and other
        //  threads continue processing the next front node in the queue. Threads are garuanteed to progress dequeuing nodes even if another consumer
        //  thread falls asleep during a dequeue, but may fail to dequeue in the combination of the queue getting pre-emptied and the thread resetting the
        //  state (reload back) falls asleep while swapping the back (between 2x consecutive CAS operations).
        //  This is usually an extremely infrequent occurence due to the combination required (can not happen unless there's exactly one item in the queue).
        //  Producer threads always progress independently.
        //
        // Notes on producer threads:
        //  A producer thread swaps the back and writes the link information in two consecutive atomic operations. If a producer thread falls asleep after the
        //  swap and before the link information has been written, the consumer thread(s) will not advance past this point since it doesn't have
        //  the information yet. Therefore the consumer threads calls will yield null until that particular producer thread wakes back up.
        //
        template<typename T>
        class alignas(sizeof(intptr_t) * 2)mpmc_node_queue
        {
        public:
            // Create a new queue instance.
            mpmc_node_queue()
            {
                m_FrontIntPtr = EmptyBitMask;
                m_Front.obj.idx = 0;
                m_Back.obj = 0;
                atomic_thread_fence(memory_order_seq_cst);
            }

            // Returns true if queue is empty.
            bool empty() const
            {
                return (m_FrontIntPtr.load(memory_order_relaxed) & EmptyBitMask) != 0;
            }

            // Push a node to the back of the queue.
            void push_back(T* node)
            {
                // link to existing back node or reset front (also clearing access/empty flags)
                node->next.store(0, memory_order_relaxed);
                if (T* prev = m_Back.exchange(node, memory_order_release))
                    prev->next.store(node, memory_order_release);
                else
                    m_FrontPair.ptr.store(node, memory_order_release);
            }

            // Push a linked list of nodes to the back of the queue.
            void push_back(T* first_node, T* last_node)
            {
                last_node->next.store(0, memory_order_relaxed);
                if (T* prev = m_Back.exchange(last_node, memory_order_release))
                    prev->next.store(first_node, memory_order_release);
                else
                    m_FrontPair.ptr.store(first_node, memory_order_release);
            }

            // Try to pop frontmost node of the queue.
            //
            // Note that even in a single consumer scenario, null can be returned even if "empty" returns false.
            // See "Notes on producer threads".
            //
            // \returns front node of the queue or null.
            T* try_pop_front()
            {
                T* node, *next;
                if (PLATFORM_HAS_NATIVE_LLSC)
                {
                    intptr_t value;
                RetryNode:
                    Baselib_atomic_llsc_ptr_acquire_release_v(&m_Front, &node, &next,
                    {
                        // If front bit 0 is set, queue back is being reloaded or queue is empty.
                        value = reinterpret_cast<intptr_t>(node);
                        if (value & ExclusiveAccessAndEmptyBitMask)
                        {
                            Baselib_atomic_llsc_break();
                            return 0;
                        }

                        // Fetch next node. If zero, node is the current back node or this node has been consumed and the next ptr may have been overwritten with zero.
                        // LLSC Monitor is internally cleared by subsequent cmpxchg.
                        if (!(next = static_cast<T*>(node->next.obj)))
                            goto BackNode;
                    });
                    return node;

                BackNode:
                    // - Filters obsolete nodes (invalidates pop loops in flight - the llsc monitor is reset by the cmpxchg).
                    // - Set exclusive access (re-entrant block)
                    // - Maintain the front ptr address since multiple threads may linger here and concurrently a push call may have set the next ptr of front node.
                    //   This is handled by the consequent code below (fetch_and on front ptr to reset access).
                    T * front = node;
                    if (!m_FrontPair.ptr.compare_exchange_strong(front, reinterpret_cast<T*>(value | ExclusiveAccessBitMask), memory_order_acquire, memory_order_relaxed))
                    {
                        // m_FrontPair has changed, so the node has been consumed by another thread or another thread is processing a back node.
                        // We retry consuming another node. If it correctly failed due to empty or exclusive access, this is checked when retrying.
                        goto RetryNode;
                    }

                    // Check if queue back has progressed by comparing front to back node.
                    // If successful, back and front was equal, back now set to zero and a front reset will
                    // be forced on next push, so we just return the node.
                    if (m_Back.compare_exchange_strong(front, 0, memory_order_acquire, memory_order_relaxed))
                    {
                        // Queue is empty, unless progressed by a producer, set empty bit
                        intptr_t expectedFrontNode = value | ExclusiveAccessBitMask;
                        m_FrontIntPtr.compare_exchange_strong(expectedFrontNode, EmptyBitMask, memory_order_release, memory_order_relaxed);
                        return node;
                    }

                    // Re-read (front) node next ptr, checking if node has completed linkage.
                    next = static_cast<T*>(node->next.load(memory_order_relaxed));
                    if (OPTIMIZER_UNLIKELY(!next))
                    {
                        // Back progressed and front node still has incomplete linkage.
                        // We need to restore access since if back has progressed (more nodes were pushed) at this point, back is not reset.
                        // This is a raise condition where a producer thread has progressed but not yet written the front node next ptr.
                        // Reset status bits and return 0.
                        m_FrontPair.ptr.store(node, memory_order_release);
                        return 0;
                    }

                    // Linkage is complete. We can now replace front with next node since we're in exclusive access to the front, and return the node.
                    m_FrontPair.ptr.store(next, memory_order_release);
                    return node;
                }
                else
                {
                    SequencedFrontPtr front, value;

                    // Get front node. The DCAS while operation will update front on retry
                    front = m_Front.load(memory_order_acquire);
                RetryNodeDCAS:
                    do
                    {
                        // If front idx bit 0 is set, queue back is being reloaded or queue is empty.
                        if (front.intPtr & ExclusiveAccessAndEmptyBitMask)
                            return 0;

                        // Fetch next node. If zero, node is the current back node or this node has been consumed and the next ptr may have been overwritten with zero.
                        node = front.ptr;
                        if (!(next = static_cast<T*>(node->next.load(memory_order_relaxed))))
                            goto BackNodeDCAS;

                        // On success, replace the current with the next node and return node. On fail, retry with updated front.
                        value.ptr = next;
                        value.idx = front.idx + 1;
                    }
                    while (!m_Front.compare_exchange_strong(front, value, memory_order_acquire, memory_order_relaxed));
                    return node;

                BackNodeDCAS:
                    // - Filters obsolete nodes (invalidates pop loops in flight by idx bit set to odd).
                    // - Set exclusive access (re-entrant block)
                    // - Maintain the front ptr address since multiple threads may linger here and concurrently a push call may have set the next ptr of front node.
                    //   This is handled by the consequent code below (store to front ptr to reset access).
                    //   Add index for next node consumed since we're outside the DCAS scope.
                    intptr_t nextIndex = front.idx + 1;
                    value.intPtr = front.intPtr | ExclusiveAccessBitMask;
                    value.idx = nextIndex;
                    if (!m_Front.compare_exchange_strong(front, value, memory_order_acquire, memory_order_relaxed))
                    {
                        // m_Front has changed, so the node has been consumed by another thread or another thread is processing a back node.
                        // We retry consuming another node. If it correctly failed due to empty or exclusive access, this is checked when retrying.
                        goto RetryNodeDCAS;
                    }

                    // Check if queue back has progressed by comparing front to back node.
                    // If successful, back and front was equal, back now set to zero and a front reset (new ptr, clear flags) will
                    // be forced on next push, so we just return the node.
                    if (m_Back.compare_exchange_strong(front.ptr, 0, memory_order_acquire, memory_order_relaxed))
                    {
                        // Queue is empty, unless progressed by a producer, add empty bit.
                        intptr_t expectedFrontNode = front.intPtr | ExclusiveAccessBitMask;
                        m_FrontIntPtr.compare_exchange_strong(expectedFrontNode, EmptyBitMask, memory_order_release, memory_order_relaxed);
                        return node;
                    }

                    // Re-read (front) node next ptr, checking if node has completed linkage.
                    next = static_cast<T*>(node->next.load(memory_order_relaxed));
                    if (OPTIMIZER_UNLIKELY(!next))
                    {
                        // Back progressed and front node still has incomplete linkage.
                        // We need to restore access since if back has progressed (more nodes were pushed) at this point, back is not reset.
                        // This is a raise condition where a producer thread has progressed but not yet written the front node next ptr.
                        // Reset status bits and return 0.
                        m_FrontPair.ptr.store(node, memory_order_release);
                        return 0;
                    }

                    // Linkage is complete. We can now replace front with next node since we're in exclusive access to the front, and return the node.
                    value.ptr = next;
                    value.idx = nextIndex;
                    m_Front.store(value, memory_order_release);
                    return node;
                }
            }

        private:
            typedef struct
            {
                union
                {
                    T*       ptr;
                    intptr_t intPtr;
                };
                intptr_t idx;
            } SequencedFrontPtr;

            typedef struct
            {
                atomic<T*>       ptr;
                atomic<intptr_t> idx;
            } FrontPair;

            // Space out atomic members to individual cache lines. Required for native LLSC operations on some architectures, others to avoid false sharing
            char _cachelineSpacer0[PLATFORM_PROPERTY_CACHE_LINE_SIZE];
            union
            {
                atomic<intptr_t> m_FrontIntPtr;
                FrontPair m_FrontPair;
                atomic<SequencedFrontPtr> m_Front;
            };
            char _cachelineSpacer1[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(SequencedFrontPtr)];
            atomic<T*> m_Back;
            char _cachelineSpacer2[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(T*)];

            // Status bitmasks
            static constexpr size_t ExclusiveAccessBitMask = 1;
            static constexpr size_t EmptyBitMask = 2;
            static constexpr size_t ExclusiveAccessAndEmptyBitMask = ExclusiveAccessBitMask | EmptyBitMask;

            // FrontPair is atomic reflections of the SequencedFront fields used for CAS vs DCAS ops. They must match in size and layout.
            // Do note that we can not check layout (offsetof) as the template class is incomplete!
            static_assert(sizeof(mpmc_node_queue::m_FrontPair) == sizeof(mpmc_node_queue::m_Front), "SequencedFrontPtr and FrontPair must be of equal size");

            // Verify mpmc_node is base of T
            static_assert(std::is_base_of<baselib::mpmc_node, T>::value, "Node class/struct used with baselib::mpmc_node_queue must derive from baselib::mpmc_node.");
        };
    }
}
