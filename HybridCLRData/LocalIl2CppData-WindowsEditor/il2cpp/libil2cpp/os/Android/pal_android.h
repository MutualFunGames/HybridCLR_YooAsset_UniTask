#pragma once

#include "il2cpp-config.h"

#if IL2CPP_TARGET_ANDROID

#define IL2CPP_USES_POSIX_CLASS_LIBRARY_PAL 1

// On Android >= 23, strerror_r changed from POSIX to GNU variant.
// see https://android.googlesource.com/platform/bionic/+/master/libc/include/string.h#140 for details.
#if __ANDROID_API__ >= 23
    #define IL2CPP_HAVE_GNU_STRERROR_R 1
#endif

#define IL2CPP_HAVE_STAT_FLAGS 1
#define IL2CPP_HAVE_LCHFLAGS 1
#define IL2CPP_HAVE_FUTIMENS 1
#define IL2CPP_HAVE_FIXED_SIZE_DIRENT 1

#define IL2CPP_HAVE_STAT_TIM 1
#include <sys/sendfile.h>
#define IL2CPP_HAVE_SENDFILE_4 1
#define IL2CPP_HAVE_SYS_UN 1

// On Android, we are not allowed to modify permissions, but the copy should still succeed;
// see https://github.com/mono/mono/issues/17133 for details.
#define IL2CPP_CANNOT_MODIFY_FILE_PERMISSIONS 1

#define stat_ stat
#define fstat_ fstat
#define lstat_ lstat

#include <dirent.h>

#endif // IL2CPP_TARGET_ANDROID
