//
// Project configuration header. This is a generated header; do not modify
// it directly. Instead, modify the config.h.in version and run CMake again.
//
#ifndef TES_CORE_CONFIG_H
#define TES_CORE_CONFIG_H

#include "CoreExport.h"

#include <3escore/Meta.h>

#ifdef TES_STATIC
/// Macro for defining extern templates. Only use extern for static library.
#define TES_EXTERN extern
#else  // TES_STATIC
#define TES_EXTERN
#endif  // TES_STATIC

// Version setup.
/// Major version number.
#define TES_VERSION_MAJOR @TES_VERSION_MAJOR@ // NOLINT(modernize-macro-to-enum)
/// Minor version number.
#define TES_VERSION_MINOR @TES_VERSION_MINOR@ // NOLINT(modernize-macro-to-enum)
/// Patch version number.
#define TES_VERSION_PATCH @TES_VERSION_PATCH@ // NOLINT(modernize-macro-to-enum)
/// Version string.
#define TES_VERSION "@TES_VERSION@"

/// @def TES_EXCEPTIONS
/// Throw @c tes::Exception when enabled, otherwise log an error and continue.
// NOLINTNEXTLINE(modernize-macro-to-enum)
#cmakedefine01 TES_EXCEPTIONS

namespace tes
{
/// Version number enum.
enum class Version : int
{
  /// Major version number.
  Major = @TES_VERSION_MAJOR@,
  /// Minor version number.
  Minor = @TES_VERSION_MINOR@,
  /// Patch version number.
  Patch = @TES_VERSION_PATCH@ 
};
}  // namespace tes

// Force MSVC to define useful things like M_PI.
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES // NOLINT(bugprone-reserved-identifier)
#endif  // _USE_MATH_DEFINES

// For MSVC to skip defining min/max as macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif  // NOMINMAX
#ifndef NOMINMAX
#define NOMINMAX
#endif  // NOMINMAX

#ifdef _MSC_VER
// Avoid dubious security warnings for plenty of legitimate code
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS 1 // NOLINT(bugprone-reserved-identifier)
#endif  // _SCL_SECURE_NO_WARNINGS
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1 // NOLINT(bugprone-reserved-identifier)
#endif  // _CRT_SECURE_NO_WARNINGS
// #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif  // _MSC_VER

// Include standard headers to ensure we effect the configuration above.
#include <cmath>
// #include <cstddef>

/// @def TES_ZLIB
/// Use ZLIB when defined.
#cmakedefine TES_ZLIB

// Define the local Endian and the network Endian
#define TES_IS_BIG_ENDIAN @TES_IS_BIG_ENDIAN@ // NOLINT(modernize-macro-to-enum)
#define TES_IS_NETWORK_ENDIAN @TES_IS_BIG_ENDIAN@ // NOLINT(modernize-macro-to-enum)

// Define assertion usages.
/// @def TES_ASSERT_ENABLE_DEBUG
/// Enable debug mode assertions when defined.
// NOLINTNEXTLINE(modernize-macro-to-enum)
#cmakedefine01 TES_ASSERT_ENABLE_DEBUG
/// @def TES_ASSERT_ENABLE_RELEASE
/// Enable release mode assertions when defined.
// NOLINTNEXTLINE(modernize-macro-to-enum)
#cmakedefine01 TES_ASSERT_ENABLE_RELEASE

#if defined(NDEBUG) && TES_ASSERT_ENABLE_RELEASE || !defined(NDEBUG) && TES_ASSERT_ENABLE_DEBUG
#define TES_ASSERT_ENABLE 1
#endif  // defined(NDEBUG) && TES_ASSERT_ENABLE_RELEASE || !defined(NDEBUG) && TES_ASSERT_ENABLE_DEBUG

// Define the word size (in bits)
/// @def TES_32
/// Defines when on a 32-bit platform.
#cmakedefine TES_32 
/// @def TES_64
/// Defines when on a 64-bit platform.
#cmakedefine TES_64

// Define a useful printf format string for size_t
/// #def TES_ZU
/// Defines a printf format specifier suitable for use with size_t.
#ifdef TES_64
#if defined(_MSC_VER)
#define TES_ZU "%Iu"
#else
#define TES_ZU "%zu"
#endif
#else  // TES_64
#define TES_ZU "%u"
#endif  // TES_64

#endif  // TES_CORE_CONFIG_H
