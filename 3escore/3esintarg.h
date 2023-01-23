//
// author: Kazys Stepanas
//
#ifndef _3ESINTARG_H_
#define _3ESINTARG_H_

#include "CoreConfig.h"

#include "3esassertrange.h"

namespace tes
{
/// A helper structure for handling integer arguments of various types without generating compiler warnings.
///
/// This is intended primarily for size_t arguments from std::vectors::size() calls
/// passed to things like the @c MesShape or @c SimpleMesh. The argument may be given as @c int,
/// @c unsigned, or @c size_t and converted to the same. A conversion which will
/// lose information generates a runtime error.
///
/// Cast operators are supplied to support casting down to the specific required type.
template <typename INT>
struct IntArgT
{
  typedef INT ValueType;

  /// Constructor from @c int.
  /// @param s The argument value.
  inline IntArgT(int ii)
    : i(INT(ii))
  {
    AssertRange<INT, int>()(ii);
  }
  /// Constructor from @c unsigned.
  /// @param s The argument value.
  inline IntArgT(unsigned ii)
    : i(INT(ii))
  {
    AssertRange<INT, unsigned>()(ii);
  }

#ifdef TES_64
  /// Constructor from @c size_t.
  /// @param s The argument value.
  inline IntArgT(size_t ii)
    : i(INT(ii))
  {
    AssertRange<INT, size_t>()(ii);
  }
#endif  // TES_64

  /// Boolean cast operator.
  /// @return @c true if @c i is non-zero.
  inline operator bool() const { return i != 0; }

  /// Convert to @c int. Raised an error if @c i is too large.
  /// @return The size as an integer.
  inline operator INT() const { return i; }

  /// The stored size value.
  INT i;
};

template struct TES_CORE_API IntArgT<int>;
typedef IntArgT<int> IntArg;
template struct TES_CORE_API IntArgT<unsigned>;
typedef IntArgT<unsigned> UIntArg;
#ifdef TES_64
template struct TES_CORE_API IntArgT<size_t>;
typedef IntArgT<size_t> SizeTArg;
#else   // TES_64
typedef UIntArg SizeTArg;
#endif  // TES_64
}  // namespace tes

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define _TES_INTARG_BOOL_OP_SELF(INTARG, OP)                                       \
  inline bool operator OP(const INTARG &a, const INTARG &b)                        \
  {                                                                                \
    return static_cast<INTARG::ValueType>(a) OP static_cast<INTARG::ValueType>(b); \
  }
#define _TES_INTARG_ARITH_OP_SELF(INTARG, OP)                                      \
  inline INTARG::ValueType operator OP(const INTARG &a, const INTARG &b)           \
  {                                                                                \
    return static_cast<INTARG::ValueType>(a) OP static_cast<INTARG::ValueType>(b); \
  }

#define _TES_INTARG_BOOL_OP(INT, INTARG, OP)                                           \
  inline bool operator OP(INT a, const INTARG &b) { return a OP static_cast<INT>(b); } \
  inline bool operator OP(const INTARG &a, INT b) { return static_cast<INT>(a) OP b; }
// #define _TES_INTARG_ARITH_OP(INT, INTARG, OP)
//  inline INT operator OP(INT a, const INTARG &b) { return a OP static_cast<INT>(b); }
//  inline INT operator OP(const INTARG &a, INT b) { return static_cast<INT>(a) OP b; }

// Comparison operators for @c IntArg and similar utilities.
#define TES_INTARG_OPERATORS(INT, INTARG) \
  _TES_INTARG_BOOL_OP(INT, INTARG, <)     \
  _TES_INTARG_BOOL_OP(INT, INTARG, <=)    \
  _TES_INTARG_BOOL_OP(INT, INTARG, >)     \
  _TES_INTARG_BOOL_OP(INT, INTARG, >=)    \
  _TES_INTARG_BOOL_OP(INT, INTARG, ==)    \
  _TES_INTARG_BOOL_OP(INT, INTARG, !=)
   // _TES_INTARG_ARITH_OP(INT, INTARG, +)
   // _TES_INTARG_ARITH_OP(INT, INTARG, -)
   // _TES_INTARG_ARITH_OP(INT, INTARG, *)
   // _TES_INTARG_ARITH_OP(INT, INTARG, /)

#define TES_INTARG_OPERATORS_SELF(INTARG) \
  _TES_INTARG_BOOL_OP_SELF(INTARG, <)     \
  _TES_INTARG_BOOL_OP_SELF(INTARG, <=)    \
  _TES_INTARG_BOOL_OP_SELF(INTARG, >)     \
  _TES_INTARG_BOOL_OP_SELF(INTARG, >=)    \
  _TES_INTARG_BOOL_OP_SELF(INTARG, ==)    \
  _TES_INTARG_BOOL_OP_SELF(INTARG, !=)    \
  _TES_INTARG_ARITH_OP_SELF(INTARG, +)    \
  _TES_INTARG_ARITH_OP_SELF(INTARG, -)    \
  _TES_INTARG_ARITH_OP_SELF(INTARG, *)    \
  _TES_INTARG_ARITH_OP_SELF(INTARG, /)

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif  // __GNUC__
TES_INTARG_OPERATORS(int, tes::IntArg);
TES_INTARG_OPERATORS_SELF(tes::IntArg);
TES_INTARG_OPERATORS(unsigned, tes::UIntArg);
TES_INTARG_OPERATORS_SELF(tes::UIntArg);
#ifdef TES_64
TES_INTARG_OPERATORS(size_t, tes::SizeTArg);
TES_INTARG_OPERATORS_SELF(tes::SizeTArg);
#endif // TES_64
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

#endif  // DOXYGEN_SHOULD_SKIP_THIS

#endif  // _3ESINTARG_H_
