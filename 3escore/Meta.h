//
// author: Kazys Stepanas
//
// This file contains utility macros and defines mostly used to avoid compiler
// warnings.
//
#ifndef TES_CORE_META_H
#define TES_CORE_META_H

// Do not include 3es-core for now. That would be circular.

#define TES_UNUSED(x) (void)(x)

#ifdef __GNUC__
#define TES_FALLTHROUGH [[clang::fallthrough]]
#endif  // __GNUC__


// Fall back definitions.
#ifndef TES_FALLTHROUGH
/// Use this macro at the end of a switch statement case which is to fall through without a break.
#define TES_FALLTHROUGH
#endif  // TES_FALLTHROUGH

#endif  // TES_CORE_META_H
