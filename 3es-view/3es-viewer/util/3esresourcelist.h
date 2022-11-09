#ifndef TES_VIEWER_UTIL_RESOURCE_LIST_H
#define TES_VIEWER_UTIL_RESOURCE_LIST_H

#include "3es-viewer.h"

#include <mutex>
#include <utility>
#include <vector>

namespace tes::viewer::util
{
using ResourceListId = size_t;

/// A resource list is a container which assigns items from it's internal buffer - resources - for external usage.
///
/// Such resource items may be released back to the @c ResourceList where they are added to a free item list and may
/// be used in future resource assignments.
///
/// Resources are assigned by @c Id and such an id must be dereferenced every time a resource item is to be accessed.
/// This is because allocating new resource may reallocate the internal buffer invalidating any resources currently held
/// externally to this class.
///
/// A @c ResourceRef can be used as a kind of resource lock which ensures the @c ResourceList cannot invalidate items.
/// As such a @c ResourceRef must be short lived and no new resources can be assigned while at least one @c ResourceRef
/// is held.
///
/// @tparam T The resource type.
template <typename T>
class ResourceList
{
public:
  /// The type used to identify resources. This maps to indices in the items list.
  using Id = ResourceListId;

  /// A null item marker. Internally used to identify the end of the free list or other linked list structures.
  static constexpr Id kNull = ~Id(0u);
  /// Marker value used for items which are currently allocated.
  static constexpr Id kAllocated = ~Id(0u) - 1u;


  /// Represents a transient reference to an item in the @c ResourceList .
  ///
  /// @c ResourceRefBase objects are obtained via @c allocate() and @c Id indexing functions ensures that the resource
  /// remains valid for the lifespan on the @c ResourceRefBase object. This includes locking the @c ResourceList for the
  /// current thread, thus only one thread at a time can hold any @c ResourceRefBase objects at a time.
  ///
  /// The resource should only be accessed using @c * and @c -> operators as these accessors remain valid even if
  /// @c allocate() causes the resource list to reallocate.
  ///
  /// @note A @c ResourceList must outlive all its @c ResourceRefBase objects.
  template <typename Item, typename List>
  class ResourceRefBase
  {
  public:
    /// Default constructor: the resulting object is not valid.
    inline ResourceRefBase() = default;
    /// Construct a resource for the given @p id and @p resource_list .
    /// @param id The resource Id.
    /// @param resource_list The resource list which we are referencing into.
    inline ResourceRefBase(Id id, List *resource_list)
      : _id(id)
      , _resource_list(resource_list)
    {
      _resource_list->lock();
    }
    inline ResourceRefBase(const ResourceRefBase<Item, List> &) = delete;
    /// Move constructor.
    /// @param other Object to move.
    inline ResourceRefBase(ResourceRefBase<Item, List> &&other)
      : _id(std::exchange(other._id, kNull))
      , _resource_list(std::exchange(other._resource_list, nullptr))
    {}

    /// Releases the resource reference, releasing a @c ResourceList lock.
    inline ~ResourceRefBase()
    {
      if (_resource_list)
      {
        _id = kNull;
        _resource_list->unlock();
      }
    }

    inline ResourceRefBase &operator=(const ResourceRefBase<Item, List> &) = delete;
    /// Move assignment.
    /// @param other Object to move; can match @c this .
    /// @return @c *this
    inline ResourceRefBase &operator=(ResourceRefBase<Item, List> &&other)
    {
      if (this != &other)
      {
        std::swap(other._id, _id);
        std::swap(other._resource_list, _resource_list);
      }
      return *this;
    }

    /// Check if this resource reference is valid. A valid reference has a valid @c Id and addresses a @c ResourceList .
    /// @return
    inline bool isValid() const { return _id != kNull && _resource_list != nullptr; }

    /// Dereference the resource.
    /// @return The references resource entry.
    inline const Item &operator*() const { return _resource_list->_items[_id].resource; }
    /// Dereference the resource.
    /// @return The references resource entry.
    inline const Item *operator->() const { return &_resource_list->_items[_id].resource; }

    /// Get the resource entry @c Id . This can be stored in order to later access the resource via @c ResourceList
    /// indexing functions.
    /// @return The resource Id.
    inline Id id() const { return _id; }

  protected:
    Id _id = kNull;
    List *_resource_list = nullptr;
  };

  class ResourceRef : public ResourceRefBase<T, ResourceList<T>>
  {
  public:
    using Super = ResourceRefBase<T, ResourceList<T>>;

    /// Default constructor: the resulting object is not valid.
    inline ResourceRef() = default;
    /// Construct a resource for the given @p id and @p resource_list .
    /// @param id The resource Id.
    /// @param resource_list The resource list which we are referencing into.
    inline ResourceRef(Id id, ResourceList<T> *resource_list)
      : Super(id, resource_list)
    {}

    inline ResourceRef(const ResourceRef &) = delete;
    /// Move constructor.
    /// @param other Object to move.
    inline ResourceRef(ResourceRef &&other)
      : Super(std::move(other))
    {}

