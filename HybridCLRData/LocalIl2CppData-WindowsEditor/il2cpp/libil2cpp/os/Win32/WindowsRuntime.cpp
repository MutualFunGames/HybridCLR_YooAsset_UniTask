#include "il2cpp-config.h"

#if IL2CPP_TARGET_WINDOWS && !IL2CPP_USE_GENERIC_WINDOWSRUNTIME

#include "il2cpp-class-internals.h"
#include "il2cpp-string-types.h"
#include "os/WindowsRuntime.h"
#include "utils/Expected.h"
#include "utils/Il2CppError.h"
#include "utils/StringUtils.h"
#include "vm/CCW.h"
#include "WindowsHeaders.h"

#include <roerrorapi.h>

namespace il2cpp
{
namespace os
{
    il2cpp_hresult_t WindowsRuntime::GetActivationFactory(Il2CppHString className, Il2CppIActivationFactory** activationFactory)
    {
        IL2CPP_ASSERT(className != NULL);
        IL2CPP_ASSERT(activationFactory != NULL);

        return RoGetActivationFactory(reinterpret_cast<HSTRING>(className), reinterpret_cast<const IID&>(Il2CppIActivationFactory::IID), reinterpret_cast<void**>(activationFactory));
    }

    il2cpp_hresult_t WindowsRuntime::CreateHStringReference(const utils::StringView<Il2CppNativeChar>& str, Il2CppHStringHeader* header, Il2CppHString* hstring)
    {
        IL2CPP_ASSERT(header != NULL);
        IL2CPP_ASSERT(hstring != NULL);

        if (str.Length() == 0)
        {
            *hstring = NULL;
            return S_OK;
        }

        return WindowsCreateStringReference(str.Str(), static_cast<uint32_t>(str.Length()), reinterpret_cast<HSTRING_HEADER*>(header), reinterpret_cast<HSTRING*>(hstring));
    }

    il2cpp_hresult_t WindowsRuntime::CreateHString(const utils::StringView<Il2CppChar>& str, Il2CppHString* hstring)
    {
        IL2CPP_ASSERT(str.Str() != NULL || str.Length() == 0);

        if (str.Length() == 0)
        {
            *hstring = NULL;
            return S_OK;
        }

        return WindowsCreateString(str.Str(), static_cast<uint32_t>(str.Length()), reinterpret_cast<HSTRING*>(hstring));
    }

    il2cpp_hresult_t WindowsRuntime::DuplicateHString(Il2CppHString hstring, Il2CppHString* duplicated)
    {
        return WindowsDuplicateString(reinterpret_cast<HSTRING>(hstring), reinterpret_cast<HSTRING*>(duplicated));
    }

    il2cpp_hresult_t WindowsRuntime::DeleteHString(Il2CppHString hstring)
    {
        if (hstring == NULL)
            return IL2CPP_S_OK;

        return WindowsDeleteString(reinterpret_cast<HSTRING>(hstring));
    }

    utils::Expected<const Il2CppChar*> WindowsRuntime::GetHStringBuffer(Il2CppHString hstring, uint32_t* length)
    {
        return WindowsGetStringRawBuffer(reinterpret_cast<HSTRING>(hstring), length);
    }

    utils::Expected<const Il2CppNativeChar*> WindowsRuntime::GetNativeHStringBuffer(Il2CppHString hstring, uint32_t* length)
    {
        return GetHStringBuffer(hstring, length);
    }

    utils::Expected<il2cpp_hresult_t> WindowsRuntime::PreallocateHStringBuffer(uint32_t length, Il2CppNativeChar** mutableBuffer, void** bufferHandle)
    {
        return WindowsPreallocateStringBuffer(length, mutableBuffer, reinterpret_cast<HSTRING_BUFFER*>(bufferHandle));
    }

    utils::Expected<il2cpp_hresult_t> WindowsRuntime::PromoteHStringBuffer(void* bufferHandle, Il2CppHString* hstring)
    {
        return WindowsPromoteStringBuffer(static_cast<HSTRING_BUFFER>(bufferHandle), reinterpret_cast<HSTRING*>(hstring));
    }

    utils::Expected<il2cpp_hresult_t> WindowsRuntime::DeleteHStringBuffer(void* bufferHandle)
    {
        return WindowsDeleteStringBuffer(static_cast<HSTRING_BUFFER>(bufferHandle));
    }

    Il2CppIRestrictedErrorInfo* WindowsRuntime::GetRestrictedErrorInfo()
    {
        Il2CppIRestrictedErrorInfo* errorInfo;
        HRESULT hr;

        hr = ::GetRestrictedErrorInfo(reinterpret_cast<IRestrictedErrorInfo**>(&errorInfo));

        if (FAILED(hr))
            return NULL;

        return errorInfo;
    }

    void WindowsRuntime::OriginateLanguageException(il2cpp_hresult_t hresult, Il2CppException* ex, Il2CppString* exceptionString, GetOrCreateFunc createCCWCallback)
    {
        utils::StringView<Il2CppNativeChar> message(utils::StringUtils::GetChars(exceptionString), utils::StringUtils::GetLength(exceptionString));
        Il2CppHString messageHString;
        Il2CppHStringHeader unused;
        CreateHStringReference(message, &unused, &messageHString);

        Il2CppIUnknown* exceptionCCW = createCCWCallback(reinterpret_cast<Il2CppObject*>(ex), Il2CppIUnknown::IID);
        RoOriginateLanguageException(hresult, reinterpret_cast<HSTRING>(messageHString), reinterpret_cast<IUnknown*>(exceptionCCW));
        exceptionCCW->Release();
    }

    void WindowsRuntime::EnableErrorReporting()
    {
        il2cpp_hresult_t hr = RoSetErrorReportingFlags(RO_ERROR_REPORTING_USESETERRORINFO);
        IL2CPP_ASSERT(IL2CPP_HR_SUCCEEDED(hr) && "RoSetErrorReportingFlags failed");
    }
}
}

#endif
