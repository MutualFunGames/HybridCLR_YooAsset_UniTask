#pragma once
#include <fileapifromapp.h>

#if IL2CPP_TARGET_WINRT

extern "C"
{
#define GetUserName GetUserNameW

    inline HANDLE WINAPI CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
    {
        return CreateFileFromAppW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    BOOL WINAPI GetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer);

#define CreateFileMappingW(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName) \
    CreateFileMappingFromApp(hFile, lpFileMappingAttributes, flProtect, (static_cast<ULONG64>(dwMaximumSizeHigh) << 32) | dwMaximumSizeLow, lpName);

#define MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap) \
    MapViewOfFileFromApp(hFileMappingObject, dwDesiredAccess, (static_cast<ULONG64>(dwFileOffsetHigh) << 32) | dwFileOffsetLow, dwNumberOfBytesToMap);
} // extern "C"

#endif
