#include "il2cpp-config.h"
#include "gc/GCHandle.h"
#include "il2cpp-object-internals.h"
#include "GarbageCollector.h"
#include "os/Mutex.h"
#include "utils/Memory.h"
#include <memory>

namespace il2cpp
{
namespace gc
{
#if IL2CPP_SIZEOF_VOID_P == 4
#define HANDLE_COUNT 992
#define HANDLE_DATA_ALIGNMENT 4096
#else
#define HANDLE_COUNT 992
#define HANDLE_DATA_ALIGNMENT 8192
#endif

    typedef struct HandleData HandleData;
    struct HandleData
    {
        HandleData *next; //immutable
        HandleData *next_free; // next free
        uint32_t   *bitmap;
        uint32_t   in_use;
        uint32_t   size;
        uint8_t    type;
        uint32_t   slot_hint : 24; /* starting slot for search in bitmap */
        void*      entries[HANDLE_COUNT];
    };


    static HandleData* gc_handles[HANDLE_PINNED + 1];
    static HandleData* gc_handles_free[HANDLE_PINNED + 1];

    inline bool HandleTypeIsWeak(GCHandleType type)
    {
        return type == GCHandleType::HANDLE_WEAK || type == GCHandleType::HANDLE_WEAK_TRACK;
    }

#define BITMAP_SIZE (sizeof (*((HandleData *)NULL)->bitmap) * CHAR_BIT)

    static bool
    slot_occupied(HandleData* handles, uint32_t slot)
    {
        return handles->bitmap[slot / BITMAP_SIZE] & (1 << (slot % BITMAP_SIZE));
    }

    static void
    vacate_slot(HandleData* handles, uint32_t slot)
    {
        handles->in_use--;
        handles->bitmap[slot / BITMAP_SIZE] &= ~(1 << (slot % BITMAP_SIZE));

        if (handles->in_use == (handles->size - 1))
        {
            uint8_t type = handles->type;
            HandleData* first = gc_handles_free[type];
            handles->next_free = first;
            gc_handles_free[type] = handles;
        }
    }

    static void
    occupy_slot(HandleData* handles, uint32_t slot)
    {
        handles->in_use++;
        handles->bitmap[slot / BITMAP_SIZE] |= 1 << (slot % BITMAP_SIZE);

        if (handles->in_use == handles->size)
        {
            uint8_t type = handles->type;
            IL2CPP_ASSERT(handles == gc_handles_free[type]);
            gc_handles_free[type] = gc_handles_free[type]->next_free;
        }
    }

    static int
    find_first_unset(uint32_t bitmap)
    {
        int i;
        for (i = 0; i < 32; ++i)
        {
            if (!(bitmap & (1 << i)))
                return i;
        }
        return -1;
    }

    static HandleData*
    handle_data_alloc_entries(int type)
    {
        IL2CPP_ASSERT(sizeof(HandleData) < HANDLE_DATA_ALIGNMENT);
        IL2CPP_ASSERT(HANDLE_COUNT % BITMAP_SIZE == 0);
        HandleData* handles = (HandleData*)utils::Memory::AlignedMalloc(sizeof(HandleData), HANDLE_DATA_ALIGNMENT);
        memset(handles, 0, sizeof(HandleData));
        handles->type = type;
        handles->size = HANDLE_COUNT;
        if (!HandleTypeIsWeak((GCHandleType)handles->type))
        {
            GarbageCollector::RegisterRoot((char*)&handles->entries[0], HANDLE_COUNT * sizeof(void*));
        }
        handles->bitmap = (uint32_t*)utils::Memory::Calloc(sizeof(char), handles->size / CHAR_BIT);

        return handles;
    }

    static int32_t
    handle_data_next_unset(HandleData* handles)
    {
        uint32_t slot;
        for (slot = handles->slot_hint; slot < handles->size / BITMAP_SIZE; ++slot)
        {
            if (handles->bitmap[slot] == 0xffffffff)
                continue;
            handles->slot_hint = slot;
            return find_first_unset(handles->bitmap[slot]);
        }
        return -1;
    }

    static int32_t
    handle_data_first_unset(HandleData* handles)
    {
        uint32_t slot;
        for (slot = 0; slot < handles->slot_hint; ++slot)
        {
            if (handles->bitmap[slot] == 0xffffffff)
                continue;
            handles->slot_hint = slot;
            return find_first_unset(handles->bitmap[slot]);
        }
        return -1;
    }

