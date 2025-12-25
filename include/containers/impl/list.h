#pragma once 
#define NOMINMAX
#include <iterator>
#include <utility.h>
#include <utility>
#include <memory>

namespace voxory {
  namespace containers {
    namespace detail {
      // ConstListIterator (виправлено postfix, category, operator[], +=/-=)
      template<typename Traits>
      class ConstListIterator {
      public:
        using value_type = typename Traits::value_type;
        using node_type = typename Traits::node_type;
        using node_pointer = node_type*;
        using const_pointer = typename Traits::const_pointer;
        using reference = typename Traits::const_reference;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        CONSTEXPR ConstListIterator() noexcept : _ptr(nullptr) {}
        CONSTEXPR explicit ConstListIterator(node_pointer ptr) noexcept : _ptr(ptr) {}

        CONSTEXPR reference operator*() const noexcept { 
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(_ptr != nullptr, "");
#endif // DEBUG_ITERATORS
          return _ptr->_data; 
        }
        CONSTEXPR const_pointer operator->() const noexcept { 
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(_ptr != nullptr, ""); 
#endif // DEBUG_ITERATORS

          return &_ptr->_data; 
        }

        CONSTEXPR ConstListIterator& operator++() noexcept {
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(_ptr != nullptr, "");
#endif // DEBUG_ITERATORS
          _ptr = _ptr->_next;
          return *this;
        }
        CONSTEXPR ConstListIterator operator++(int) noexcept {
          ConstListIterator tmp = *this;
          ++(*this);
          return tmp;
        }

        CONSTEXPR ConstListIterator& operator--() noexcept {
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(_ptr != nullptr, "");
#endif // DEBUG_ITERATORS
          _ptr = _ptr->_last;
          return *this;
        }
        CONSTEXPR ConstListIterator operator--(int) noexcept {
          ConstListIterator tmp = *this;
          --(*this);
          return tmp;
        }

        CONSTEXPR ConstListIterator& operator+=(difference_type off) noexcept {
          if (off >= 0) {
            while (off--) {
#ifdef DEBUG_ITERATORS
              ASSERT_ABORT(_ptr != nullptr, "");
#endif // DEBUG_ITERATORS
              _ptr = _ptr->_next;
            }
          }
          else {
            while (off++) {
#ifdef DEBUG_ITERATORS
              ASSERT_ABORT(_ptr != nullptr, "");
#endif // DEBUG_ITERATORS
              _ptr = _ptr->_last;
            }
          }
          return *this;
        }
        CONSTEXPR ConstListIterator operator+(difference_type off) const noexcept {
          ConstListIterator tmp = *this; tmp += off; return tmp;
        }

        CONSTEXPR ConstListIterator& operator-=(difference_type off) noexcept {
          return *this += -off;
        }
        CONSTEXPR ConstListIterator operator-(difference_type off) const noexcept {
          ConstListIterator tmp = *this; tmp -= off; return tmp;
        }

        CONSTEXPR bool operator==(const ConstListIterator& other) const noexcept { return _ptr == other._ptr; }
        CONSTEXPR bool operator!=(const ConstListIterator& other) const noexcept { return _ptr != other._ptr; }

        CONSTEXPR reference operator[](difference_type off) const noexcept {
          node_pointer tmp = _ptr;
          if (off >= 0) {
            while (off--) { 
#ifdef DEBUG_ITERATORS
              ASSERT_ABORT(tmp != nullptr, ""); 
#endif // DEBUG_ITERATORS
              tmp = tmp->_next; 
            }
          }
          else {
            while (off++) { 
#ifdef DEBUG_ITERATORS
              ASSERT_ABORT(tmp != nullptr, ""); 
#endif // DEBUG_ITERATORS
              tmp = tmp->_last; 
            }
          }
          return tmp->_data;
        }

        CONSTEXPR const node_pointer unwrap() const {
          return _ptr;
        }
      protected:
        node_pointer _ptr;
      };

      // ListIterator (mutable)
      template<typename Traits>
      class ListIterator : public ConstListIterator<Traits> {
        using base_type = ConstListIterator<Traits>;
      public:
        using value_type = typename Traits::value_type;
        using pointer = typename Traits::pointer;
        using reference = typename Traits::reference;
        using node_type = typename base_type::node_type;
        using node_pointer = typename base_type::node_pointer;
        using difference_type = typename base_type::difference_type;
        using iterator_category = std::bidirectional_iterator_tag;

        CONSTEXPR ListIterator() noexcept : base_type(nullptr) {}
        CONSTEXPR explicit ListIterator(node_pointer ptr) noexcept : base_type(ptr) {}

        CONSTEXPR reference operator*() const noexcept {
          return const_cast<reference>(base_type::operator*());
        }
        CONSTEXPR pointer operator->() const noexcept {
          return &this->_ptr->_data;
        }

