// static_array_tests.cpp
#include <tests_details.h>
#include <containers/static_array.h> // <-- підкоригуй шлях, якщо потрібно
#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <chrono>

using containers::static_array;

// Test 1: basic construct, capacity, data, operator[], begin/end
void static_array_test_basic_int_behavior() {
  constexpr size_t N = 8;
  static_array<int, N> a;

  // capacity() must equal template parameter
  NOYX_ASSERT_EQ(a.capacity(), (static_array<int, N>::size_type)N);

  // data() must be valid for N>0
  auto p = a.data();
  NOYX_ASSERT_TRUE(p != nullptr || N == 0);

  // write via operator[] and read back
  for (size_t i = 0; i < N; ++i) {
    a[(static_array<int, N>::size_type)i] = static_cast<int>(i * 3 + 1);
  }
  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(a[(static_array<int, N>::size_type)i], (int)(i * 3 + 1));
    if (p) NOYX_ASSERT_EQ(p[i], (int)(i * 3 + 1));
  }

  // iterators begin/end should span capacity elements
  size_t cnt = 0;
  for (auto it = a.begin(); it != a.end(); ++it) ++cnt;
  NOYX_ASSERT_EQ(cnt, N);
}

// Test 2: copy-constructor from base (deep copy) with non-trivial type (std::string)
void static_array_test_copy_constructor_deep_strings() {
  constexpr size_t N = 6;
  static_array<std::string, N> src;
  for (size_t i = 0; i < N; ++i) src[(static_array<std::string, N>::size_type)i] = "s" + std::to_string(i);

  // use base class copy-constructor: static_array(const detail::static_array_base<T>&)
  const containers::detail::static_array_base<std::string>& base_ref = reinterpret_cast<const containers::detail::static_array_base<std::string>&>(src);
  static_array<std::string, N> dst(base_ref);

  // deep copy: modifying dst must not change src
  dst[(static_array<std::string, N>::size_type)0] = "CHANGED";
  NOYX_ASSERT_EQ(src[(static_array<std::string, N>::size_type)0], std::string("s0"));
  NOYX_ASSERT_EQ(dst[(static_array<std::string, N>::size_type)0], std::string("CHANGED"));
}

// Test 3: copy-assignment from base
void static_array_test_copy_assignment() {
  constexpr size_t N = 5;
  static_array<int, N> a;
  for (size_t i = 0; i < N; ++i) a[(static_array<int, N>::size_type)i] = (int)(10 + i);

  static_array<int, N> b;
  const containers::detail::static_array_base<int>& base_ref = reinterpret_cast<const containers::detail::static_array_base<int>&>(a);
  b = base_ref;

  // modifying b must not change a (deep copy)
  b[(static_array<int, N>::size_type)1] = 77;
  NOYX_ASSERT_EQ(a[(static_array<int, N>::size_type)1], 11);
  NOYX_ASSERT_EQ(b[(static_array<int, N>::size_type)1], 77);
}

// Test 4: move-constructor and move-assignment using base rvalue
void static_array_test_move_semantics_strings() {
  constexpr size_t N = 4;
  static_array<std::string, N> a;
  for (size_t i = 0; i < N; ++i) a[(static_array<std::string, N>::size_type)i] = "v" + std::to_string(i);

  // move-construct from base rvalue
  containers::detail::static_array_base<std::string>&& rbase = reinterpret_cast<containers::detail::static_array_base<std::string>&&>(a);
  static_array<std::string, N> moved(std::move(rbase));

  // moved should contain the original values (though originals may be in moved-from state)
  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(moved[(static_array<std::string, N>::size_type)i], std::string("v" + std::to_string(i)));
  }

  // move-assignment: create another and move-assign
  static_array<std::string, N> x;
  for (size_t i = 0; i < N; ++i) x[(static_array<std::string, N>::size_type)i] = "X" + std::to_string(i);
  containers::detail::static_array_base<std::string>&& rbase2 = reinterpret_cast<containers::detail::static_array_base<std::string>&&>(moved);
  x = std::move(rbase2);

  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(x[(static_array<std::string, N>::size_type)i], std::string("v" + std::to_string(i)));
  }
}

