//
// Author: Kazys Stepanas
//

#ifndef TES_CORE_PTR
#define TES_CORE_PTR

#include "CoreConfig.h"

#include <memory>

namespace tes
{
/// An abstraction for having either a borrowed or shared pointer to something of type @c T .
///
/// The @p Ptr class is used extensively in order to support multiple use case scenarios for
/// pointers passed to @c tes objects. It enabled both shared and borrowed semantics without
/// forcing the API exclusively into one or the other.
///
/// @tparam T The type being pointed stored by address.
template <typename T>
class Ptr
{
public:
  /// Defines the status of this @c Ptr object.
  enum class Status
  {
    Empty,     ///< Empty/null pointer.
    Borrowed,  ///< Pointer is a borrowed raw pointer (semantically a reference).
    Shared,    ///< Storing a @c std::shared_ptr<T>
  };

  /// Default constructor. Results in a null pointer.
  Ptr() = default;
  /// Construct from a raw pointer resulting in @c Status::Borrowed, unless @p ptr is null.
  /// @param ptr The pointer to assign.
  Ptr(T *ptr);
  /// Construct from a shared pointer resulting in @c Status::Shared, unless @p ptr is null.
  /// @param ptr The pointer to assign.
  Ptr(std::shared_ptr<T> ptr);
  /// Move constructor.
  /// @param other Object to move.
  Ptr(Ptr<T> &&other) noexcept = default;
  /// Copy constructor.
  /// @param other Object to copy.
  Ptr(const Ptr<T> &other) = default;
  /// Copy constructor from non-const equivalent @c T to `const T`.
  /// @param other Object to copy.
  template <typename = std::enable_if_t<std::is_const_v<T>>>
  Ptr(const Ptr<std::remove_const_t<T>> &other)
  {
    *this = other;
  }
  /// Construct from a either a non-const @c shard_ptr<T> or using upcasting.
  /// @tparam U The type to cast from. Must be implicitly convertible to @c T.
  /// @param other Object to copy.
  template <typename U>
  Ptr(const std::shared_ptr<U> &ptr)
    : _shared(ptr)
  {}
  /// Construct from a either a non-const @c T* or using upcasting.
  /// @tparam U The type to cast from. Must be implicitly convertible to @c T.
  /// @param other Object to copy.
  template <typename U>
  Ptr(U *ptr)
    : _borrowed(ptr)
  {}
  /// Construct from another @c Ptr supporting upcasting.
  ///
  /// Note there is no downcasting support.
  ///
  /// @tparam U The type to cast from, must be derived from @p T .
  /// @param other The pointer to assign from.
  template <typename U>
  Ptr(const Ptr<U> &other);
  /// Destructor.
  ~Ptr() = default;

  /// Assign a raw pointer resulting in @c Status::Borrowed, unless @p ptr is null.
  /// @param ptr The pointer to assign.
  /// @return @c *this
  Ptr &operator=(T *ptr);
  /// Assign a shared pointer resulting in @c Status::Shared, unless @p ptr is null.
  /// @param ptr The pointer to assign.
  /// @return @c *this
  Ptr &operator=(std::shared_ptr<T> ptr);
  /// Move assignment.
  /// @param other Object to move
  /// @return @c *this
  Ptr &operator=(Ptr<T> &&other) noexcept = default;
  /// @brief Copy assignment.
  /// @param other Object to copy.
  /// @return @c *this
  Ptr &operator=(const Ptr<T> &other) = default;
  /// Copy assignment from non-const equivalent @c T to `const T`.
  /// @param other Object to copy.
  /// @return @c *this
  template <typename = std::enable_if_t<std::is_const_v<T>>>
  Ptr &operator=(const Ptr<std::remove_const_t<T>> &other)
  {
    _shared = other.shared();
    _borrowed = other.borrowed();
    return *this;
  }
  /// Assign from a either a non-const @c shard_ptr<T> or using upcasting.
  /// @tparam U The type to cast from. Must be implicitly convertible to @c T.
  /// @param other Object to copy.
  template <typename U>
  Ptr &operator=(const std::shared_ptr<U> &ptr)
  {
    _shared = ptr;
    _borrowed = nullptr;
    return *this;
  }
  /// Assign from a either a non-const @c T* or using upcasting.
  /// @tparam U The type to cast from. Must be implicitly convertible to @c T.
  /// @param other Object to copy.
  template <typename U>
  Ptr &operator=(U *ptr)
  {
    _shared = {};
    _borrowed = ptr;
    return *this;
  }
  /// Assignment from another @c Ptr supporting upcasting.
  ///
  /// Note there is no downcasting support.
  ///
  /// @tparam U The type to cast from, must be derived from @p T .
  /// @param other The pointer to assign from.
  /// @return @c @this
  template <typename U>
  Ptr &operator=(const Ptr<U> &other);

  /// Dereference operator.
  ///
  /// Behaviour is undefined when @c status() is @c Status::Empty.
  /// @return The dereferenced pointer.
  T &operator*() { return *get(); }
  /// Dereference operator.
  ///
  /// Behaviour is undefined when @c status() is @c Status::Empty.
  /// @return The dereferenced pointer.
  const T &operator*() const { return *get(); }

  /// Call operator.
  ///
  /// Behaviour is undefined when @c status() is @c Status::Empty.
  /// @return The underlying pointer.
  T *operator->() { return get(); }
  /// Call operator.
  ///
  /// Behaviour is undefined when @c status() is @c Status::Empty.
  /// @return The underlying pointer.
  const T *operator->() const { return get(); }

