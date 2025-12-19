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
#include <thread>
#include <vector>
#include <random>
#include <atomic>

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

  // resize downward
  a.resize(2);
  NOYX_ASSERT_EQ(a.capacity(), (old_cap + 10));
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

  NOYX_ASSERT_EQ(a[(heap_t::size_type)0], b0);
  NOYX_ASSERT_EQ(b[(heap_t::size_type)0], a0);
}

void heap_array_test_smoke() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  heap_t a(1);
  NOYX_ASSERT_EQ(a.size(), 1);
  a[0] = 123;
  NOYX_ASSERT_EQ(a[0], 123);

  a.reserve(10);
  NOYX_ASSERT_TRUE(a.capacity() >= 10);
  NOYX_ASSERT_EQ(a[0], 123);

  a.resize(1);
  NOYX_ASSERT_EQ(a.size(), 1);
  NOYX_ASSERT_EQ(a[0], 123);
}

void heap_array_test_self_copy_assignment() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  heap_t a(5);
  for (size_t i = 0; i < 5; ++i)
    a[i] = (int)i;

  a = a; // self assign

  for (size_t i = 0; i < 5; ++i)
    NOYX_ASSERT_EQ(a[i], (int)i);
}

void heap_array_test_self_move_assignment() {
  using policy_t = debug_policy<std::string>;
  using heap_t = heap_array<policy_t>;

  heap_t a(3);
  a[0] = "a"; a[1] = "b"; a[2] = "c";

  a = std::move(a); 

  NOYX_ASSERT_EQ(a.size(), 3);
  NOYX_ASSERT_EQ(a[0], "a");
  NOYX_ASSERT_EQ(a[1], "b");
  NOYX_ASSERT_EQ(a[2], "c");
}


void heap_array_test_const_access() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  heap_t a(4);
  for (size_t i = 0; i < 4; ++i)
    a[i] = (int)(10 + i);

  const heap_t& ca = a;

  NOYX_ASSERT_EQ(ca[0], 10);
  NOYX_ASSERT_EQ(ca.at(3), 13);

  size_t cnt = 0;
  for (auto it = ca.begin(); it != ca.end(); ++it)
    ++cnt;

  NOYX_ASSERT_EQ(cnt, 4);
}

void heap_array_test_stress_large() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  constexpr size_t N = 1'000'000;
  heap_t a(N);

  for (size_t i = 0; i < N; ++i)
    a[i] = (int)(i ^ 0xDEADBEEF);

  long long checksum = 0;
  for (size_t i = 0; i < N; ++i)
    checksum += a[i];

  NOYX_ASSERT_TRUE(checksum != 0);
}

void heap_array_test_multiple_reserve_preserves_data() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  heap_t a(2);
  a[0] = 1;
  a[1] = 2;

  for (int i = 0; i < 10; ++i) {
    a.reserve(a.capacity() + 5);
    NOYX_ASSERT_EQ(a[0], 1);
    NOYX_ASSERT_EQ(a[1], 2);
  }
}

void heap_array_test_resize_to_zero_and_back() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  heap_t a(5);
  for (size_t i = 0; i < 5; ++i)
    a[i] = (int)i;

  a.resize(0);
  NOYX_ASSERT_EQ(a.size(), 0);
  NOYX_ASSERT_EQ(a.capacity(), 5);

  a.resize(3);
  NOYX_ASSERT_EQ(a.size(), 3);
  NOYX_ASSERT_EQ(a.capacity(), 5);
}

void heap_array_test_stress_iterators_multithreaded() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  constexpr heap_t::size_type N = 10'000;
  heap_t a(N);
  for (heap_t::size_type i = 0; i < N; ++i) a[i] = static_cast<int>(i);

  // thread count and iterations tuned to be heavy but reasonable in CI
  unsigned hc = std::thread::hardware_concurrency();
  size_t threads = hc == 0 ? 4u : std::min<unsigned>(hc, 8u);
  const size_t iters_per_thread = 5000;

  std::atomic<bool> failed{ false };

  auto worker = [&](unsigned seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<heap_t::size_type> dist(0, N - 1);

    for (size_t it = 0; it < iters_per_thread; ++it) {
      // pick random index and create iterator to it
      heap_t::size_type idx = dist(rng);
      auto itpos = a.begin() + idx;
      int val = *itpos;
      // assert observed value equals expected
      NOYX_ASSERT_EQ(val, (int)idx);
      // also create const_iterator and read again
      const heap_t& ca = a;
      auto cit = ca.begin() + idx;
      int cval = *cit;
      NOYX_ASSERT_EQ(cval, (int)idx);

      // occasionally walk a short range from this iterator (read-only)
      if ((it & 0xF) == 0) {
        auto it2 = itpos;
        size_t steps = std::min<heap_t::size_type>(16u, N - idx);
        for (size_t s = 0; s < steps; ++s, ++it2) {
          NOYX_ASSERT_EQ(*it2, (int)(idx + s));
        }
      }

      if (failed.load(std::memory_order_relaxed)) return;
    }
    };

  std::vector<std::thread> ths;
  ths.reserve(threads);
  for (size_t t = 0; t < threads; ++t) {
    ths.emplace_back(worker, static_cast<unsigned>(std::random_device{}() ^ (unsigned)t));
  }
  for (auto& th : ths) th.join();
}

