#include "il2cpp-config.h"

#if IL2CPP_TARGET_WINDOWS && IL2CPP_HAS_OS_SYNCHRONIZATION_CONTEXT

#include "os/SynchronizationContext.h"
#include "os/WindowsRuntime.h"
#include "vm/Class.h"
#include "vm/Exception.h"
#include "vm/RCW.h"
#include "WindowsHelpers.h"

#ifndef WINDOWS_SDK_BUILD_VERSION
#error "We need to know which Windows SDK version we are compiling against!"
#endif

#include <windows.System.h>
#include <windows.ui.core.h>
#include <wrl.h>

using il2cpp::os::SynchronizationContext;
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Wrappers::HStringReference;

template<typename T>
using AgileCallback = Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, T, Microsoft::WRL::FtmBase>;

#if !IL2CPP_TARGET_WINDOWS_DESKTOP
ComPtr<ABI::Windows::UI::Core::ICoreWindowStatic> s_CoreWindowStatics;
#endif

ComPtr<ABI::Windows::System::IDispatcherQueueStatics> s_DispatcherQueueStatics;

Il2CppObject* SynchronizationContext::GetForCurrentThread()
{
    HRESULT hr;

#if !IL2CPP_TARGET_WINDOWS_DESKTOP
    if (s_CoreWindowStatics != nullptr)
    {
        ComPtr<ABI::Windows::UI::Core::ICoreWindow> currentThreadWindow;
        hr = s_CoreWindowStatics->GetForCurrentThread(&currentThreadWindow);
        if (SUCCEEDED(hr) && currentThreadWindow != nullptr)
        {
            ComPtr<ABI::Windows::UI::Core::ICoreDispatcher> dispatcher;
            hr = currentThreadWindow->get_Dispatcher(&dispatcher);
            if (SUCCEEDED(hr))
                return vm::RCW::GetOrCreateFromIInspectable(reinterpret_cast<Il2CppIInspectable*>(dispatcher.Get()), il2cpp_defaults.il2cpp_com_object_class);
        }
    }
#endif

    if (s_DispatcherQueueStatics != nullptr)
    {
        ComPtr<ABI::Windows::System::IDispatcherQueue> dispatcherQueue;
        hr = s_DispatcherQueueStatics->GetForCurrentThread(&dispatcherQueue);
        if (SUCCEEDED(hr) && dispatcherQueue != nullptr)
            return vm::RCW::GetOrCreateFromIInspectable(reinterpret_cast<Il2CppIInspectable*>(dispatcherQueue.Get()), il2cpp_defaults.il2cpp_com_object_class);
    }

    return nullptr;
}

void SynchronizationContext::Post(Il2CppObject* context, SynchronizationContextCallback callback, intptr_t arg)
{
    IL2CPP_ASSERT(vm::Class::HasParent(context->klass, il2cpp_defaults.il2cpp_com_object_class));

    HRESULT hr;
    auto dispatcherUnknown = reinterpret_cast<IUnknown*>(static_cast<Il2CppComObject*>(context)->identity);

#if !IL2CPP_TARGET_WINDOWS_DESKTOP
    ComPtr<ABI::Windows::UI::Core::ICoreDispatcher> dispatcher;
    hr = dispatcherUnknown->QueryInterface(__uuidof(dispatcher), &dispatcher);
    if (SUCCEEDED(hr))
    {
        ComPtr<ABI::Windows::Foundation::IAsyncAction> ignoredAction;
        hr = dispatcher->RunAsync(ABI::Windows::UI::Core::CoreDispatcherPriority_Normal, Callback<AgileCallback<ABI::Windows::UI::Core::IDispatchedHandler> >([callback, arg]() -> HRESULT
        {
            callback(arg);
            return S_OK;
        }).Get(), &ignoredAction);
        vm::Exception::RaiseIfFailed(hr, false);
    }
#endif

    ComPtr<ABI::Windows::System::IDispatcherQueue> dispatcherQueue;
    hr = dispatcherUnknown->QueryInterface(__uuidof(dispatcherQueue), &dispatcherQueue);
    if (SUCCEEDED(hr))
    {
        boolean ignoredResult;
        hr = dispatcherQueue->TryEnqueueWithPriority(ABI::Windows::System::DispatcherQueuePriority_Normal, Callback<AgileCallback<ABI::Windows::System::IDispatcherQueueHandler> >([callback, arg]() -> HRESULT
        {
            callback(arg);
            return S_OK;
        }).Get(), &ignoredResult);
        vm::Exception::RaiseIfFailed(hr, false);
    }
}

void SynchronizationContext::Initialize()
{
#if !IL2CPP_TARGET_WINDOWS_DESKTOP
    RoGetActivationFactory(HStringReference(L"Windows.UI.Core.CoreWindow").Get(), __uuidof(s_CoreWindowStatics), &s_CoreWindowStatics);
#endif

    RoGetActivationFactory(HStringReference(L"Windows.System.DispatcherQueue").Get(), __uuidof(s_DispatcherQueueStatics), &s_DispatcherQueueStatics);
}

void SynchronizationContext::Shutdown()
{
#if !IL2CPP_TARGET_WINDOWS_DESKTOP
    s_CoreWindowStatics = nullptr;
#endif

    s_DispatcherQueueStatics = nullptr;
}

#endif // IL2CPP_TARGET_WINDOWS && IL2CPP_HAS_OS_SYNCHRONIZATION_CONTEXT