    static int32_t
    handle_data_find_slot(HandleData* handles)
    {
        int32_t slot = 0;
        int32_t i = handle_data_next_unset(handles);
        if (i == -1 && handles->slot_hint != 0)
            i = handle_data_first_unset(handles);

        IL2CPP_ASSERT(i != -1);

        slot = handles->slot_hint * BITMAP_SIZE + i;
        return slot;
    }

    static Il2CppGCHandle
    handle_tag_weak(Il2CppGCHandle handle)
    {
        return (Il2CppGCHandle)((uintptr_t)handle | (uintptr_t)1);
    }

    static Il2CppGCHandle
    handle_untag_weak(Il2CppGCHandle handle)
    {
        return (Il2CppGCHandle)((uintptr_t)handle & ~(uintptr_t)1);
    }

    static uintptr_t AlignDownTo(uintptr_t size, uintptr_t align)
    {
        return size & ~(align - 1);
    }

    static HandleData*
    get_handle_data_from_handle(Il2CppGCHandle handle)
    {
        HandleData* handles = (HandleData*)AlignDownTo((uintptr_t)handle, HANDLE_DATA_ALIGNMENT);
        return handles;
    }

    static HandleData*
    handle_lookup(Il2CppGCHandle handle, uint32_t* slot)
    {
        HandleData* handles = get_handle_data_from_handle(handle);
        if (slot)
            *slot = (uint32_t)(ptrdiff_t)((void**)handle_untag_weak(handle) - &handles->entries[0]);
        return handles;
    }

    static baselib::ReentrantLock g_HandlesMutex;

#define lock_handles(handles) g_HandlesMutex.Acquire ()
#define unlock_handles(handles) g_HandlesMutex.Release ()

    static Il2CppGCHandle
    alloc_handle(GCHandleType type, Il2CppObject *obj, bool track)
    {
        int32_t slot = 0;
        Il2CppGCHandle res = 0;
        HandleData* handles = gc_handles[type];
        lock_handles(handles);
        handles = gc_handles_free[type];
        if (!handles)
        {
            handles = handle_data_alloc_entries(type);
            handles->next = gc_handles[type];
            gc_handles[type] = handles;

            handles->next_free = gc_handles_free[type];
            gc_handles_free[type] = handles;
        }
        slot = handle_data_find_slot(handles);
        occupy_slot(handles, slot);

        handles->entries[slot] = NULL;
        if (handles->type <= HANDLE_WEAK_TRACK)
        {
            if (obj)
                GarbageCollector::AddWeakLink(&(handles->entries[slot]), obj, track);
        }
        else
        {
            handles->entries[slot] = obj;
            GarbageCollector::SetWriteBarrier(handles->entries + slot);
        }

        //mono_perfcounters->gc_num_handles++;
        unlock_handles(handles);

        res = (Il2CppGCHandle) & handles->entries[slot];
        if (HandleTypeIsWeak((GCHandleType)handles->type))
        {
            /*
             * Use lowest bit as an optimization to indicate weak GC handle.
             * This allows client code to simply dereference strong GCHandle
             * when the bit is not set.
            */
            res = handle_tag_weak(res);
        }
        return res;
    }

    Il2CppGCHandle GCHandle::New(Il2CppObject *obj, bool pinned)
    {
        return alloc_handle(pinned ? HANDLE_PINNED : HANDLE_NORMAL, obj, false);
    }

    utils::Expected<Il2CppGCHandle> GCHandle::NewWeakref(Il2CppObject *obj, bool track_resurrection)
    {
        Il2CppGCHandle handle = alloc_handle(track_resurrection ? HANDLE_WEAK_TRACK : HANDLE_WEAK, obj, track_resurrection);

#ifndef HAVE_SGEN_GC
        if (track_resurrection)
            return utils::Il2CppError(utils::NotSupported, "IL2CPP does not support resurrection for weak references. Pass the trackResurrection with a value of false.");
#endif

        return (Il2CppGCHandle)handle;
    }

    GCHandleType GCHandle::GetHandleType(Il2CppGCHandle gchandle)
    {
        HandleData* handles = handle_lookup(gchandle, NULL);
        return (GCHandleType)handles->type;
    }

