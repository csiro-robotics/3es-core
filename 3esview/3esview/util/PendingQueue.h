#ifndef TES_VIEW_UTIL_PENDING_QUEUE_H
#define TES_VIEW_UTIL_PENDING_QUEUE_H

#include <3esview/ViewConfig.h>

#include <algorithm>
#include <iterator>
#include <vector>

namespace tes::view::util
{
/// A FIFO fashion, with sections marked by a monotonic increasing value.
///
/// The primary use case is to enqueue modifications for up coming render frames, when report only
/// items up to a requested frame number. Further comments are made with this context in mind.
///
/// Typical usage sees a producer adding items to the queue using @c emplace_back() before a call to
/// @c mark() is made. This @c mark() call sets the frame number after which the newly queued items
/// become relevant. Further items can be queued, with subsequent @c mark() calls requiring the
/// frame number be greater than the previous mark.
///
/// A consumer of the queue will then request a @c view() into the queue, specifying the relevant
/// frame number. The @c View incorporates all items in the queue marked with a frame number less
/// than or equal to the requested frame number. Once the @c View is released, the items from that
/// view are removed from the queue, unless the @c View is configured to preserve these items.
///
/// Note the container is not threadsafe and requires external thread synchronisation to protect it.
/// @tparam T
template <typename T>
class PendingQueue
{
public:
  // NOLINTBEGIN(readability-identifier-naming)
  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::iterator;
  using value_type = typename std::vector<T>::value_type;
  using size_type = typename std::vector<T>::size_type;
  using difference_type = typename std::vector<T>::difference_type;
  using reference = typename std::vector<T>::reference;
  using const_reference = typename std::vector<T>::const_reference;
  using pointer = typename std::vector<T>::pointer;
  using reverse_iterator = typename std::vector<T>::reverse_iterator;
  using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;
  // NOLINTEND(readability-identifier-naming)
  using MarkType = size_t;
  using Queue = std::vector<std::pair<MarkType, T>>;

  /// A view into a @c PendingQueue which includes all items up to a specific mark value.
  ///
  /// This template version supports const and non-const types to build const and non-const views.
  ///
  /// Iterate the range using @c begin() and @c end() normally, or a range based for loop.
  ///
  /// Oh for C++ 20 ranges...
  ///
  /// @tparam Q The queue type, either @c PendingQueue<T> or `const PendingQueue<T>`
  /// @tparam U The type contained by the queue, either @c T or `const T`.
  /// @tparam I The @c Queue iterator type, either @c PendingQueue<T>::Queue::iterator or
  /// @c PendingQueue<T>::Queue::const_iterator
  template <typename Q, typename U, typename I>
  class ViewT
  {
  public:
    /// Iterator type into the @c ViewT
    class iterator
    {
    public:
      /// Default constructor.
      iterator() = default;
      /// Construct from a @c Queue::iterator or @c Queue::const_iterator
      /// @param iter The internal queue iterator.
      iterator(I iter)
        : _iter(std::move(iter))
      {}
      /// Copy constructor.
      /// @param other Iterator to copy.
      iterator(const iterator &other) = default;
      /// Move constructor.
      /// @param other Iterator to move.
      iterator(iterator &&other) noexcept = default;

      /// Copy assignemnt.
      /// @param other Iterator to copy.
      iterator &operator=(const iterator &other) = default;
      /// Move assignemnt.
      /// @param other Iterator to move.
      iterator &operator=(iterator &&other) noexcept = default;

      /// Equality operator.
      /// @param other Object to compare against.
      /// @return True when equal.
      [[nodiscard]] bool operator==(const iterator &other) const { return _iter == other._iter; }
      /// Equality operator.
      /// @param other Object to compare against.
      /// @return True when not equal.
      [[nodiscard]] bool operator!=(const iterator &other) const { return _iter != other._iter; }

