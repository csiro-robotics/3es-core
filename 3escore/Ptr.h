//
// Author: Kazys Stepanas
//

#include "CoreConfig.h"

#include <any>
#include <memory>

namespace tes
{
/// An abstraction for having either a borrowed or shared pointer to something of type @c T .
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

  Ptr() = default;
  explicit Ptr(T *ptr);
  explicit Ptr(std::shared_ptr<T> ptr);
  Ptr(Ptr<T> &&other) noexcept = default;
  Ptr(const Ptr<T> &other) = default;
  template <typename = std::enable_if_t<std::is_const_v<T>>>
  Ptr(const Ptr<std::remove_const_t<T>> &other)
  {
    *this = other;
  }
  ~Ptr() = default;

  Ptr &operator=(T *ptr);
  Ptr &operator=(std::shared_ptr<T> ptr);
  Ptr &operator=(Ptr<T> &&other) noexcept = default;
  Ptr &operator=(const Ptr<T> &other) = default;
  template <typename = std::enable_if_t<std::is_const_v<T>>>
  Ptr &operator=(const Ptr<std::remove_const_t<T>> &other)
  {
    if (other.status() == other.Status::Borrowed)
    {
      _ptr = const_cast<T *>(other.get());
      _status = Status::Borrowed;
    }
    else if (other.status() == other.Status::Shared)
    {
      _ptr = std::shared_ptr<T>(std::move(other.shared()));
      _status = Status::Shared;
    }
    else
    {
      _ptr = std::any();
      _status = Status::Empty;
    }
    return *this;
  }

  T &operator*() { return *get(); }
  const T &operator*() const { return *get(); }

  T *operator->() { return get(); }
  const T *operator->() const { return get(); }

  std::shared_ptr<T> shared()
  {
    return (_status == Status::Shared) ? std::any_cast<std::shared_ptr<T>>(_ptr) :
                                         std::shared_ptr<T>();
  }

  std::shared_ptr<T> shared() const
  {
    return (_status == Status::Shared) ? std::any_cast<std::shared_ptr<T>>(_ptr) :
                                         std::shared_ptr<T>();
  }

  T *get();
  const T *get() const;
  Status status() const { return _status; }

private:
  std::any _ptr;
  Status _status = Status::Empty;
};

template <typename T>
Ptr<T>::Ptr(T *ptr)
  : _ptr(ptr)
  , _status(Status::Borrowed)
{}


template <typename T>
Ptr<T>::Ptr(std::shared_ptr<T> ptr)
  : _ptr(std::move(ptr))
  , _status(Status::Shared)
{}


template <typename T>
Ptr<T> &Ptr<T>::operator=(T *ptr)
{
  _ptr = ptr;
  _status = Status::Borrowed;
  return *this;
}


template <typename T>
Ptr<T> &Ptr<T>::operator=(std::shared_ptr<T> ptr)
{
  _ptr = ptr;
  _status = Status::Shared;
  return *this;
}


template <typename T>
T *Ptr<T>::get()
{
  if (_status == Status::Borrowed)
  {
    return std::any_cast<T *>(_ptr);
  }
  if (_status == Status::Shared)
  {
    return std::any_cast<std::shared_ptr<T>>(_ptr).get();
  }
  return nullptr;
}


template <typename T>
const T *Ptr<T>::get() const
{
  if (_status == Status::Borrowed)
  {
    return std::any_cast<T *>(_ptr);
  }
  if (_status == Status::Shared)
  {
    return std::any_cast<const std::shared_ptr<T>>(_ptr).get();
  }
  return nullptr;
}
}  // namespace tes