        CONSTEXPR ListIterator& operator++() noexcept { base_type::operator++(); return *this; }
        CONSTEXPR ListIterator operator++(int) noexcept { ListIterator tmp = *this; base_type::operator++(); return tmp; }
        CONSTEXPR ListIterator& operator--() noexcept { base_type::operator--(); return *this; }
        CONSTEXPR ListIterator operator--(int) noexcept { ListIterator tmp = *this; base_type::operator--(); return tmp; }

        CONSTEXPR ListIterator& operator+=(difference_type off) noexcept { base_type::operator+=(off); return *this; }
        CONSTEXPR ListIterator& operator-=(difference_type off) noexcept { base_type::operator-=(off); return *this; }
        CONSTEXPR ListIterator operator+(difference_type off) const noexcept { ListIterator tmp = *this; tmp += off; return tmp; }
        CONSTEXPR ListIterator operator-(difference_type off) const noexcept { ListIterator tmp = *this; tmp -= off; return tmp; }

        CONSTEXPR bool operator==(const ListIterator& other) const noexcept { return base_type::operator==(other); }
        CONSTEXPR bool operator!=(const ListIterator& other) const noexcept { return base_type::operator!=(other); }

        CONSTEXPR reference operator[](difference_type off) const noexcept {
          return const_cast<reference>(base_type::operator[](off));
        }

        CONSTEXPR node_pointer unwrap() const {
          return const_cast<node_pointer>(this->unwrap());
        }
      };
      template<typename T>
      struct default_node {
        using value_type = T;
        default_node() : _data(nullptr), _next(nullptr), _last(nullptr) {}
        default_node(value_type data, default_node* next = nullptr, default_node* last = nullptr)
          : _data(data), _next(next), _last(last) {
        }
        ~default_node() = default;
        value_type _data;
        default_node* _next;
        default_node* _last;
      };
    }

    template<typename T, typename Node = detail::default_node<T>>
    class list {
      using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
      using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
    public:
      using node_type = Node;
      using allocator_type = std::allocator<node_type>;
      using allocator_traits = std::allocator_traits<allocator_type>;
      using value_type = T;
      using pointer = node_type*;
      using const_pointer = const pointer;
      using reference = value_type&;
      using const_reference = const reference;
      using iterator = detail::ListIterator<list>;
      using const_iterator = detail::ConstListIterator<list>;
      using size_type = std::size_t;

      list(allocator_type alloc = allocator_type()) noexcept(std::is_nothrow_default_constructible_v<allocator_type>) : _pair(FirstOneSecondArgs{}, alloc), _size(0) {};

      explicit list(size_t n, allocator_type alloc = allocator_type())
        : _pair(FirstOneSecondArgs{}, alloc, std::pair<pointer, pointer>{nullptr, nullptr}), _size(0)
      {
        for (size_t i = 0; i < n; ++i) emplace_back(nullptr);
      }

      list(const list& o) : _pair(FirstOneSecondArgs{}, allocator_traits::select_on_container_copy_construction(o._pair.first())), _size(o._size) {
        for (auto& e : o) {
          emplace_back(e);
        }
      }

      list(list&& o) : _pair(FirstOneSecondArgs{}, std::move(o._pair.first())), _size(o._size) {
        for (auto& e : o) {
          emplace_back(std::move(e));
        }
      }

      ~list() {
        clear();
      }

      list& operator=(const list& o) {
        if (this == &o) return *this;
        auto& al = get_allocator();
        auto& o_al = o.get_allocator();

        auto& data = _pair._second;
        auto& o_data = o._pair._second;

        if constexpr (allocator_traits::propagate_on_container_copy_assignment::value) {
          //change allocator, allocate with copy ctor from new one
          if (al != o_al) {
            clear();
            al = o_al;
          }
        }

        assign_copy(o);
        return *this;
      };

      list& operator=(list&& o) {
        if (this == std::addressof(o)) return *this;
        auto& al = get_allocator();
        auto& o_al = o.get_allocator();

        if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
          clear();
          al = std::move(o_al);
          return steal_no_cleanup(o);
        }
        else {
          if constexpr (allocator_traits::is_always_equal::value)
          {
            clear();
            return steal_no_cleanup(o);
          }
          else {
            if (al == o_al)
            {
              clear();
              return steal_no_cleanup(o);
            }
            else
            {
              assign_move(std::move(o));
              o.clear();
              return *this;
            }
          }
        }
      }


      NODISCARD CONSTEXPR iterator begin() noexcept {
        return iterator(_pair._second.first);
      };

      NODISCARD CONSTEXPR const_iterator begin() const noexcept {
        return const_iterator(_pair._second.first);
      };

      NODISCARD CONSTEXPR const_iterator cbegin() const noexcept {
        return const_iterator(_pair._second.first);
      };

      NODISCARD CONSTEXPR iterator end() noexcept {
        return iterator(nullptr);
      };

      NODISCARD CONSTEXPR const_iterator end() const noexcept {
        return const_iterator(nullptr);
      };