      /// Prefix increment operator.
      /// @return @c *this after increment.
      iterator &operator++()
      {
        ++_iter;
        return *this;
      }
      /// Postfix increment operator.
      /// @return The state of @c *this before increment.
      iterator operator++(int)
      {
        auto old = *this;
        ++_iter;
        return old;
      }

      /// Prefix decrement operator.
      /// @return @c *this after decrement.
      iterator &operator--()
      {
        --_iter;
        return *this;
      }
      /// Postfix decrement operator.
      /// @return The state of @c *this before decrement.
      iterator operator--(int)
      {
        auto old = *this;
        --_iter;
        return old;
      }

      /// Dereference operator.
      /// @return The referenced type.
      [[nodiscard]] U &operator*() const { return _iter->second; };
      /// Dereference operator.
      /// @return The referenced type.
      [[nodiscard]] U *operator->() const { return &_iter->second; };

    private:
      I _iter;
    };

    /// Construct a view for @p queue at the given @p view_mark .
    /// @param queue The queue to view.
    /// @param view_mark The mark to view up to and including.
    /// @param preserve True to preserve the contents of the view from @p queue when done otherwise
    /// the contents are discarded from @p queue on destruction of the view.
    ViewT(Q &queue, MarkType view_mark, bool preserve = false)
      : _queue(queue)
      , _preserve(preserve)
    {
      _end = _queue.find(view_mark);
    }

    /// Construct a view of all items in @p queue .
    /// @param queue The queue to view.
    /// @param preserve True to preserve the contents of the view from @p queue when done otherwise
    /// the contents are discarded from @p queue on destruction of the view.
    ViewT(Q &queue, bool preserve = false)
      : _queue(queue)
      , _preserve(preserve)
      , _view_all(true)
    {
      _end = _queue._items.end();
    }

    /// Destructor, discarding the contents of the view from the queue unless marked to preserve.
    ///
    /// A constant view always preserves as the @c discard() overload it calls does nothing.
    ~ViewT()
    {
      if (!_preserve)
      {
        _queue.discard(_end);
      }
    }

    /// Get the begin iterator.
    /// @return Iterator to the first item.
    [[nodiscard]] iterator begin() { return iterator(_queue._items.begin()); }
    /// Get the end iterator.
    /// @return Iterator to beyond the last item.
    [[nodiscard]] iterator end() { return iterator(_end); }

    /// Preserve the contents of the view when done?
    ///
    /// Defaults to discard.
    /// @return True to preserve.
    [[nodiscard]] bool preserve() const { return _preserve; }
    /// Set the @c preserve() flag.
    /// @param preserve True to preserve, false to discard.
    void setPreserve(bool preserve) const { _preserve = preserve; }

    /// Check if the view is empty.
    /// @return True when the view empty.
    [[nodiscard]] bool empty() const { return begin() == _end; }

    /// Calculate the number of items in the @c View.
    /// @return Number of items in the view.
    [[nodiscard]] size_t size() const { return _end - begin(); }

  protected:
    Q &_queue;
    I _end;
    bool _preserve = false;
    bool _view_all = false;
  };

  /// A mutable view into the queue.
  class View : public ViewT<PendingQueue<T>, T, typename PendingQueue<T>::Queue::iterator>
  {
  public:
    /// Base class type.
    using Super = ViewT<PendingQueue<T>, T, typename PendingQueue<T>::Queue::iterator>;

    /// Construct a view into @p queue up to @p view_mark (inclusive).
    /// @param queue The queue to view.
    /// @param view_mark The mark to view up to.
    View(PendingQueue<T> &queue, MarkType view_mark)
      : Super(queue, view_mark, false)
    {}

    /// Construct a view of all items in @p queue .
    /// @param queue The queue to view.
    View(PendingQueue<T> &queue)
      : Super(queue, false)
    {}
  };