    inline ResourceRef &operator=(ResourceRef &&other)
    {
      if (this != &other)
      {
        std::swap(other._id, _id);
        std::swap(other._resource_list, _resource_list);
      }
      return *this;
    }

    /// Dereference the resource.
    /// @return The references resource entry.
    inline T &operator*() { return _resource_list->_items[_id].resource; }
    inline T *operator->() { return &_resource_list->_items[_id].resource; }
  };

  using ResourceConstRef = ResourceRefBase<const T, const ResourceList<T>>;

  template <typename R>
  class BaseIterator
  {
  public:
    using ResourceListT = R;
    BaseIterator() {}
    BaseIterator(ResourceListT *owner, ResourceListId id)
      : _owner(owner)
      , _id(id)
    {
      owner->lock();
    }

    ~BaseIterator()
    {
      if (_owner)
      {
        _owner->unlock();
      }
    }

    BaseIterator(const BaseIterator &other) = default;
    BaseIterator(BaseIterator &&other) = default;

    BaseIterator &operator=(const BaseIterator &other) = default;
    BaseIterator &operator=(BaseIterator &&other) = default;

    ResourceListT *owner() const { return _owner; }
    inline Id id() const { return _id; }

  protected:
    void next()
    {
      while (_owner && _id < _owner->_items.size() && _owner->_items[_id].next_free != kNull)
      {
        ++_id;
      }
    }

    void prev()
    {
      while (_owner && _id < _owner->_items.size() && _id != kNull && _owner->_items[_id].next_free != kNull)
      {
        _id = (_id > 0) ? --_id : kNull;
      }
    }

    ResourceListT *_owner = nullptr;
    Id _id = kNull;
  };

  class iterator : public BaseIterator<ResourceList<T>>
  {
  public:
    iterator() {}
    iterator(ResourceList<T> *owner, Id id)
      : BaseIterator<ResourceList<T>>(owner, id)
    {}
    iterator(const iterator &other) = default;
    iterator(iterator &&other) = default;

    /// Dereference the resource.
    /// @return The references resource entry.
    inline T &operator*() { return _owner->_items[_id].resource; }
    /// @overload
    inline const T &operator*() const { return _owner->_items[_id].resource; }
    /// Dereference the resource.
    /// @return The references resource entry.
    inline T *operator->() { return &_owner->_items[_id].resource; }
    /// @overload
    inline const T *operator->() const { return &_owner->_items[_id].resource; }

    iterator &operator=(const iterator &other) = default;
    iterator &operator=(iterator &&other) = default;

    template <typename R>
    bool operator==(const BaseIterator<R> &other) const
    {
      return _owner == other.owner() && _id == other.id();
    }

    template <typename R>
    bool operator!=(const BaseIterator<R> &other) const
    {
      return !operator==(other);
    }

    iterator &operator++()
    {
      next();
      return *this;
    }

    iterator operator++(int)
    {
      iterator current = *this;
      next();
      return current;
    }

    iterator &operator--()
    {
      prev();
      return *this;
    }

    iterator operator--(int)
    {
      iterator current = *this;
      prev();
      return current;
    }

    friend class const_iterator;
  };

  class const_iterator : public BaseIterator<const ResourceList<T>>
  {
  public:
    const_iterator() {}
    const_iterator(const ResourceList<T> *owner, Id id)
      : BaseIterator<const ResourceList<T>>(owner, id)
    {}
    const_iterator(const iterator &other)
      : BaseIterator<const ResourceList<T>>(other._owner, other._id)
    {}
    const_iterator(const const_iterator &other) = default;
    const_iterator(const_iterator &&other) = default;

    /// Dereference the resource.
    /// @return The references resource entry.
    inline const T &operator*() const { return _owner->_items[_id].resource; }
    /// Dereference the resource.
    /// @return The references resource entry.
    inline const T *operator->() const { return &_owner->_items[_id].resource; }

    const_iterator &operator=(const iterator &other)
    {
      _owner = other._owner;
      _id = other._id;
      return *this;
    }
    const_iterator &operator=(const const_iterator &other) = default;
    const_iterator &operator=(const_iterator &&other) = default;

    template <typename R>
    bool operator==(const BaseIterator<R> &other) const
    {
      return _owner == other.owner() && _id == other.id();
    }

    template <typename R>
    bool operator!=(const BaseIterator<R> &other) const
    {
      return !operator==(other);
    }

    const_iterator &operator++()
    {
      next();
      return *this;
    }

    const_iterator operator++(int)
    {
      const_iterator current = *this;
      next();
      return current;
    }

    const_iterator &operator--()
    {
      prev();
      return *this;
    }

    const_iterator operator--(int)
    {
      const_iterator current = *this;
      prev();
      return current;
    }
  };

  /// Construct a resource list optionally specifying the initial capacity.
  /// @param capacity The initial resource capacity.
  ResourceList(size_t capacity = 0);
  /// Destructor
  ~ResourceList() noexcept(false);

