//
// author: Kazys Stepanas
//
#ifndef _3ESSHAPEID_H
#define _3ESSHAPEID_H

#include "3es-core.h"

#include <cinttypes>

namespace tes
{
/// A shape identifier and category.
///
/// A zero ID represents a transient shape (lasting a single frame), while a non zero ID shape will persist until
/// explicitly destroyed. The ID must be unique for the particular shape type, but shapes of different types may share
/// IDs. Zero ID shapes (transient) are nevery unique identified.
///
/// An @c ShapeId may also be constructed from a pointer value as a convenient way to generate a unique shape ID.
///
/// Note; the id 0xffffffu is reserved.
class ShapeId
{
public:
  ShapeId(size_t id = 0u, uint16_t category = 0u)
    : _id(uint32_t(id))
    , _category(category)
  {}

  // ShapeId(const void *id_ptr, uint16_t category = 0)
  //   : _category(category)
  // {
  //   setId(id_ptr);
  // }

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
}  // namespace tes

#endif  // _3ESSHAPEID_H