  /// A const view into the queue.
  class ViewConst
    : public ViewT<const PendingQueue<T>, const T, typename PendingQueue<T>::Queue::const_iterator>
  {
  public:
    /// Alias the iterator as @c const_iterator, which is more correct.
    using const_iterator = iterator;
    /// Base class type.
    using Super =
      ViewT<const PendingQueue<T>, const T, typename PendingQueue<T>::Queue::const_iterator>;

    /// Construct a view into @p queue up to @p view_mark (inclusive).
    /// @param queue The queue to view.
    /// @param view_mark The mark to view up to.
    ViewConst(const PendingQueue<T> &queue, MarkType view_mark)
      : Super(queue, view_mark, true)
    {}

    /// Construct a view of all items in @p queue .
    /// @param queue The queue to view.
    ViewConst(const PendingQueue<T> &queue)
      : Super(queue, true)
    {}
  };

  friend ViewT<PendingQueue<T>, T, typename PendingQueue<T>::Queue::iterator>;
  friend ViewT<const PendingQueue<T>, const T, typename PendingQueue<T>::Queue::const_iterator>;

  /// The default mark assigned to new, unmarked items.
  static constexpr MarkType kDefaultMark = 0;

  /// Default constructor.
  PendingQueue() = default;
  /// Copy constructor.
  /// @param other Object to copy.
  PendingQueue(const PendingQueue<T> &other) = default;
  /// Move constructor.
  /// @param other Object to move.
  PendingQueue(PendingQueue<T> &&other) noexcept = default;

  /// Copy assignment.
  /// @param other Object to copy.
  PendingQueue &operator=(const PendingQueue<T> &other) = default;
  /// Move assignment.
  /// @param other Object to move.
  PendingQueue &operator=(PendingQueue<T> &&other) noexcept = default;

  /// Request a view of all items in the queue up to the given @p mark (inclusive).
  ///
  /// **Items from the view are discarded when the @c View goes out of scope.**
  ///
  /// @param mark The mark value to view up to and including.
  /// @return The @c View .
  [[nodiscard]] View view(MarkType mark) { return View(*this, mark); }

  /// Request a view of all items in the queue.
  ///
  /// **All items in the queue are discarded when the @c View goes out of scope.**
  ///
  /// @return The @c View .
  [[nodiscard]] View view() { return View(*this); }

  /// Request a const view of all items in the queue up to the given @p mark (inclusive).
  ///
  /// Items from the view are **preserved** after the @c ViewConst goes out of scope.
  ///
  /// @param mark The mark value to view up to and including.
  /// @return The @c ViewConst .
  [[nodiscard]] ViewConst view(MarkType mark) const { return ViewConst(*this, mark); }
  /// Alias for @c view() retrieving a @c ViewConst.
  /// @param mark The mark value to view up to and including.
  /// @return The @c ViewConst .
  [[nodiscard]] ViewConst viewConst(MarkType mark) const { return ViewConst(*this, mark); }

  /// Request a const view of all items in the queue.
  ///
  /// Items from the view are **preserved** after the @c ViewConst goes out of scope.
  ///
  /// @return The @c ViewConst .
  [[nodiscard]] ViewConst view() const { return ViewConst(*this); }

  /// Alias for @c view() retrieving a @c ViewConst.
  /// @return The @c ViewConst .
  [[nodiscard]] ViewConst viewConst() const { return ViewConst(*this); }

  /// Add an item to the queue.
  ///
  /// The new item is unmarked.
  /// @tparam ...Args Argument types
  /// @param ...args Arguments
  template <typename... Args>
  void emplace_back(Args &&...args)
  {
    _items.emplace_back(std::pair<size_t, T>(kDefaultMark, T{ args... }));
  }

  /// Check if the queue is empty.
  /// @return True when the queue empty.
  [[nodiscard]] bool empty() const noexcept { return _items.empty(); }
  /// Calculate the number of items in the queue.
  /// @return Number of items in the queue.
  [[nodiscard]] size_type size() const noexcept { _items.size(); }

