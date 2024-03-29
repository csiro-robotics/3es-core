//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_SHAPE_ID_H
#define TES_CORE_SHAPES_SHAPE_ID_H

#include <3escore/CoreConfig.h>

#include <cinttypes>

/// A helper macro for defining various explicit constructors for @c Id .
/// This is to deal with an ambituity with casting 0 to an integer or a null pointer.
#define TES_SHAPE_ID_INT_CTOR(int_type)   \
  Id(int_type id, uint16_t category = 0u) \
    : Id(uint32_t(id), category)          \
  {}

namespace tes
{
/// A shape identifier and category.
///
/// A zero ID represents a transient shape (lasting a single frame), while a non zero ID shape will persist until
/// explicitly destroyed. The ID must be unique for the particular shape type, but shapes of different types may share
/// IDs. Zero ID shapes (transient) are nevery unique identified.
///
/// An @c Id may also be constructed from a pointer value as a convenient way to generate a unique shape ID.
///
/// Note; the id 0xffffffu is reserved.
class Id
{
public:
  Id(uint32_t id = 0u, uint16_t category = 0u)
    : _id(id)
    , _category(category)
  {}

  TES_SHAPE_ID_INT_CTOR(int8_t);
  TES_SHAPE_ID_INT_CTOR(uint8_t);
  TES_SHAPE_ID_INT_CTOR(int16_t);
  TES_SHAPE_ID_INT_CTOR(uint16_t);
  TES_SHAPE_ID_INT_CTOR(int32_t);
  TES_SHAPE_ID_INT_CTOR(int64_t);
  TES_SHAPE_ID_INT_CTOR(uint64_t);

  template <typename T>
  Id(const T *id_ptr, uint16_t category = 0)
    : _category(category)
  {
    setId(id_ptr);
  }

  inline uint32_t id() const { return _id; }
  inline void setId(size_t id) { _id = uint32_t(id); }

  /// Set the @c id() from a pointer.
  ///
  /// This copies the address as an integer value. A 64-bit pointer will be truncated to 32 bits.
  ///
  /// The pointer value is not recoverable.
  /// @param id_ptr The pointer address to convert to an id value.
  inline void setId(const void *id_ptr)
  {
#ifdef TES_64
    _id = static_cast<uint32_t>(reinterpret_cast<size_t>(id_ptr));
#else   // TES_64
    _id = static_cast<size_t>(id_ptr);
#endif  // TES_64
  }

  inline uint16_t category() const { return _category; }
  inline void setCategory(uint16_t category) { _category = category; }

private:
  uint32_t _id;
  uint16_t _category;
};  // namespace tes

/// Convenience operator to increment a @c Id::id() value. Handy when basing a range of Ids off a common value
/// @param id The base @c Id object
/// @param inc The increment value
/// @return A @c Id object with the same category as @p id and an @c Id::id() value equal to `id.id() + inc`.
inline Id operator+(const Id &id, size_t inc)
{
  return Id(id.id() + inc, id.category());
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_SHAPE_ID_H
