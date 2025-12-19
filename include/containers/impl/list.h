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

        CONSTEXPR ConstListIterator() noexcept : ptr_(nullptr) {}
        CONSTEXPR explicit ConstListIterator(node_pointer ptr) noexcept : ptr_(ptr) {}

        CONSTEXPR reference operator*() const noexcept { 
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(ptr_ != nullptr, "");
#endif // DEBUG_ITERATORS
          return ptr_->data_; 
        }
        CONSTEXPR const_pointer operator->() const noexcept { 
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(ptr_ != nullptr, ""); 
#endif // DEBUG_ITERATORS

          return &ptr_->data_; 
        }

        CONSTEXPR ConstListIterator& operator++() noexcept {
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(ptr_ != nullptr, "");
#endif // DEBUG_ITERATORS
          ptr_ = ptr_->next_;
          return *this;
        }
        CONSTEXPR ConstListIterator operator++(int) noexcept {
          ConstListIterator tmp = *this;
          ++(*this);
          return tmp;
        }

        CONSTEXPR ConstListIterator& operator--() noexcept {
#ifdef DEBUG_ITERATORS
          ASSERT_ABORT(ptr_ != nullptr, "");
#endif // DEBUG_ITERATORS
          ptr_ = ptr_->last_;
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
              ASSERT_ABORT(ptr_ != nullptr, "");
#endif // DEBUG_ITERATORS
              ptr_ = ptr_->next_;
            }
          }
          else {
            while (off++) {
#ifdef DEBUG_ITERATORS
              ASSERT_ABORT(ptr_ != nullptr, "");
#endif // DEBUG_ITERATORS
              ptr_ = ptr_->last_;
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

        CONSTEXPR bool operator==(const ConstListIterator& other) const noexcept { return ptr_ == other.ptr_; }
        CONSTEXPR bool operator!=(const ConstListIterator& other) const noexcept { return ptr_ != other.ptr_; }

        CONSTEXPR reference operator[](difference_type off) const noexcept {
          node_pointer tmp = ptr_;
          if (off >= 0) {
            while (off--) { 
#ifdef DEBUG_ITERATORS
              ASSERT_ABORT(tmp != nullptr, ""); 
#endif // DEBUG_ITERATORS
              tmp = tmp->next_; 
            }
          }
          else {
            while (off++) { 
#ifdef DEBUG_ITERATORS
              ASSERT_ABORT(tmp != nullptr, ""); 
#endif // DEBUG_ITERATORS
              tmp = tmp->last_; 
            }
          }
          return tmp->data_;
        }

        CONSTEXPR const node_pointer unwrap() const {
          return ptr_;
        }
      protected:
        node_pointer ptr_;
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
          return &this->ptr_->data_;
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
        default_node() : data_(nullptr), next_(nullptr), last_(nullptr) {}
        default_node(value_type data, default_node* next = nullptr, default_node* last = nullptr)
          : data_(data), next_(next), last_(last) {
        }
        ~default_node() = default;
        value_type data_;
        default_node* next_;
        default_node* last_;
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

      list(allocator_type alloc = allocator_type()) noexcept(std::is_nothrow_default_constructible_v<allocator_type>) : pair_(FirstOneSecondArgs{}, alloc) {};

      explicit list(size_t n, allocator_type alloc = allocator_type())
        : pair_(FirstOneSecondArgs{}, alloc, std::pair<pointer, pointer>{nullptr, nullptr}), size_(0)
      {
        for (size_t i = 0; i < n; ++i) emplace_back(nullptr);
      }

      list(const list& o) : pair_(FirstOneSecondArgs{}, allocator_traits::select_on_container_copy_construction(o.pair_.first())), size_(o.size_) {
        for (auto& e : o) {
          emplace_back(e);
        }
      }

      list(list&& o) : pair_(FirstOneSecondArgs{}, std::move(o.pair_.first())), size_(o.size_) {
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

        auto& data = pair_.second_;
        auto& o_data = o.pair_.second_;

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
        return iterator(pair_.second_.first);
      };

      NODISCARD CONSTEXPR const_iterator begin() const noexcept {
        return const_iterator(pair_.second_.first);
      };

      NODISCARD CONSTEXPR const_iterator cbegin() const noexcept {
        return const_iterator(pair_.second_.first);
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


      CONSTEXPR bool empty() const noexcept { return size_ == 0; }
      size_type size() const noexcept { return size_; }

      // push_back (встановлюємо last_ і next_)
      CONSTEXPR node_type* push_back(const value_type& val) {
        return emplace_back(val);
      }

      CONSTEXPR node_type* push_back(value_type&& val) {
        return emplace_back(std::move(val));
      }

      CONSTEXPR void pop_back() {
        if (!pair_.second_.first) return;
        if (pair_.second_.first == pair_.second_.second) {
          _destroy_node(pair_.second_.first);
          pair_.second_.first = pair_.second_.second = nullptr;
          size_ = 0;
          return;
        }
        pointer tail = pair_.second_.second;
        pointer prev = tail->last_;
        _destroy_node(tail);
        prev->next_ = nullptr;
        pair_.second_.second = prev;
        --size_;
      }

      CONSTEXPR void pop_at(pointer loc) {
        if (!loc) return;

        if (loc->last_) loc->last_->next_ = loc->next_;
        else {
          pair_.second_.first = loc->next_;
          if (loc->next_) {
            loc->next_->last_ = nullptr;
          }
        }

        if (loc->next_) loc->next_->last_ = loc->last_;
        else {
          pair_.second_.second = loc->last_;
          if (loc->last_) {
            loc->last_->next_ = nullptr;
          }
        }

        _destroy_node(loc);
        --size_;
      }

      CONSTEXPR void clear() noexcept {
        node_type* cur = pair_.second_.first;
        while (cur) {
          node_type* next = cur->next_;
          _destroy_node(cur);
          cur = next;
        }
        pair_.second_.first = pair_.second_.second = nullptr;
        size_ = 0;
      }

      CONSTEXPR node_type* data() {
        return pair_.second_.first;
      }

      CONSTEXPR const allocator_type& get_allocator() const {
        return pair_.first();
      }
      CONSTEXPR allocator_type& get_allocator() {
        return pair_.first();
      }

      CONSTEXPR void _destroy_node(node_type* p) noexcept {
        if constexpr (!std::is_trivially_destructible_v<value_type>)
        {
          if constexpr (!type_traits::has_destroy_v<allocator_type, pointer>) {
            std::destroy_at(p);
          }
          else
          {
            pair_.first().destroy(p);
          }
        }
        pair_.first().deallocate(p, 1);
      }
    private:
      inline list& steal_no_cleanup(list& o) noexcept {
        pair_.second_.first = std::exchange(o.pair_.second_.first, nullptr);
        pair_.second_.second = std::exchange(o.pair_.second_.second, nullptr);
        size_ = std::exchange(o.size_, 0);
        return *this;
      }

      template<typename U>
      CONSTEXPR node_type* emplace_back(U&& val) {
        pointer p = pair_.first().allocate(1);
        // placement new з повною ініціалізацією last_:
        new (p) node_type(std::forward<U>(val), nullptr, pair_.second_.second);
        if (!pair_.second_.first) {
          pair_.second_.first = pair_.second_.second = p;
        }
        else {
          pair_.second_.second->next_ = p;
          p->last_ = pair_.second_.second;
          pair_.second_.second = p;
        }
        ++size_;
        return p;
      }

      void assign_move(list&& o) {
        size_t my_sz = size_;
        size_t other_sz = o.size_;
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
          size_ = other_sz;
        }
      }

      void assign_copy(const list& o) {
        size_t my_sz = size_;
        size_t other_sz = o.size_;
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
          size_ = other_sz;
        }
      }

    private:
      utility::compressed_pair<allocator_type, std::pair<pointer, pointer>> pair_;
      size_t size_;
    };
  }
}
