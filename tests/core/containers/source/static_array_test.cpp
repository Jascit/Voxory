// static_array_tests.cpp
#include <tests_details.h>
#include <containers/impl/static_array.h>
#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <chrono>

using voxory::containers::static_array;

// Test 1: basic construct, capacity, data, operator[], begin/end
NOYX_TEST(static_array_test, basic_int_behavior) {
  constexpr size_t N = 8;
  static_array<int, N> a;

  NOYX_ASSERT_EQ(a.capacity(), (static_array<int, N>::size_type)N);

  auto p = a.data();
  NOYX_ASSERT_TRUE(p != nullptr || N == 0);

  for (size_t i = 0; i < N; ++i)
    a[(static_array<int, N>::size_type)i] = static_cast<int>(i * 3 + 1);

  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(a[(static_array<int, N>::size_type)i], (int)(i * 3 + 1));
    if (p) NOYX_ASSERT_EQ(p[i], (int)(i * 3 + 1));
  }

  size_t cnt = 0;
  for (auto it = a.begin(); it != a.end(); ++it) ++cnt;
  NOYX_ASSERT_EQ(cnt, N);
}

// Test 2: copy-constructor from base (deep copy)
NOYX_TEST(static_array_test, copy_constructor_deep_strings) {
  constexpr size_t N = 6;
  static_array<std::string, N> src;
  for (size_t i = 0; i < N; ++i)
    src[(static_array<std::string, N>::size_type)i] = "s" + std::to_string(i);

  const voxory::containers::detail::static_array_base<std::string>& base_ref =
    reinterpret_cast<const voxory::containers::detail::static_array_base<std::string>&>(src);

  static_array<std::string, N> dst(base_ref);

  dst[(static_array<std::string, N>::size_type)0] = "CHANGED";
  NOYX_ASSERT_EQ(src[(static_array<std::string, N>::size_type)0], std::string("s0"));
  NOYX_ASSERT_EQ(dst[(static_array<std::string, N>::size_type)0], std::string("CHANGED"));
}

// Test 3: copy-assignment from base
NOYX_TEST(static_array_test, copy_assignment) {
  constexpr size_t N = 5;
  static_array<int, N> a;
  for (size_t i = 0; i < N; ++i)
    a[(static_array<int, N>::size_type)i] = (int)(10 + i);

  static_array<int, N> b;
  const voxory::containers::detail::static_array_base<int>& base_ref =
    reinterpret_cast<const voxory::containers::detail::static_array_base<int>&>(a);

  b = base_ref;

  b[(static_array<int, N>::size_type)1] = 77;
  NOYX_ASSERT_EQ(a[(static_array<int, N>::size_type)1], 11);
  NOYX_ASSERT_EQ(b[(static_array<int, N>::size_type)1], 77);
}

// Test 4: move semantics
NOYX_TEST(static_array_test, move_semantics_strings) {
  constexpr size_t N = 4;
  static_array<std::string, N> a;
  for (size_t i = 0; i < N; ++i)
    a[(static_array<std::string, N>::size_type)i] = "v" + std::to_string(i);

  auto&& rbase =
    reinterpret_cast<voxory::containers::detail::static_array_base<std::string>&&>(a);

  static_array<std::string, N> moved(std::move(rbase));

  for (size_t i = 0; i < N; ++i)
    NOYX_ASSERT_EQ(moved[(static_array<std::string, N>::size_type)i],
      std::string("v" + std::to_string(i)));

  static_array<std::string, N> x;
  for (size_t i = 0; i < N; ++i)
    x[(static_array<std::string, N>::size_type)i] = "X" + std::to_string(i);

  auto&& rbase2 =
    reinterpret_cast<voxory::containers::detail::static_array_base<std::string>&&>(moved);

  x = std::move(rbase2);

  for (size_t i = 0; i < N; ++i)
    NOYX_ASSERT_EQ(x[(static_array<std::string, N>::size_type)i],
      std::string("v" + std::to_string(i)));
}

// Test 5: iterators arithmetic
NOYX_TEST(static_array_test, iterators_arithmetic) {
  constexpr size_t N = 10;
  static_array<int, N> a;
  for (size_t i = 0; i < N; ++i)
    a[(static_array<int, N>::size_type)i] = (int)(i + 1);

  auto it = a.begin();
  it += 5;
  NOYX_ASSERT_EQ(*it, 6);
  it -= 3;
  NOYX_ASSERT_EQ(*it, 3);

  auto it2 = a.begin() + 7;
  NOYX_ASSERT_EQ((int)(it2 - a.begin()), 7);
}

// Test 6: zero-sized behaviour
NOYX_TEST(static_array_test, zero_size) {
  static_array<int, 0> z;
  NOYX_ASSERT_EQ(z.capacity(), (static_array<int, 0>::size_type)0);
  NOYX_ASSERT_TRUE(z.begin() == z.end());
  (void)z.data();
}

// Test 7: swap
NOYX_TEST(static_array_test, swap) {
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

  NOYX_ASSERT_EQ(a[(static_array<int, N>::size_type)0] * 10, b0);
  NOYX_ASSERT_EQ(b[(static_array<int, N>::size_type)0], a0 * 10);
}

// Test 8: RAII smoke test
NOYX_TEST(static_array_test, stress_alloc_dealloc) {
  for (int iter = 0; iter < 50000; ++iter) {
    switch (iter % 16) {
    case 0: { static_array<int, 0> a; break; }
    case 1: { static_array<int, 1> a; a[0] = 1; break; }
    case 2: { static_array<int, 2> a; a[1] = 2; break; }
    default: {
      static_array<int, 8> a;
      if (a.capacity() > 0) a[0] = iter;
      break;
    }
    }
  }
}
