// heap_array_tests.cpp
#include <tests_details.h>
#include <containers/impl/heap_array.h>
#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <stdexcept>

using voxory::containers::debug_policy;
using voxory::containers::heap_array;

// Test 1: basic construct, capacity, data, operator[], begin/end
void heap_array_test_basic_int_behavior() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  constexpr size_t N = 8;
  heap_t a(N); // default-constructed elements

  NOYX_ASSERT_EQ(a.capacity(), (heap_t::size_type)N);

  auto p = a.data();
  NOYX_ASSERT_TRUE(p != nullptr || N == 0);

  for (size_t i = 0; i < N; ++i) {
    a[(heap_t::size_type)i] = static_cast<int>(i * 5 + 2);
  }
  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(a[(heap_t::size_type)i], (int)(i * 5 + 2));
    if (p) NOYX_ASSERT_EQ(p[i], (int)(i * 5 + 2));
  }

  size_t cnt = 0;
  for (auto it = a.begin(); it != a.end(); ++it) ++cnt;
  NOYX_ASSERT_EQ(cnt, N);
}

// Test 2: construct with fill value (value ctor)
void heap_array_test_construct_with_value() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  constexpr size_t N = 5;
  heap_t a(N, 42); // fill with 42 via rvalue ctor
  NOYX_ASSERT_EQ(a.size(), (heap_t::size_type)N);
  for (size_t i = 0; i < N; ++i) NOYX_ASSERT_EQ(a[(heap_t::size_type)i], 42);
}

// Test 3: at() bounds checking
void heap_array_test_at_throws() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  heap_t a(3);
  NOYX_ASSERT_EQ(a.size(), (heap_t::size_type)3);
  bool thrown = false;
  try {
    [[maybe_unused]] auto p = a.at(5);
  }
  catch (const std::out_of_range&) {
    thrown = true;
  }
  catch (...) {
    thrown = false;
  }
  NOYX_ASSERT_TRUE(thrown);
}

// Test 4: copy-constructor deep copy
void heap_array_test_copy_constructor_deep_strings() {
  using policy_t = debug_policy<std::string>;
  using heap_t = heap_array<policy_t>;
  constexpr size_t N = 4;
  heap_t src(N);
  for (size_t i = 0; i < N; ++i) src[(heap_t::size_type)i] = "s" + std::to_string(i);

  heap_t dst(src); // copy ctor
  dst[(heap_t::size_type)0] = "CHANGED";
  NOYX_ASSERT_EQ(src[(heap_t::size_type)0], std::string("s0"));
  NOYX_ASSERT_EQ(dst[(heap_t::size_type)0], std::string("CHANGED"));
}

// Test 5: copy-assignment
void heap_array_test_copy_assignment() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  constexpr size_t N = 6;
  heap_t a(N);
  for (size_t i = 0; i < N; ++i) a[(heap_t::size_type)i] = (int)(100 + i);

  heap_t b(2);
  b = a; // copy assign

  b[(heap_t::size_type)1] = 777;
  NOYX_ASSERT_EQ(a[(heap_t::size_type)1], 101);
  NOYX_ASSERT_EQ(b[(heap_t::size_type)1], 777);
}

// Test 6: move-constructor and move-assignment
void heap_array_test_move_semantics_strings() {
  using policy_t = debug_policy<std::string>;
  using heap_t = heap_array<policy_t>;
  constexpr size_t N = 3;
  heap_t a(N);
  for (size_t i = 0; i < N; ++i) a[(heap_t::size_type)i] = "v" + std::to_string(i);

  heap_t moved(std::move(a)); // move ctor

  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(moved[(heap_t::size_type)i], std::string("v" + std::to_string(i)));
  }

  heap_t x(N);
  for (size_t i = 0; i < N; ++i) x[(heap_t::size_type)i] = "X" + std::to_string(i);
  x = std::move(moved); // move assign

  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(x[(heap_t::size_type)i], std::string("v" + std::to_string(i)));
  }
}

// Test 7: iterators random access & arithmetic
void heap_array_test_iterators_arithmetic() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  constexpr size_t N = 12;
  heap_t a(N);
  for (size_t i = 0; i < N; ++i) a[(heap_t::size_type)i] = (int)(i + 1);

  auto it = a.begin();
  it += 5;
  NOYX_ASSERT_EQ(*it, 6);
  it -= 3;
  NOYX_ASSERT_EQ(*it, 3);

  auto it2 = a.begin() + 7;
  NOYX_ASSERT_EQ((int)(it2 - a.begin()), 7);

  NOYX_ASSERT_EQ(a[0], 1);
  NOYX_ASSERT_EQ(a[5], 6);
  NOYX_ASSERT_EQ(a[11], 12);
}

// Test 8: reserve / resize behaviour
void heap_array_test_reserve_and_resize() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  heap_t a(4);
  for (size_t i = 0; i < 4; ++i) a[(heap_t::size_type)i] = (int)(i + 1);

  auto old_cap = a.capacity();
  a.reserve(old_cap + 10);
  NOYX_ASSERT_TRUE(a.capacity() >= old_cap + 10);
  NOYX_ASSERT_EQ(a.size(), (heap_t::size_type)4);

  // resize downward (shrinks capacity to given value)
  a.resize(2);
  NOYX_ASSERT_EQ(a.capacity(), (heap_t::size_type)2);
  NOYX_ASSERT_EQ(a.size(), (heap_t::size_type)2);
  // values preserved in the front
  NOYX_ASSERT_EQ(a[(heap_t::size_type)0], 1);
  NOYX_ASSERT_EQ(a[(heap_t::size_type)1], 2);
}

// Test 9: zero-sized heap_array behaviour
void heap_array_test_zero_size() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  heap_t z(0);
  NOYX_ASSERT_EQ(z.capacity(), (heap_t::size_type)0);
  auto b = z.begin();
  auto e = z.end();
  NOYX_ASSERT_TRUE(b == e);
  (void)z.data();
}

// Test 10: swap
void heap_array_test_swap() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;
  constexpr size_t N = 4;
  heap_t a(N);
  heap_t b(N);
  for (size_t i = 0; i < N; ++i) {
    a[(heap_t::size_type)i] = (int)(i + 1);
    b[(heap_t::size_type)i] = (int)((i + 1) * 10);
  }

  int a0 = a[(heap_t::size_type)0];
  int b0 = b[(heap_t::size_type)0];

  std::swap(a, b);

  NOYX_ASSERT_EQ(a[(heap_t::size_type)0] * 10, b0);
  NOYX_ASSERT_EQ(b[(heap_t::size_type)0], a0 * 10);
}

// grouping & runner
NOYX_TEST(heap_array_test, unit_test) {
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

  time_and_run(heap_array_test_basic_int_behavior, "basic_int_behavior");
  time_and_run(heap_array_test_construct_with_value, "construct_with_value");
  time_and_run(heap_array_test_at_throws, "at_throws");
  time_and_run(heap_array_test_copy_constructor_deep_strings, "copy_constructor_deep_strings");
  time_and_run(heap_array_test_copy_assignment, "copy_assignment");
  time_and_run(heap_array_test_move_semantics_strings, "move_semantics_strings");
  time_and_run(heap_array_test_iterators_arithmetic, "iterators_arithmetic");
  time_and_run(heap_array_test_reserve_and_resize, "reserve_and_resize");
  time_and_run(heap_array_test_zero_size, "zero_size");
  time_and_run(heap_array_test_swap, "swap");

  return;
}
