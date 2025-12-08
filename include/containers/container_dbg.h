#pragma once
#include <vector>
#include <platform/platform.h>
#include <utility.h>
#include <multithreading/spin_lock.h>
#include <mutex>

namespace voxory {
  namespace containers {
    namespace debug {
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

          constexpr ConstListIterator() noexcept : ptr_(nullptr) {}
          constexpr explicit ConstListIterator(node_pointer ptr) noexcept : ptr_(ptr) {}

          constexpr reference operator*() const noexcept { ASSERT_ABORT(ptr_ != nullptr, ""); return ptr_->data_; }
          constexpr const_pointer operator->() const noexcept { ASSERT_ABORT(ptr_ != nullptr, ""); return &ptr_->data_; }

          constexpr ConstListIterator& operator++() noexcept {
            ASSERT_ABORT(ptr_ != nullptr, "");
            ptr_ = ptr_->next_;
            return *this;
          }
          constexpr ConstListIterator operator++(int) noexcept {
            ConstListIterator tmp = *this;
            ++(*this);
            return tmp;
          }

          constexpr ConstListIterator& operator--() noexcept {
            ASSERT_ABORT(ptr_ != nullptr, "");
            ptr_ = ptr_->last_;
            return *this;
          }
          constexpr ConstListIterator operator--(int) noexcept {
            ConstListIterator tmp = *this;
            --(*this);
            return tmp;
          }

          constexpr ConstListIterator& operator+=(difference_type off) noexcept {
            if (off >= 0) {
              while (off--) {
                ASSERT_ABORT(ptr_ != nullptr, "");
                ptr_ = ptr_->next_;
              }
            }
            else {
              while (off++) {
                ASSERT_ABORT(ptr_ != nullptr, "");
                ptr_ = ptr_->last_;
              }
            }
            return *this;
          }
          constexpr ConstListIterator operator+(difference_type off) const noexcept {
            ConstListIterator tmp = *this; tmp += off; return tmp;
          }

          constexpr ConstListIterator& operator-=(difference_type off) noexcept {
            return *this += -off;
          }
          constexpr ConstListIterator operator-(difference_type off) const noexcept {
            ConstListIterator tmp = *this; tmp -= off; return tmp;
          }

          constexpr bool operator==(const ConstListIterator& other) const noexcept { return ptr_ == other.ptr_; }
          constexpr bool operator!=(const ConstListIterator& other) const noexcept { return ptr_ != other.ptr_; }

          constexpr reference operator[](difference_type off) const noexcept {
            node_pointer tmp = ptr_;
            if (off >= 0) {
              while (off--) { ASSERT_ABORT(tmp != nullptr, ""); tmp = tmp->next_; }
            }
            else {
              while (off++) { ASSERT_ABORT(tmp != nullptr, ""); tmp = tmp->last_; }
            }
            return tmp->data_;
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

          constexpr ListIterator() noexcept : base_type(nullptr) {}
          constexpr explicit ListIterator(node_pointer ptr) noexcept : base_type(ptr) {}

          constexpr reference operator*() const noexcept {
            return const_cast<reference>(base_type::operator*());
          }
          constexpr pointer operator->() const noexcept {
            return &this->ptr_->data_;
          }

          constexpr ListIterator& operator++() noexcept { base_type::operator++(); return *this; }
          constexpr ListIterator operator++(int) noexcept { ListIterator tmp = *this; base_type::operator++(); return tmp; }
          constexpr ListIterator& operator--() noexcept { base_type::operator--(); return *this; }
          constexpr ListIterator operator--(int) noexcept { ListIterator tmp = *this; base_type::operator--(); return tmp; }

          constexpr ListIterator& operator+=(difference_type off) noexcept { base_type::operator+=(off); return *this; }
          constexpr ListIterator& operator-=(difference_type off) noexcept { base_type::operator-=(off); return *this; }
          constexpr ListIterator operator+(difference_type off) const noexcept { ListIterator tmp = *this; tmp += off; return tmp; }
          constexpr ListIterator operator-(difference_type off) const noexcept { ListIterator tmp = *this; tmp -= off; return tmp; }

          constexpr bool operator==(const ListIterator& other) const noexcept { return base_type::operator==(other); }
          constexpr bool operator!=(const ListIterator& other) const noexcept { return base_type::operator!=(other); }

          constexpr reference operator[](difference_type off) const noexcept {
            return const_cast<reference>(base_type::operator[](off));
          }
        };

        class iterator_base_dbg;

