//
// author: Kazys Stepanas
//
#ifndef _3ESOBJECTID_H_
#define _3ESOBJECTID_H_

#include "3es-core.h"

#include <cstdint>

namespace tes
{
  /// A helper class for generating object IDs for use with TES macros.
  ///
  /// The ObjectID converts a variety of integer types to the correct width for an object ID. It also handles converting
  /// a pointer into an ID, which is expected to be the most common usage.
  class ObjectID
  {
  public:
    /// Empty constructor; zero (transient) ID.
    ObjectID() = default;
    /// Copy constructor.
    /// @param other Object to duplicate the ID of.
    ObjectID(const ObjectID &other) : _id(other._id) {}
    /// @overload
    explicit ObjectID(char id) : _id(uint32_t(id)) {}
    /// @overload
    explicit ObjectID(unsigned char id) : _id(uint32_t(id)) {}
    /// @overload
    explicit ObjectID(short id) : _id(uint32_t(id)) {}
    /// @overload
    explicit ObjectID(unsigned short id) : _id(uint32_t(id)) {}
    /// Integer type constructor.
    /// @param id The ID value to assume. This value may be truncated or sign converted (for negative values).
    explicit ObjectID(int id) : _id(uint32_t(id)) {}
    /// @overload
    explicit ObjectID(unsigned id) : _id(id) {}
    /// @overload
    explicit ObjectID(long long id) : _id(uint32_t(id)) {}
    /// @overload
    explicit ObjectID(unsigned long long id) : _id(uint32_t(id)) {}
    /// @overload
    explicit ObjectID(const void *ptr) : _id(uint32_t(reinterpret_cast<size_t>(ptr))) {}

    /// Accesor for the ID value.
    /// @return The captured ID value.
    inline uint32_t id() const { return _id; }

    /// Type cast operator to access the ID value.
    /// @return The captured ID value.
    inline operator uint32_t() { return _id; }

  private:
    uint32_t _id = 0;
  };
}

#endif // _3ESOBJECTID_H_
