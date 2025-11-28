#pragma once
#include <xmemory>
#include <utility.h>

namespace audio {

  template<typename Alloc = std::allocator<float>>
  class audio_buffer {
    using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
    using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
  public:
    using allocator_type = Alloc;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using value_type = typename allocator_traits::value_type;        // повинно бути float
    using size_type = typename allocator_traits::size_type;
    using pointer = typename allocator_traits::pointer;
    using const_pointer = typename allocator_traits::const_pointer;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = pointer;
    using const_iterator = const_pointer;

    static_assert(std::is_same_v<value_type, float>, "Allocator::value_type must be float");

    audio_buffer() noexcept(std::is_nothrow_constructible_v<utility::compressed_pair<allocator_type, pointer>, FirstZeroSecondArgs>) : pair_(FirstZeroSecondArgs{}) {};
    explicit audio_buffer(size_type n, const allocator_type& alloc = allocator_type())
      noexcept(std::is_nothrow_default_constructible_v<allocator_type>&& std::is_nothrow_copy_assignable_v<allocator_type>)
      : pair_(FirstOneSecondArgs{}, std::forward<allocator_type&>(alloc)) 
    {
      pair_.second() = allocate_block(n);
      capacity_ = n;

      utility::uninitialized_default_construct_n(pair_.first(), pair_.second(), frames);
    };
    audio_buffer(const_pointer src, size_type n, const allocator_type& alloc = allocator_type()) noexcept(std::is_nothrow_default_constructible_v<allocator_type>) {
      
    };
    audio_buffer(const audio_buffer& other) noexcept();
    audio_buffer(audio_buffer&& other) noexcept(std::is_nothrow_move_constructible_v<allocator_type>);
    audio_buffer(size_type n, allocator_type&& alloc) noexcept(std::is_nothrow_move_constructible_v<allocator_type>);
    audio_buffer& operator=(const audio_buffer& other) noexcept(std::is_nothrow_copy_assignable_v<allocator_type>);
    audio_buffer& operator=(audio_buffer&& other) noexcept(std::is_nothrow_move_assignable_v<allocator_type>);
    void operator>>(const value_type& v) noexcept();
    void operator>>(value_type&& v) noexcept();
    void operator<<(const value_type& v) noexcept();
    void operator<<(value_type&& v) noexcept();
    ~audio_buffer();

    // Основні операції
    _CONSTEXPR20 void reserve(size_type n);
    _CONSTEXPR20 void resize(size_type n);
    _CONSTEXPR20 void shrink_to_fit();
    _CONSTEXPR20 void clear() noexcept;

    _NODISCARD _CONSTEXPR20 size_type size() const noexcept { return size_; }
    _NODISCARD _CONSTEXPR20 size_type capacity() const noexcept { return capacity_; }
    _NODISCARD _CONSTEXPR20 bool empty() const noexcept { return size_ == 0; }

    _NODISCARD _CONSTEXPR20 pointer data() noexcept;
    _NODISCARD _CONSTEXPR20 const_pointer data() const noexcept;

    // Random access
    _NODISCARD _CONSTEXPR20 reference operator[](size_type i);
    _NODISCARD _CONSTEXPR20 const_reference operator[](size_type i) const;
    _NODISCARD _CONSTEXPR20 reference at(size_type i);
    _NODISCARD _CONSTEXPR20 const_reference at(size_type i) const;

    // Iterators
    _NODISCARD _CONSTEXPR20 iterator begin() noexcept;
    _NODISCARD _CONSTEXPR20 const_iterator begin() const noexcept;
    _NODISCARD _CONSTEXPR20 iterator end() noexcept;
    _NODISCARD _CONSTEXPR20 const_iterator end() const noexcept;
    _NODISCARD _CONSTEXPR20 const_iterator cbegin() const noexcept;
    _NODISCARD _CONSTEXPR20 const_iterator cend() const noexcept;

    // Модифікатори
    _CONSTEXPR20 void push_back(const value_type& v);
    _CONSTEXPR20 void push_back(value_type&& v);
    template<typename InputIt>
    _CONSTEXPR20 void append(InputIt first, InputIt last);
    _CONSTEXPR20 void append(const_pointer src, size_type n);
    _CONSTEXPR20 void assign(size_type n, const value_type& v);

    _CONSTEXPR20 void swap(audio_buffer& other) noexcept(
      std::is_nothrow_swappable_v<utility::compressed_pair<allocator_type, pointer>>&&
      std::is_nothrow_swappable_v<size_type>);

    _NODISCARD _CONSTEXPR20 std::uint32_t sample_rate() const noexcept { return sample_rate_; }
    _CONSTEXPR20 void set_sample_rate(std::uint32_t sr) noexcept { sample_rate_ = sr; }

    _NODISCARD _CONSTEXPR20 std::uint16_t channels() const noexcept { return channels_; }
    _CONSTEXPR20 void set_channels(std::uint16_t ch) noexcept { channels_ = ch; }

    _NODISCARD _CONSTEXPR20 size_type frames() const noexcept {
      return channels_ == 0 ? 0 : (size_ / static_cast<size_type>(channels_));
    }

  private:
    utility::compressed_pair<allocator_type, pointer> pair_;
    size_type size_ = 0;
    size_type capacity_ = 0;
    std::uint32_t sample_rate_ = 0;
    std::uint16_t channels_ = 0;

    // приватні допоміжні функції
    pointer allocate_block(size_type n);
    void deallocate_block(pointer p, size_type n) noexcept;
    void destroy_and_deallocate() noexcept;
  };

  int foo() {
    audio_buffer<std::allocator<float>> a();
  }
} // namespace audio