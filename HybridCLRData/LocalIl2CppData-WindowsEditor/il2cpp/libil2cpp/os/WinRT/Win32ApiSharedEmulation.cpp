#include "il2cpp-config.h"

#if IL2CPP_TARGET_WINRT

#include "Win32ApiSharedEmulation.h"

extern "C"
{
    // Provide a dummy GetIfEntry for WinRT. This is used by the class library
    // code to implement GetAllNetworkInterfaces(). It looks like the values
    // returned though are never actually used. So this dummy implementation seems
    // to be enough for the class library code to work in WinRT.
    DWORD WINAPI GetIfEntry(PMIB_IFROW pIfRow)
    {
        memset(pIfRow, 0, sizeof(MIB_IFROW));
        return NO_ERROR;
    }
} // extern "C"

#endif // IL2CPP_TARGET_WINRT