// Test 5: iterators random access & arithmetic
void static_array_test_iterators_arithmetic() {
  constexpr size_t N = 10;
  static_array<int, N> a;
  for (size_t i = 0; i < N; ++i) a[(static_array<int, N>::size_type)i] = (int)(i + 1);

  auto it = a.begin();
  it += 5;
  NOYX_ASSERT_EQ(*it, 6);
  it -= 3;
  NOYX_ASSERT_EQ(*it, 3);

  auto it2 = a.begin() + 7;
  NOYX_ASSERT_EQ((int)(it2 - a.begin()), 7);
}

// Test 6: zero-sized static_array behaviour
void static_array_test_zero_size() {
  static_array<int, 0> z;
  NOYX_ASSERT_EQ(z.capacity(), (static_array<int, 0>::size_type)0);

  auto b = z.begin();
  auto e = z.end();
  NOYX_ASSERT_TRUE(b == e);
  // data() may be nullptr for zero-sized arrays
  (void)z.data();
}

// Test 7: swap
void static_array_test_swap() {
  constexpr size_t N = 4;
  static_array<int, N> a;
  static_array<int, N> b;
  for (size_t i = 0; i < N; ++i) { 
    a[(static_array<int, N>::size_type)i] = (int)(i + 1); 
    b[(static_array<int, N>::size_type)i] = (int)((i + 1) * 10); 
  }

  int a0 = a[(static_array<int, N>::size_type)0];
  int b0 = b[(static_array<int, N>::size_type)0];

  std::swap(a, b);

  NOYX_ASSERT_EQ(a[(static_array<int, N>::size_type)0]*10, b0);
  NOYX_ASSERT_EQ(b[(static_array<int, N>::size_type)0], a0*10);
}

// Test 8: RAII smoke test (stack construction/destruction)
void static_array_test_stress_alloc_dealloc() {
  for (int iter = 0; iter < 50000; ++iter) {
    // sizes cycle 0..15
    constexpr int MAX = 16;
    switch (iter % MAX) {
    case 0: { static_array<int, 0> a; break; }
    case 1: { static_array<int, 1> a; a[(static_array<int, 1>::size_type)0] = 1; break; }
    case 2: { static_array<int, 2> a; a[(static_array<int, 2>::size_type)1] = 2; break; }
    default: { static_array<int, 8> a; if (a.capacity() > 0) a[(static_array<int, 8>::size_type)0] = iter; break; }
    }
  }
}
#include <array>
// Entry point grouping
NOYX_TEST(static_array_test, unit_test) {
  using clk = std::chrono::steady_clock;
  using rep = std::chrono::duration<double, std::milli>;

  auto time_and_run = [&](auto&& fn, const char* name) {
    auto s = clk::now();
    fn();
    auto e = clk::now();
    rep d = e - s;
    std::cout.setf(std::ios::fixed); std::cout << std::setprecision(3);
    std::cout << "[TIMING] " << name << " : " << d.count() << " ms\n";
    std::cout.unsetf(std::ios::fixed);
    };

  time_and_run(static_array_test_basic_int_behavior, "basic_int_behavior");
  time_and_run(static_array_test_copy_constructor_deep_strings, "copy_constructor_deep_strings");
  time_and_run(static_array_test_copy_assignment, "copy_assignment");
  time_and_run(static_array_test_move_semantics_strings, "move_semantics_strings");
  time_and_run(static_array_test_iterators_arithmetic, "iterators_arithmetic");
  time_and_run(static_array_test_zero_size, "zero_size");
  time_and_run(static_array_test_swap, "swap");
  time_and_run(static_array_test_stress_alloc_dealloc, "stress_alloc_dealloc");

  return;
}
