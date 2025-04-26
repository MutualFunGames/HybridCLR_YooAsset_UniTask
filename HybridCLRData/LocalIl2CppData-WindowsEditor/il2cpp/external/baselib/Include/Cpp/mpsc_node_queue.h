#pragma once

#include "../C/Baselib_Memory.h"
#include "mpsc_node.h"

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

        // This implementation is a lockless node queue capable of handling multiple producers and a single consumer (exclusive access)
        //
        // Node types are required to inherit the mpsc_node class. No data from the inherited class is modified/copied, so no restrictions apply.
        // The node memory is allocated and destroyed by the user (user owned).
        // Dequeued nodes may be deleted, overwritten/discarded and/or reused.
        //
        // Notes consumer threads:
        //  Only one consumer thread will exclusively access the front node. Other consumer threads will always progress, either by failing to dequeue or
        //  successfully dequeuing the next node once the current thread thread opens access. As opposed to the parallel consumer implementation,
        //  this is significantly more performant as no DCAS-operations/loops are involved, but if the consumer thread with current exclusive access falls asleep
        //  when dequeuing, no other threads will successfully dequeue until the thread wakes up.
        //  Producer threads always progress independently.
        //
        // Notes on producer threads:
        //  A producer thread swaps the back and writes the link information in two consecutive atomic operations. If a producer thread falls asleep after the
        //  swap and before the link information has been written, the consumer thread(s) will not advance past this point since it doesn't have
        //  the information yet. Therefore the consumer threads calls will yield null until that particular producer thread wakes back up.
        //
        template<typename T>
        class alignas(sizeof(intptr_t) * 2)mpsc_node_queue
        {
        public:
            // Create a new queue instance.
            mpsc_node_queue()
            {
                m_FrontIntPtr.obj = EmptyBitMask;
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
                node->next.store(0, memory_order_relaxed);
                if (T* prev = m_Back.exchange(node, memory_order_release))
                    prev->next.store(node, memory_order_release);
                else
                    m_Front.store(node, memory_order_release);
            }

            // Push a linked list of nodes to the back of the queue.
            void push_back(T* first_node, T* last_node)
            {
                last_node->next.store(0, memory_order_relaxed);
                if (T* prev = m_Back.exchange(last_node, memory_order_release))
                    prev->next.store(first_node, memory_order_release);
                else
                    m_Front.store(first_node, memory_order_release);
            }

            // Try to pop frontmost node of the queue.
            //
            // Note that even in a single consumer scenario, null can be returned even if "empty" returns false.
            // See "Notes on producer threads".
            // Null is always returned if there is another thread currently in a consumer process on the queue.
            //
            // \returns front node of the queue or null.
            T* try_pop_front()
            {
                T* node, *next;

                // acquire thread exclusive access of front node, return 0 if fail or queue is empty
                intptr_t front = m_FrontIntPtr.fetch_or(ExclusiveAccessBitMask, memory_order_acquire);
                if (front & ExclusiveAccessAndEmptyBitMask)
                    return 0;

                node = (T*)front;
                next = static_cast<T*>(node->next.load(memory_order_relaxed));
                if (!next)
                {
                    // Check if queue back has progressed by comparing front to back node.
                    // If successful, back and front was equal, back now set to zero and a front reset (new ptr, clear flags) will
                    // be forced on next push, so we just return the node.
                    T* expectedBackNode = node;
                    if (m_Back.compare_exchange_strong(expectedBackNode, 0, memory_order_acquire, memory_order_relaxed))
                    {
                        // Queue is empty, unless progressed by a producer, set empty bit
                        intptr_t expectedFrontNode = front | ExclusiveAccessBitMask;
                        m_FrontIntPtr.compare_exchange_strong(expectedFrontNode, EmptyBitMask, memory_order_release, memory_order_relaxed);
                        return node;
                    }

                    // Re-read (front) node next ptr, checking if node has completed linkage.
                    // If linkage is complete use next ptr and continue usual code path (new front, return node).
                    next = static_cast<T*>(node->next.load(memory_order_relaxed));
                    if (OPTIMIZER_UNLIKELY(!next))
                    {
                        // Back progressed and front node still has incomplete linkage.
                        // We need to restore access since if back has progressed (more nodes were pushed) at this point, back is not reset.
                        // This is a raise condition where a producer thread has progressed but not yet written the front node next ptr.
                        // Reset front ptr (clear exclusive and empty bits) and return 0.
                        m_Front.store(node, memory_order_release);
                        return 0;
                    }
                }

                // Store next node (clear flags) and return node
                m_Front.store(next, memory_order_release);
                return node;
            }

        private:
            // Space out atomic members to individual cache lines. Required for native LLSC operations on some architectures, others to avoid false sharing
            char _cachelineSpacer0[PLATFORM_PROPERTY_CACHE_LINE_SIZE];
            union
            {
                atomic<T*> m_Front;
                atomic<intptr_t> m_FrontIntPtr;
            };
            char _cachelineSpacer1[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(T*)];
            atomic<T*> m_Back;
            char _cachelineSpacer2[PLATFORM_PROPERTY_CACHE_LINE_SIZE - sizeof(T*)];

            // Status bitmasks
            static constexpr size_t ExclusiveAccessBitMask = 1;
            static constexpr size_t EmptyBitMask = 2;
            static constexpr size_t ExclusiveAccessAndEmptyBitMask = ExclusiveAccessBitMask | EmptyBitMask;

            // Verify mpsc_node is base of T
            static_assert(std::is_base_of<baselib::mpsc_node, T>::value, "Node class/struct used with baselib::mpsc_node_queue must derive from baselib::mpsc_node.");
        };
    }
}