  inline iterator begin() { return iterator(this, firstValid()); }
  inline iterator end() { return iterator(this, kNull); }
  inline const_iterator begin() const { return const_iterator(this, firstValid()); }
  inline const_iterator end() const { return const_iterator(this, kNull); }

  /// Allocate a new resource.
  ///
  /// The @c Id from the @c ResourceRef return value should be stored for later release.
  ///
  /// @return A resource reference to the allocated item.
  ResourceRef allocate();

  /// Access the item at the given @p id .
  ///
  /// Raises a @c std::runtime_error if @p id does not reference a valid item.
  ///
  /// @param id The @c Id of the item to reference.
  /// @return The reference item.
  ResourceRef at(Id id);
  ResourceConstRef at(Id id) const;

  /// Release the item at the given @p id .
  /// @param id The @c Id of the item to release.
  void release(Id id);

  /// Access the item at the given @p id with undefined behaviour if @p id is invalid.
  /// @param id The @c Id of the item to reference.
  /// @return The reference item.
  ResourceRef operator[](Id id);
  /// @overload
  ResourceConstRef operator[](Id id) const;

  /// Return the number of allocated items.
  /// @return
  size_t size() const { return _item_count; }

  /// Release all resources. Raises a @c std::runtime_error if there are outstanding references.
  void clear();

private:
  friend ResourceRef;
  friend iterator;
  friend const_iterator;

  inline Id firstValid() const
  {
    for (Id id = 0; id < _items.size(); ++id)
    {
      const auto &item = _items[id];
      if (item.next_free == kAllocated)
      {
        return id;
      }
    }

    return kNull;
  }

  inline void lock() const
  {
    _lock.lock();
    ++_lock_count;
  }
  inline void unlock() const
  {
    --_lock_count;
    _lock.unlock();
  }

  struct Item
  {
    T resource;
    Id next_free;
  };

  std::vector<Item> _items;
  mutable std::recursive_mutex _lock;
  mutable std::atomic_uint32_t _lock_count = {};
  std::atomic_size_t _item_count = {};
  Id _free_head = kNull;
  Id _free_tail = kNull;
};


template <typename T>
ResourceList<T>::ResourceList(size_t capacity)
{
  if (capacity)
  {
    _items.reserve(capacity);
  }
}


template <typename T>
ResourceList<T>::~ResourceList() noexcept(false)
{
  std::unique_lock<decltype(_lock)> guard(_lock);
  if (_lock_count > 0)
  {
    throw std::runtime_error("Deleting resource list with outstanding resource references");
  }
}


template <typename T>
typename ResourceList<T>::ResourceRef ResourceList<T>::allocate()
{
  std::unique_lock<decltype(_lock)> guard(_lock);
  // Try free list first.
  if (_free_head != kNull)
  {
    ResourceRef resource(_free_head, this);
    if (_free_head != _free_tail)
    {
      _free_head = _items[_free_head].next_free;
    }
    else
    {
      _free_tail = _free_head = kNull;
    }
    _items[resource.id()].next_free = kAllocated;
    ++_item_count;
    return resource;
  }

  if (_items.size() == kAllocated)
  {
    throw std::runtime_error("Out of resources");
  }

  // Grow the container.
  _items.emplace_back(Item{ T{}, kAllocated });
  ++_item_count;
  return ResourceRef(_items.size() - 1u, this);
}


template <typename T>
typename ResourceList<T>::ResourceRef ResourceList<T>::at(Id id)
{
  if (id < _items.size() && _items[id].next_free == kAllocated)
  {
    return ResourceRef(id, this);
  }
  return ResourceRef(kNull, this);
}


template <typename T>
typename ResourceList<T>::ResourceConstRef ResourceList<T>::at(Id id) const
{
  if (id < _items.size() && _items[id].next_free == kAllocated)
  {
    return ResourceConstRef(id, this);
  }
  return ResourceConstRef(kNull, this);
}


template <typename T>
void ResourceList<T>::release(Id id)
{
  std::unique_lock<decltype(_lock)> guard(_lock);
  if (_free_head != kNull)
  {
    // Append to the free list tail.
    _items[_free_tail].next_free = id;
    _free_tail = id;
  }
  else
  {
    // First free item.
    _free_head = _free_tail = id;
  }
  --_item_count;
  _items[id].next_free = kNull;
}


template <typename T>
typename ResourceList<T>::ResourceRef ResourceList<T>::operator[](Id id)
{
  return ResourceRef(id, this);
}


template <typename T>
typename ResourceList<T>::ResourceConstRef ResourceList<T>::operator[](Id id) const
{
  return ResourceConstRef(id, this);
}


template <typename T>
void ResourceList<T>::clear()
{
  std::unique_lock<decltype(_lock)> guard(_lock);
  if (_lock_count > 0)
  {
    throw std::runtime_error("Deleting resource list with outstanding resource references");
  }
  _items.clear();
  _free_head = _free_tail = kNull;
}
}  // namespace tes::viewer::util

#endif  // TES_VIEWER_UTIL_RESOURCE_LIST_H