  /// Set the capacity for the internal queue.
  /// @param new_cap The new capacity.
  void reserve(size_type new_cap) { _items.reserve(new_cap); }
  /// Query the capacity of the queue.
  /// @return The current capacity.
  [[nodiscard]] size_type capacity() const noexcept { return _items.capacity(); }

  /// Release excess memory from the queue.
  void shrink_to_fit() { _items.shrink_to_fit(); }

  /// Remove all items from the queue.
  ///
  /// Memory is retained.
  void clear()
  {
    _items.clear();
    _last_mark = kDefaultMark;
    _next_mark_index = 0;
  }

  /// Mark most recent items in the queue (unmarked items) with the given @p value.
  /// @param value The mark value.
  void mark(MarkType value);

  /// Query the last mark value given to @c mark() .
  /// @return The last mark value.
  MarkType lastMark() const { return _last_mark; }

  /// Discard all items which have a mark value less than or equal to the given @p mark_value.
  /// @param mark_value Discard mark value: remove items with marks less than or equal to this
  /// value.
  void discard(MarkType mark_value);

private:
  /// Discard items up to the given @p iter (exclusive).
  /// @param iter The end iterator to discard up to.
  void discard(const typename Queue::iterator &iter);
  /// To support @c ViewConst - does nothing.
  void discard(const typename Queue::const_iterator &iter) const { (void)iter; }
  /// Find the "end" iterator for items up to and including the @p mark_value .
  /// @param mark_value The mark value of interest.
  /// @return The iterator ot the first item marked with a value greater than @p mark_value, or
  /// the end iterator of the iternal queue.
  typename Queue::iterator find(MarkType mark_value);
  /// @overload
  typename Queue::const_iterator find(MarkType mark_value) const;

  Queue _items;
  MarkType _last_mark = kDefaultMark;
  /// Index at which to start the next marking sequence.
  size_t _next_mark_index = 0;
};


template <typename T>
inline void PendingQueue<T>::mark(MarkType value)
{
  // Iterate the container marking items which have a mark value greater than the given value.
  // We could start later in the container if we can resolve where the last mark was.
  for (size_t i = _next_mark_index; i < _items.size(); ++i)
  {
    _items[i].first = value;
  }
  // Record where the last mark ended for the next mark call.
  _next_mark_index = _items.size();
  _last_mark = value;
}


template <typename T>
inline void PendingQueue<T>::discard(MarkType mark_value)
{
  // Find the last item to remove.
  const auto iter = find(mark_value);
  discard(iter);
}


template <typename T>
inline void PendingQueue<T>::discard(const typename Queue::iterator &iter)
{
  if (iter == _items.begin())
  {
    // Nothing to remove.
    return;
  }

  if (iter == _items.end())
  {
    // remove everything.
    _items.clear();
    _next_mark_index = 0;
    return;
  }

  // Remove up to iter
  const size_t iter_index = iter - _items.begin();
  const size_t remaining = _items.size() - iter_index;
  std::copy(iter, _items.end(), _items.begin());
  // Clear the excess.
  _items.resize(remaining);
  if (_next_mark_index >= iter_index)
  {
    _next_mark_index -= iter_index;
  }
  else
  {
    _next_mark_index = 0;
  }
}


template <typename T>
typename PendingQueue<T>::Queue::iterator PendingQueue<T>::find(MarkType mark_value)
{
  // Find the last item to remove.
  auto iter = _items.begin();
  for (; iter != _items.end(); ++iter)
  {
    if (iter->first > mark_value)
    {
      // First item to not remove.
      break;
    }
  }

  return iter;
}


template <typename T>
typename PendingQueue<T>::Queue::const_iterator PendingQueue<T>::find(MarkType mark_value) const
{
  // Find the last item to remove.
  auto iter = _items.begin();
  for (; iter != _items.end(); ++iter)
  {
    if (iter->first > mark_value)
    {
      // First item to not remove.
      break;
    }
  }

  return iter;
}
}  // namespace tes::view::util

#endif  // TES_VIEW_UTIL_PENDING_QUEUE_H
