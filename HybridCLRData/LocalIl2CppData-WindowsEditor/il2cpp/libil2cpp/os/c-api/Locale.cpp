#include "il2cpp-config.h"
#include "os/c-api/il2cpp-config-platforms.h"

#include "os/Locale.h"
#include "Allocator.h"

#include <string>

extern "C"
{
    void UnityPalLocaleInitialize()
    {
        il2cpp::os::Locale::Initialize();
    }

    char* UnityPalGetLocale()
    {
        return Allocator::CopyToAllocatedStringBuffer(il2cpp::os::Locale::GetLocale());
    }
}
