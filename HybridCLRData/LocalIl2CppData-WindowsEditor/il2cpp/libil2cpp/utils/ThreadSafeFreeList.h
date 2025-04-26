#pragma once

#include "Baselib.h"
#include "Cpp/mpmc_node_stack.h"

namespace il2cpp
{
namespace utils
{
    typedef baselib::mpmc_node ThreadSafeFreeListNode;

/// Lockless allocator that keeps instances of T on a free list.
///
/// T must be derived from ThreadSafeFreeListNode.
///
    template<typename T>
    struct ThreadSafeFreeList
    {
        T* Allocate()
        {
            T* instance = m_NodePool.try_pop_back();
            if (!instance)
                instance = new T();

            return instance;
        }

        void Release(T* instance)
        {
            m_NodePool.push_back(instance);
        }

        ~ThreadSafeFreeList()
        {
            T* instance;
            while ((instance = m_NodePool.try_pop_back()) != NULL)
                delete instance;
        }

    private:

        baselib::mpmc_node_stack<T> m_NodePool;
    };
} /* utils */
} /* il2cpp */
