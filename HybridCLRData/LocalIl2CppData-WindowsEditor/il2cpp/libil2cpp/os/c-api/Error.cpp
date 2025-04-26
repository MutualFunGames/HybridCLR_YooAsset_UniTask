#include "os/c-api/il2cpp-config-platforms.h"

#include "os/Error.h"
#include "os/c-api/Error-c-api.h"

extern "C"
{
    int32_t UnityPalSuccess(UnityPalErrorCode code)
    {
        return (int32_t)(code == il2cpp::os::kErrorCodeSuccess);
    }
}
