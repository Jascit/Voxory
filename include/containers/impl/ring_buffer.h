#pragma once
#include <vector>
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <cstring> // std::memcpy
#include <iostream>
#include <algorithm>

namespace containers {

  template<typename T, typename Alloc = std::allocator<T>>
  class ring_buffer {
  public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    // True when we can memcpy safely
    constexpr static bool VTrivial =
      std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

    // --- ctors/dtor ---
    explicit ring_buffer(const allocator_type& alloc = allocator_type())
      : storage_(alloc), capacity_(0), head_(0)
    {
    }

    explicit ring_buffer(size_type n, const allocator_type& alloc = allocator_type())
      : storage_(n, alloc), capacity_(n), head_(0)
    {
    }

    ring_buffer(const ring_buffer& o) : storage_(o.storage_), capacity_(o.capacity_), head_(o.head_) {};
    ring_buffer(ring_buffer&& o) noexcept : storage_(std::move(o.storage_)), capacity_(o.capacity_), head_(o.head_) {};
    ring_buffer& operator=(const ring_buffer& o) {
      if (this == std::addressof(o)) return *this;
      storage_ = o.storage_;
      capacity_ = o.capacity_;
      head_ = o.head_;
      return *this;
    };

    ring_buffer& operator=(ring_buffer&& o) noexcept {
      if (this == std::addressof(o)) return *this;
      storage_ = std::move(o.storage_);
      capacity_ = o.capacity_;
      head_ = o.head_;
      return *this;
    };

    ~ring_buffer() = default;

    // --- capacity / allocator ---
    size_type size() const noexcept { return capacity_; } // matches старий semantic: size_ == capacity
    allocator_type get_allocator() const noexcept { return storage_.get_allocator(); }

    // Reserve/allocate exactly n elements (default-constructed).
    // This uses vector::resize so operator[] is safe later.
    void reserve(size_type n) {
      if (n == 0) {
        storage_.clear();
        storage_.shrink_to_fit();
        capacity_ = 0;
        head_ = 0;
        return;
      }
      storage_.resize(n);   // default-construct n elements
      capacity_ = n;
      head_ = 0;
    }

    // push: записує значення в позицію head_ і переміщує head_ вперед (wrap)
    void push(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
      if (capacity_ == 0) return;
      storage_[head_] = value;
      head_ = (head_ + 1) % capacity_;
    }
    void push(T&& value) noexcept(std::is_nothrow_move_assignable_v<T>) {
      if (capacity_ == 0) return;
      storage_[head_] = std::move(value);
      head_ = (head_ + 1) % capacity_;
    }

    // get_interval: копіює 'count' елементів, починаючи з логічного 'start',
    // у вихідний ring_buffer 'out' (out.reserve буде викликаний за потреби).
    void get_interval(size_type start, size_type count, ring_buffer& out) const {
      if (count > capacity_) {
        std::cerr << "Requested count exceeds ring_buffer capacity\n";
        return;
      }
      if (out.size() < count) out.reserve(count);

      if (capacity_ == 0 || count == 0) return;

      size_type norm_start = normalize_index(static_cast<int64_t>(start));
      size_type end_idx = normalize_index(static_cast<int64_t>(start + count));

      const_pointer src = storage_.data();
      pointer dst = out.storage_.data();

      if constexpr (VTrivial) {
        if (norm_start < end_idx) {
          std::memcpy(dst, src + norm_start, sizeof(value_type) * count);
        }
        else {
          size_type first_part = capacity_ - norm_start;
          std::memcpy(dst, src + norm_start, sizeof(value_type) * first_part);
          std::memcpy(dst + first_part, src, sizeof(value_type) * (count - first_part));
        }
      }
      else {
        for (size_type i = 0; i < count; ++i) {
          size_type idx = normalize_index(static_cast<int64_t>(start + i));
          dst[i] = src[idx];
        }
      }
    }

    // clear: якщо тип нетривіальний — присвоюємо дефолтні значення
    void clear() noexcept(std::is_nothrow_destructible_v<T>) {
      if (capacity_ == 0) return;
      if constexpr (!VTrivial) {
        for (size_type i = 0; i < capacity_; ++i) storage_[i] = T{};
      }
      head_ = 0;
    }

    // ітератори / доступи
    pointer data() noexcept { return storage_.data(); }
    const_pointer data() const noexcept { return storage_.data(); }

    // логічний "головний" ітератор (вказує на елемент за head_ позицією)
    pointer head() noexcept { return storage_.data() + head_; }
    const_pointer head() const noexcept { return storage_.data() + head_; }

    // end() тут означає pointer на one-past-end масиву (для роботи старих алгоритмів)
    pointer end() noexcept { return storage_.data() + capacity_; }
    const_pointer end() const noexcept { return storage_.data() + capacity_; }

    // оператор індексації з нормалізацією (логічний індекс)
    reference operator[](size_type index) noexcept {
      size_type idx = normalize_index(static_cast<int64_t>(index));
      return storage_[idx];
    }
    const_reference operator[](size_type index) const noexcept {
      size_type idx = normalize_index(static_cast<int64_t>(index));
      return storage_[idx];
    }

    // отримати поточну головну позицію (індекс)
    size_type get_head() const noexcept { return head_; }

  private:
    static size_type normalize_index(int64_t index, size_type m) noexcept {
      if (m == 0) return 0;
      int64_t idx = (index % static_cast<int64_t>(m) + static_cast<int64_t>(m)) % static_cast<int64_t>(m);
      return static_cast<size_type>(idx);
    }
    size_type normalize_index(int64_t index) const noexcept {
      return normalize_index(index, capacity_);
    }

  private:
    std::vector<T, allocator_type> storage_;
    size_type capacity_;
    size_type head_; // next write position (0..capacity_-1)
  };

} // namespace containers
