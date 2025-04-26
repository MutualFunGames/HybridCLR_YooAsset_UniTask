#include "il2cpp-config.h"

#if IL2CPP_TARGET_WINRT

#include "os/Win32/WindowsHeaders.h"

#include <string>
#include <windows.system.profile.h>
#include <windows.system.userprofile.h>

#include "os/Mutex.h"
#include "SynchronousOperation.h"
#include "utils/Il2CppHashMap.h"
#include "utils/Memory.h"
#include "utils/StringUtils.h"
#include "Win32ApiSharedEmulation.h"
#include "Win32ApiWinRTEmulation.h"

#include "Baselib.h"
#include "Cpp/ReentrantLock.h"

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System::Profile;
using namespace ABI::Windows::System::UserProfile;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace il2cpp::winrt;

extern "C"
{
    BOOL WINAPI GetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer)
    {
#define ERROR_CHECK(hr) do { if (FAILED(hr)) { SetLastError(WIN32_FROM_HRESULT(hr)); return FALSE; } } while (false)

        ComPtr<IUserInformationStatics> info;
        auto hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_System_UserProfile_UserInformation).Get(), __uuidof(info), &info);
        ERROR_CHECK(hr);

        boolean isAccessAllowed;
        hr = info->get_NameAccessAllowed(&isAccessAllowed);
        ERROR_CHECK(hr);

        if (!isAccessAllowed)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }

        ComPtr<IAsyncOperation<HSTRING> > op;
        hr = info->GetDisplayNameAsync(&op);
        ERROR_CHECK(hr);

        HString name;
        hr = MakeSynchronousOperation(op.Get())->GetResults(name.GetAddressOf());
        ERROR_CHECK(hr);

#undef ERROR_CHECK

        return CopyHStringToBuffer(name, lpBuffer, pcbBuffer);
    }
} // extern "C"

#endif