        struct node {
          using value_type = iterator_base_dbg*;
          node() : data_(nullptr), next_(nullptr), last_(nullptr) {}
          node(value_type data, node* next = nullptr, node* last = nullptr)
            : data_(data), next_(next), last_(last) {
          }
          mutable value_type data_;
          mutable node* next_;
          mutable node* last_;
        };

        class list {
        public:
          using node_type = node;
          using allocator_type = std::allocator<node>;
          using value_type = node::value_type;
          using pointer = value_type*;
          using const_pointer = const pointer;
          using reference = value_type&;
          using const_reference = const reference;
          using iterator = ListIterator<list>;
          using const_iterator = ConstListIterator<list>;
          using size_type = std::size_t;

          explicit list(size_t n = 0, allocator_type alloc = allocator_type())
            : pair_(utility::detail::FirstOneSecondArgs{}, alloc, std::pair<node*, node*>{nullptr, nullptr}), size_(0)
          {
            for (size_t i = 0; i < n; ++i) push_back(nullptr);
          }

          ~list() {
            clear();
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
          CONSTEXPR node* push_back(value_type val) {
            node* p = pair_.first().allocate(1);
            // placement new з повною ініціалізацією last_:
            new (p) node(val, nullptr, pair_.second_.second);
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

          CONSTEXPR void pop_back() {
            if (!pair_.second_.first) return;
            if (pair_.second_.first == pair_.second_.second) {
              _destroy_node(pair_.second_.first);
              pair_.second_.first = pair_.second_.second = nullptr;
              size_ = 0;
              return;
            }
            node* tail = pair_.second_.second;
            node* prev = tail->last_;
            _destroy_node(tail);
            prev->next_ = nullptr;
            pair_.second_.second = prev;
            --size_;
          }

          CONSTEXPR void clear() noexcept {
            node* cur = pair_.second_.first;
            while (cur) {
              node* next = cur->next_;
              _destroy_node(cur);
              cur = next;
            }
            pair_.second_.first = pair_.second_.second = nullptr;
            size_ = 0;
          }
          CONSTEXPR node* data() {
            return pair_.second_.first;
          }

          CONSTEXPR void _destroy_node(node* p) noexcept {
            std::allocator_traits<allocator_type>::destroy(pair_.first(), p);
            pair_.first().deallocate(p, 1);
          }

        private:

          utility::compressed_pair<allocator_type, std::pair<node*, node*>> pair_;
          size_t size_;
        };

        class container_base_dbg {
        public:
          void release_proxy() noexcept {
            std::lock_guard<multithreading::spin_lock> guard(sl_);
            for (auto& elem : list_)
              elem->proxy_ = nullptr;
            list_.clear();
          }

          list list_;
          multithreading::spin_lock sl_;
        };


        class iterator_base_dbg {
        public:
          const container_base_dbg* container_ = nullptr;
          node* proxy_ = nullptr;

          iterator_base_dbg(container_base_dbg* container) : container_(container) {
            register_to_container(container);
          }

          iterator_base_dbg(const container_base_dbg* container) : container_(container) {
            register_to_container(container);
          }

          CONSTEXPR void release_on_destroy() {
            if (!proxy_) return;
            auto nonconst_container = const_cast<container_base_dbg*>(container_);
            std::lock_guard<multithreading::spin_lock> guard(nonconst_container->sl_);

            if (proxy_->last_) proxy_->last_->next_ = proxy_->next_;
            if (proxy_->next_) proxy_->next_->last_ = proxy_->last_;
            nonconst_container->list_._destroy_node(proxy_);
            proxy_ = nullptr;
          }

        private:
          void register_to_container(const container_base_dbg* container) {
            auto nonconst_container = const_cast<container_base_dbg*>(container);
            std::lock_guard<multithreading::spin_lock> guard(nonconst_container->sl_);
            proxy_ = nonconst_container->list_.push_back(const_cast<iterator_base_dbg*>(this));
          }
        };

        class iterator_base_rls {
        public:
          iterator_base_rls(container_base_dbg* container) {};
          CONSTEXPR void release_on_destroy() {};
        };
        class container_base_rls {
        public:
          CONSTEXPR void update_proxy() noexcept {}
        };
      }
#ifdef DEBUG_ITERATORS
      using container_base = detail::container_base_dbg;
      using iterator_base = detail::iterator_base_dbg;
#else
      using container_base = detail::container_base_rls;
      using iterator_base = detail::iterator_base_rls;
#endif // DEBUG

    }
  }
}