      NODISCARD CONSTEXPR const_iterator cend() const noexcept {
        return const_iterator(nullptr);
      };


      CONSTEXPR bool empty() const noexcept { return _size == 0; }
      size_type size() const noexcept { return _size; }

      // push_back (встановлюємо _last і _next)
      CONSTEXPR node_type* push_back(const value_type& val) {
        return emplace_back(val);
      }

      CONSTEXPR node_type* push_back(value_type&& val) {
        return emplace_back(std::move(val));
      }

      CONSTEXPR void pop_back() {
        if (!_pair._second.first) return;
        if (_pair._second.first == _pair._second.second) {
          _destroy_node(_pair._second.first);
          _pair._second.first = _pair._second.second = nullptr;
          _size = 0;
          return;
        }
        pointer tail = _pair._second.second;
        pointer prev = tail->_last;
        _destroy_node(tail);
        prev->_next = nullptr;
        _pair._second.second = prev;
        --_size;
      }

      CONSTEXPR void pop_at(pointer loc) {
        if (!loc) return;

        if (loc->_last) loc->_last->_next = loc->_next;
        else {
          _pair._second.first = loc->_next;
          if (loc->_next) {
            loc->_next->_last = nullptr;
          }
        }

        if (loc->_next) loc->_next->_last = loc->_last;
        else {
          _pair._second.second = loc->_last;
          if (loc->_last) {
            loc->_last->_next = nullptr;
          }
        }

        _destroy_node(loc);
        --_size;
      }

      CONSTEXPR void clear() noexcept {
        node_type* cur = _pair._second.first;
        while (cur) {
          node_type* next = cur->_next;
          _destroy_node(cur);
          cur = next;
        }
        _pair._second.first = _pair._second.second = nullptr;
        _size = 0;
      }

      CONSTEXPR node_type* data() {
        return _pair._second.first;
      }

      CONSTEXPR const allocator_type& get_allocator() const {
        return _pair.first();
      }
      CONSTEXPR allocator_type& get_allocator() {
        return _pair.first();
      }

      CONSTEXPR void _destroy_node(node_type* p) noexcept {
        if constexpr (!std::is_trivially_destructible_v<value_type>)
        {
          if constexpr (!type_traits::has_destroy_v<allocator_type, pointer>) {
            std::destroy_at(p);
          }
          else
          {
            _pair.first().destroy(p);
          }
        }
        _pair.first().deallocate(p, 1);
      }
    private:
      inline list& steal_no_cleanup(list& o) noexcept {
        _pair._second.first = std::exchange(o._pair._second.first, nullptr);
        _pair._second.second = std::exchange(o._pair._second.second, nullptr);
        _size = std::exchange(o._size, 0);
        return *this;
      }

      template<typename U>
      CONSTEXPR node_type* emplace_back(U&& val) {
        pointer p = _pair.first().allocate(1);
        // placement new з повною ініціалізацією _last:
        new (p) node_type(std::forward<U>(val), nullptr, _pair._second.second);
        if (!_pair._second.first) {
          _pair._second.first = _pair._second.second = p;
        }
        else {
          _pair._second.second->_next = p;
          p->_last = _pair._second.second;
          _pair._second.second = p;
        }
        ++_size;
        return p;
      }

      void assign_move(list&& o) {
        size_t my_sz = _size;
        size_t other_sz = o._size;
        size_t common = (std::min)(my_sz, other_sz);

        auto it = begin();
        auto o_it = o.begin();

        for (size_t i = 0; i < common; ++i, ++it, ++o_it) {
          *it = std::move(*o_it);
        }

        if (my_sz < other_sz) {
          for (; o_it != o.end(); ++o_it) {
            emplace_back(std::move(*o_it));
          }
        }
        else if (my_sz > other_sz) {
          auto erase_it = it;
          while (erase_it != end()) {
            auto next = erase_it + 1;
            _destroy_node(erase_it.unwrap());
            erase_it = next;
          }
          _size = other_sz;
        }
      }

      void assign_copy(const list& o) {
        size_t my_sz = _size;
        size_t other_sz = o._size;
        size_t common = (std::min)(my_sz, other_sz);

        auto it = begin();
        auto o_it = o.begin();

        for (size_t i = 0; i < common; ++i, ++it, ++o_it) {
          *it = *o_it;
        }

        if (my_sz < other_sz) {
          for (; o_it != o.end(); ++o_it) {
            emplace_back(*o_it);
          }
        }
        else if (my_sz > other_sz) {
          auto erase_it = it;
          while (erase_it != end()) {
            auto next = erase_it + 1;
            _destroy_node(erase_it.unwrap());
            erase_it = next;
          }
          _size = other_sz;
        }
      }

    private:
      utility::compressed_pair<allocator_type, std::pair<pointer, pointer>> _pair;
      size_t _size;
    };
  }
}
