#include "os/Assert.h"

#if IL2CPP_DEBUG

#if IL2CPP_TARGET_WINDOWS
#include <crtdbg.h>

void il2cpp_assert(const char* assertion, const char* file, unsigned int line)
{
    if (_CrtDbgReport(_CRT_ASSERT, file, line, "", "%s", assertion) == 1)
    {
        _CrtDbgBreak();
    }
}

#endif // IL2CPP_TARGET_WINDOWS

#endif // IL2CPP_DEBUG
