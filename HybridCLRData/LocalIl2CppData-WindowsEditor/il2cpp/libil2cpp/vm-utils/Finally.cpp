#include "il2cpp-config.h"
#include "Finally.h"
#include "vm/Exception.h"

NORETURN void il2cpp::utils::RethrowException(Il2CppException* exception)
{
    vm::Exception::Rethrow(exception);
}
