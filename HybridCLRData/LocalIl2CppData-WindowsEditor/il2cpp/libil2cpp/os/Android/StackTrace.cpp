#include "il2cpp-config.h"

#if IL2CPP_TARGET_ANDROID

#include "os/StackTrace.h"
#include "os/Image.h"

#include <unwind.h>
#include <dlfcn.h>
#include <link.h>
#include <pthread.h>
#include <string.h>

namespace il2cpp
{
namespace os
{
    const int kMaxStackFrames = 128;

namespace
{
    extern "C" char end;

    uintptr_t s_BaseAddress;
    uintptr_t s_EndAddress;
    uintptr_t s_LibUnityBaseAddress;
    uintptr_t s_LibUnityEndAddress;
    pthread_once_t s_InitKnownSymbolInfoOnceFlag = PTHREAD_ONCE_INIT;

    static int
    libUnityLookupCallback(struct dl_phdr_info *info, size_t size, void *data)
    {
        int j;
        uintptr_t endAddr;

        // dlpi_name can have different values depending on Android OS:
        // Google Pixel 2 Android 10, dlpi_name will be "/data/app/com.unity.stopaskingforpackagename-uRHSDLXYA4cnHxyTNT30-g==/lib/arm/libunity.so"
        // Samsung GT-I9505 Android 5, dlpi_name will be "libunity.so"
        if (info->dlpi_name == NULL || strstr(info->dlpi_name, "libunity.so") == NULL)
            return 0;

        s_LibUnityBaseAddress = s_LibUnityEndAddress = info->dlpi_addr;

        for (j = 0; j < info->dlpi_phnum; j++)
        {
            endAddr = (uintptr_t)(((char*)info->dlpi_addr) + info->dlpi_phdr[j].p_vaddr + info->dlpi_phdr[j].p_memsz);
            if (s_LibUnityEndAddress < endAddr)
                s_LibUnityEndAddress = endAddr;
        }
        return 0;
    }

    static void InitKnownSymbolInfo()
    {
        s_BaseAddress = reinterpret_cast<uintptr_t>(os::Image::GetImageBase());
        s_EndAddress = reinterpret_cast<uintptr_t>(&end);

        dl_iterate_phdr(libUnityLookupCallback, NULL);
    }

    static bool KnownSymbol(const uintptr_t addr)
    {
        pthread_once(&s_InitKnownSymbolInfoOnceFlag, &InitKnownSymbolInfo);

        if (addr >= s_BaseAddress && addr <= s_EndAddress)
            return true;

        if (addr >= s_LibUnityBaseAddress && addr <= s_LibUnityEndAddress)
            return true;

        return false;
    }

    struct AndroidStackTrace
    {
        size_t size;
        Il2CppMethodPointer addrs[kMaxStackFrames];

        bool PushStackFrameAddress(const uintptr_t addr)
        {
            if (size >= kMaxStackFrames)
                return false;

            addrs[size++] = reinterpret_cast<Il2CppMethodPointer>(addr);
            return true;
        }

        static _Unwind_Reason_Code Callback(struct _Unwind_Context* context, void* self)
        {
            const uintptr_t addr = _Unwind_GetIP(context);

            // Workaround to avoid crash when generating stack trace in some third-party libraries
            if (!KnownSymbol(addr))
                return _URC_END_OF_STACK;

            if (static_cast<AndroidStackTrace*>(self)->PushStackFrameAddress(addr))
                return _URC_NO_REASON;
            else
                return _URC_END_OF_STACK;
        }
    };
}

    void StackTrace::WalkStackNative(WalkStackCallback callback, void* context, WalkOrder walkOrder)
    {
        AndroidStackTrace callstack = {};
        _Unwind_Backtrace(AndroidStackTrace::Callback, &callstack);
        for (size_t i = 0; i < callstack.size; ++i)
        {
            const size_t index = (walkOrder == kFirstCalledToLastCalled) ? (callstack.size - i - 1) : i;
            if (!callback(callstack.addrs[index], context))
                break;
        }
    }

    std::string StackTrace::NativeStackTrace()
    {
        return std::string();
    }

    const void* StackTrace::GetStackPointer()
    {
        return __builtin_frame_address(0);
    }
}
}

#endif
