#pragma once

#if IL2CPP_TARGET_WINDOWS

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#define INC_OLE2 1
#include <Windows.h>
#include <Objidl.h>

#include <wincrypt.h>
#include <winsock2.h>
#include <Iphlpapi.h>

#include <roapi.h>
#include <robuffer.h>
#include <winstring.h>

#endif