  /// Retrieve the pointer as a shared pointer.
  ///
  /// The result is a null/empty pointer when @c status() is not @c Status::Shared.
  /// @return The shared pointer.
  std::shared_ptr<T> shared() { return _shared; }
  /// Retrieve the pointer as a shared pointer.
  ///
  /// The result is a null/empty pointer when @c status() is not @c Status::Shared.
  /// @return The shared pointer.
  std::shared_ptr<const T> shared() const { return _shared; }

  /// Retrieve the pointer as a borrowed/raw pointer.
  ///
  /// The result is a null/empty pointer when @c status() is not @c Status::Borrowed.
  ///
  /// Behaviour is undefined if the source of the raw pointer has gone out of scope.
  /// @return The raw pointer.
  T *borrowed() { return _borrowed; }
  /// Retrieve the pointer as a borrowed/raw pointer.
  ///
  /// The result is a null/empty pointer when @c status() is not @c Status::Borrowed.
  ///
  /// Behaviour is undefined if the source of the raw pointer has gone out of scope.
  /// @return The raw pointer.
  const T *borrowed() const { return _borrowed; }

  /// Check if empty.
  /// @return True when empty.
  bool empty() const { return !_shared && !_borrowed; }

  /// Boolean conversion.
  /// @return True when not empty.
  operator bool() const { return !empty(); }

  /// Logical negation.
  /// @return True not empty.
  bool operator!() const { return empty(); }

  /// Equality operator.
  /// @param other Object to check equivalence with.
  /// @return True when equivalent.
  bool operator==(const Ptr<T> &other) const
  {
    return _shared == other._shared && _borrowed == other._borrowed;
  }

  /// Inequality operator.
  /// @param other Object to check equivalence with.
  /// @return False when equivalent.
  bool operator!=(const Ptr<T> &other) const { return !operator==(other); }

  /// Equality comparison with raw pointer.
  /// @param ptr Pointer to compare
  /// @return True if @p ptr matches either the @c borrowed() or @c shared() pointers.
  bool operator==(const T *ptr) { return _borrowed == ptr || _shared.get() == ptr; }

  /// Inequality comparison with raw pointer.
  /// @param ptr Pointer to compare
  /// @return False if @p ptr matches either the @c borrowed() or @c shared() pointers.
  bool operator!=(const T *ptr) { return !operator==(ptr) }

  /// Equality comparison with raw pointer.
  /// @param ptr Pointer to compare
  /// @return True if @p ptr matches either the @c borrowed() or @c shared() pointers.
  bool operator==(const std::shared_ptr<const T> &ptr)
  {
    return _borrowed == ptr.get() || _shared.get() == ptr.get();
  }

  /// Inequality comparison with raw pointer.
  /// @param ptr Pointer to compare
  /// @return False if @p ptr matches either the @c borrowed() or @c shared() pointers.
  bool operator!=(const std::shared_ptr<const T> &ptr){ return !operator==(ptr) }

  /// Get a raw pointer from either a shared or borrowed pointer.
  ///
  /// This returns either @c shared() or @c borrowed() depending on status. A null pointer is
  /// returned when @c status() is @c Status::Empty.
  /// @return The pointer or null if empty.
  T *get();
  /// Get a raw pointer from either a shared or borrowed pointer.
  ///
  /// This returns either @c shared() or @c borrowed() depending on status. A null pointer is
  /// returned when @c status() is @c Status::Empty.
  /// @return The pointer or null if empty.
  const T *get() const;

  /// Reset this pointer to a null/empty status
  void reset()
  {
    _shared.reset();
    _borrowed = nullptr;
  }

  /// Query the type of pointer being help.
  /// @return The pointer type.
  Status status() const;

private:
  // Note(KS): this seems to be the simplest and smallest implementation without using a union.
  // The initial implementation used std::any and the Ptr size was 72 bytes using the MSC compiler.
  // This layout is 24 byte - 16 for shared, 8 for raw pointer - which is a large savings. Using
  // a union could save 8 bytes, but std::shared_ptr is not POD. Despite the overhead, this layout
  // is save and (relatively) small.
  std::shared_ptr<T> _shared;
  T *_borrowed = nullptr;
};

template <typename T>
inline Ptr<T>::Ptr(T *ptr)
  : _borrowed(ptr)
{}


template <typename T>
inline Ptr<T>::Ptr(std::shared_ptr<T> ptr)
  : _shared(std::move(ptr))
{}


template <typename T>
template <typename U>
Ptr<T>::Ptr(const Ptr<U> &other)
{
  operator=(other);
}


template <typename T>
inline Ptr<T> &Ptr<T>::operator=(T *ptr)
{
  _shared.reset();
  _borrowed = ptr;
  return *this;
}


template <typename T>
inline Ptr<T> &Ptr<T>::operator=(std::shared_ptr<T> ptr)
{
  _shared = ptr;
  _borrowed = nullptr;
  return *this;
}


template <typename T>
template <typename U>
Ptr<T> &Ptr<T>::operator=(const Ptr<U> &other)
{
  reset();
  _shared = other.shared();
  _borrowed = other.borrowed();
  return *this;
}


template <typename T>
inline T *Ptr<T>::get()
{
  if (_shared)
  {
    return _shared.get();
  }
  if (_borrowed)
  {
    return _borrowed;
  }
  return nullptr;
}


template <typename T>
inline const T *Ptr<T>::get() const
{
  if (_shared)
  {
    return _shared.get();
  }
  if (_borrowed)
  {
    return _borrowed;
  }
  return nullptr;
}


template <typename T>
inline typename Ptr<T>::Status Ptr<T>::status() const
{
  if (_shared)
  {
    return Status::Shared;
  }
  if (_borrowed)
  {
    return Status::Borrowed;
  }
  return Status::Empty;
}

}  // namespace tes

#endif  // TES_CORE_PTR