void heap_array_test_stress_alloc_dealloc_multithreaded() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  unsigned hc = std::thread::hardware_concurrency();
  size_t threads = hc == 0 ? 4u : std::min<unsigned>(hc, 8u);
  const size_t iters_per_thread = 3000;
  const heap_t::size_type max_size = 2000;

  std::atomic<bool> failed{ false };

  auto worker = [&](unsigned seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<heap_t::size_type> dist_size(0, max_size);
    for (size_t it = 0; it < iters_per_thread; ++it) {
      heap_t::size_type n = dist_size(rng);
      heap_t local(n);
      // touch a few elements to ensure construction & prevent trivial optimizations
      if (n > 0) {
        size_t limit = std::min<heap_t::size_type>(n, 10);
        for (heap_t::size_type i = 0; i < limit; ++i) local[i] = static_cast<int>(i + (it & 0xFF));
        // verify reads
        for (heap_t::size_type i = 0; i < limit; ++i) NOYX_ASSERT_EQ(local[i], (int)(i + (it & 0xFF)));
      }
      // local goes out of scope and deallocates
      if (failed.load(std::memory_order_relaxed)) return;
    }
    };

  std::vector<std::thread> ths;
  ths.reserve(threads);
  for (size_t t = 0; t < threads; ++t) {
    ths.emplace_back(worker, static_cast<unsigned>(std::random_device{}() ^ (unsigned)t));
  }
  for (auto& th : ths) th.join();
}

void heap_array_test_stress_alloc_dealloc_singlethreaded() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  const size_t iters = 3000 * 8;               
  const heap_t::size_type max_size = 2000;

  std::mt19937_64 rng(static_cast<unsigned long long>(std::random_device{}()));
  std::uniform_int_distribution<heap_t::size_type> dist_size(0, max_size);

  for (size_t it = 0; it < iters; ++it) {
    heap_t::size_type n = dist_size(rng);

    heap_t local(n);

    if (n > 0) {
      size_t limit = static_cast<size_t>(std::min<heap_t::size_type>(n, 10));
      int pattern = static_cast<int>(it & 0xFF);

      for (heap_t::size_type i = 0; i < limit; ++i) {
        local[i] = static_cast<int>(i + pattern);
      }

      for (heap_t::size_type i = 0; i < limit; ++i) {
        NOYX_ASSERT_EQ(local[i], static_cast<int>(i + pattern));
      }
    }
  }
}


void heap_array_test_shrink_to_fit_behavior() {
  using policy_t = debug_policy<int>;
  using heap_t = heap_array<policy_t>;

  // start with a known capacity
  constexpr heap_t::size_type initial_cap = 100;
  heap_t a(initial_cap);
  for (heap_t::size_type i = 0; i < initial_cap; ++i) a[i] = static_cast<int>(i);

  NOYX_ASSERT_EQ(a.capacity(), (heap_t::size_type)initial_cap);
  // resize smaller: should not reduce capacity_
  a.resize(10);
  NOYX_ASSERT_EQ(a.size(), (heap_t::size_type)10);
  // capacity must remain at least the initial capacity (i.e. not decreased by resize)
  NOYX_ASSERT_TRUE(a.capacity() >= initial_cap);

  // Now call shrink_to_fit() - expects capacity to be reduced to size (10)
  a.shrink_to_fit();
  NOYX_ASSERT_EQ(a.size(), (heap_t::size_type)10);
  NOYX_ASSERT_EQ(a.capacity(), (heap_t::size_type)10);

  // Values preserved
  for (heap_t::size_type i = 0; i < a.size(); ++i) NOYX_ASSERT_EQ(a[i], (int)i);
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
  time_and_run(heap_array_test_smoke, "smoke");
  time_and_run(heap_array_test_self_copy_assignment, "self_copy_assignment");
  time_and_run(heap_array_test_self_move_assignment, "self_move_assignment");
  time_and_run(heap_array_test_const_access, "const_access");
  time_and_run(heap_array_test_stress_large, "stress_large");
  time_and_run(heap_array_test_multiple_reserve_preserves_data, "multiple_reserve_preserves_data");
  time_and_run(heap_array_test_resize_to_zero_and_back, "resize_to_zero_and_back");

  time_and_run(heap_array_test_stress_iterators_multithreaded, "stress_iterators_multithreaded");
  time_and_run(heap_array_test_stress_alloc_dealloc_multithreaded, "stress_alloc_dealloc_multithreaded");
  time_and_run(heap_array_test_shrink_to_fit_behavior, "shrink_to_fit_behavior");
  time_and_run(heap_array_test_stress_alloc_dealloc_singlethreaded, "stress_alloc_dealloc_singlethreaded");
  return;
}