    Il2CppObject* GCHandle::GetTarget(Il2CppGCHandle gchandle)
    {
        uint32_t slot = 0;
        HandleData* handles = handle_lookup(gchandle, &slot);
        Il2CppObject *obj = NULL;

        if (handles->type >= HANDLE_TYPE_MAX)
            return NULL;

        lock_handles(handles);
        if (slot < handles->size && slot_occupied(handles, slot))
        {
            if (handles->type <= HANDLE_WEAK_TRACK)
            {
                obj = GarbageCollector::GetWeakLink(&handles->entries[slot]);
            }
            else
            {
                obj = (Il2CppObject*)handles->entries[slot];
            }
        }
        else
        {
            /* print a warning? */
        }
        unlock_handles(handles);
        /*g_print ("get target of entry %d of type %d: %p\n", slot, handles->type, obj);*/
        return obj;
    }

    static void
    il2cpp_gchandle_set_target(Il2CppGCHandle gchandle, Il2CppObject *obj)
    {
        uint32_t slot = 0;
        HandleData* handles = handle_lookup(gchandle, &slot);
        Il2CppObject *old_obj = NULL;


        IL2CPP_ASSERT(handles->type < HANDLE_TYPE_MAX);
        lock_handles(handles);
        if (slot < handles->size && slot_occupied(handles, slot))
        {
            if (handles->type <= HANDLE_WEAK_TRACK)
            {
                old_obj = (Il2CppObject*)handles->entries[slot];
                if (handles->entries[slot])
                    GarbageCollector::RemoveWeakLink(&handles->entries[slot]);
                if (obj)
                    GarbageCollector::AddWeakLink(&handles->entries[slot], obj, handles->type == HANDLE_WEAK_TRACK);
            }
            else
            {
                handles->entries[slot] = obj;
            }
        }
        else
        {
            /* print a warning? */
        }
        unlock_handles(handles);
    }

    void GCHandle::Free(Il2CppGCHandle gchandle)
    {
        if (!gchandle)
            return;

        uint32_t slot = 0;
        HandleData* handles = handle_lookup(gchandle, &slot);
        if (handles->type >= HANDLE_TYPE_MAX)
            return;

        lock_handles(handles);
        if (slot < handles->size && slot_occupied(handles, slot))
        {
            if (HandleTypeIsWeak((GCHandleType)handles->type))
            {
                if (handles->entries[slot])
                    GarbageCollector::RemoveWeakLink(&handles->entries[slot] /*, handles->type == HANDLE_WEAK_TRACK*/);
            }
            else
            {
                handles->entries[slot] = NULL;
            }
            vacate_slot(handles, slot);
        }
        else
        {
            /* print a warning? */
        }

        unlock_handles(handles);
    }

    utils::Expected<Il2CppGCHandle> GCHandle::GetTargetHandle(Il2CppObject * obj, Il2CppGCHandle handle, int32_t type)
    {
        if (type == -1)
        {
            il2cpp_gchandle_set_target(handle, obj);
            /* the handle doesn't change */
            return handle;
        }
        switch (type)
        {
            case HANDLE_WEAK:
                return NewWeakref(obj, false);
            case HANDLE_WEAK_TRACK:
                return NewWeakref(obj, true);
            case HANDLE_NORMAL:
                return New(obj, false);
            case HANDLE_PINNED:
                return New(obj, true);
            default:
                IL2CPP_ASSERT(0);
        }
        return 0;
    }

    void GCHandle::WalkStrongGCHandleTargets(WalkGCHandleTargetsCallback callback, void* context)
    {
        lock_handles(handles);
        const GCHandleType types[] = { HANDLE_NORMAL, HANDLE_PINNED };

        for (int gcHandleTypeIndex = 0; gcHandleTypeIndex < 2; gcHandleTypeIndex++)
        {
            const HandleData* handles = gc_handles[types[gcHandleTypeIndex]];

            while (handles != NULL)
            {
                for (uint32_t i = 0; i < handles->size; i++)
                {
                    if (handles->entries[i] != NULL)
                        callback(static_cast<Il2CppObject*>(handles->entries[i]), context);
                }

                handles = handles->next;
            }
        }
        unlock_handles(handles);
    }
} /* gc */
} /* il2cpp */
