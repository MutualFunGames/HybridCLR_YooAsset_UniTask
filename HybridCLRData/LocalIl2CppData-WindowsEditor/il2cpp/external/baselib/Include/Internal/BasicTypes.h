#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Custom type suitable for representing a UTF-16 codepoint crossplatform.
// Because char16_t is not available on all platforms,
// uint16_t is chosen as a type that inflicts the same behavior across platforms,
// as is requiring a cast from platform specific UTF-16 representation.
typedef uint16_t baselib_char16_t;
