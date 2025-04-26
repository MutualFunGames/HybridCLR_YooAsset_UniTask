#include "il2cpp-config.h"

#include "icalls/mscorlib/System/RuntimeMethodHandle.h"
#include "vm/Exception.h"
#include "vm/Method.h"
#include "vm/Runtime.h"
#include "vm/MetadataCache.h"

namespace il2cpp
{
namespace icalls
{
namespace mscorlib
{
namespace System
{
    intptr_t RuntimeMethodHandle::GetFunctionPointer(intptr_t m)
    {
        MethodInfo* method = reinterpret_cast<MethodInfo*>(m);
        if (method->is_unmanaged_callers_only)
            return reinterpret_cast<intptr_t>(il2cpp::vm::MetadataCache::GetReversePInvokeWrapper(method->klass->image, method));
        return reinterpret_cast<intptr_t>(method);
    }
} /* namespace System */
} /* namespace mscorlib */
} /* namespace icalls */
} /* namespace il2cpp */
