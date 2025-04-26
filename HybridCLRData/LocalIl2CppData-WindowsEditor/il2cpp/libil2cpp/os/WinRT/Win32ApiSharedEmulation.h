#pragma once

#if IL2CPP_TARGET_WINRT

#include "os/Win32/WindowsHeaders.h"
#include <wrl.h>

namespace il2cpp
{
namespace winrt
{
    inline DWORD WIN32_FROM_HRESULT(HRESULT hr)
    {
        if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0))
            return HRESULT_CODE(hr);
        if (hr == S_OK)
            return HRESULT_CODE(hr);
        return ERROR_SUCCESS;
    }

    inline static BOOL CopyHStringToBuffer(Microsoft::WRL::Wrappers::HString& source, LPWSTR target, LPDWORD targetSize)
    {
        unsigned int sourceLength;
        auto sourceBuffer = source.GetRawBuffer(&sourceLength);

        if (sourceLength + 1 > *targetSize)
        {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            *targetSize = sourceLength + 1;
            return FALSE;
        }

        *targetSize = sourceLength;

        if (target != nullptr)
        {
            memcpy(target, sourceBuffer, sourceLength * sizeof(wchar_t));
            target[sourceLength] = L'\0';
            return TRUE;
        }

        return FALSE;
    }
}
}

#if IL2CPP_TARGET_WINRT

extern "C"
{
    DWORD WINAPI GetIfEntry(PMIB_IFROW pIfRow);
} // extern "C"

#endif // IL2CPP_TARGET_WINRT

#endif
