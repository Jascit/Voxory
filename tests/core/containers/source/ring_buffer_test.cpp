#include <tests_details.h>
#include <containers/ring_buffer.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <type_traits>
#include <iomanip>

using containers::ring_buffer;

// Test 1: construct, size, data, push, operator[]
void ring_buffer_test_basic_int_behavior() {
  const size_t N = 8;
  ring_buffer<int> rb((ring_buffer<int>::size_type)N);

  // IMPORTANT: constructor with capacity sets size() == 0 (capacity = N, but no elements constructed as "alive")
  NOYX_ASSERT_EQ(rb.size(), (ring_buffer<int>::size_type)0);

  // push values 0..N-1
  for (size_t i = 0; i < N; ++i) rb.push(static_cast<int>(i));

  // now size should be N
  NOYX_ASSERT_EQ(rb.size(), (ring_buffer<int>::size_type)N);

  // data() pointer check
  auto p = rb.data();
  NOYX_ASSERT_TRUE(p != nullptr || N == 0);

  // After N pushes into an empty buffer starting at head=0 we expect physical layout to contain 0..N-1
  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(rb[(ring_buffer<int>::size_type)i], (int)i);
    if (p) NOYX_ASSERT_EQ(p[i], (int)i);
  }
}

// Test 2: wrap-around behavior of push (overwrite oldest entries)
void ring_buffer_test_wraparound_push() {
  const size_t N = 5;
  ring_buffer<int> rb((ring_buffer<int>::size_type)N);

  // push 0..N-1
  for (int i = 0; i < (int)N; ++i) rb.push(i);

  // push additional values to overwrite: N..2N-1
  for (int i = (int)N; i < (int)2 * (int)N; ++i) rb.push(i);

  // After 2N pushes buffer should contain last N values: N..2N-1 in logical order
  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(rb[(ring_buffer<int>::size_type)i], (int)(N + i));
  }
}

// Test 3: copy constructor produces deep copy (modifying copy does not change original)
void ring_buffer_test_copy_constructor_deep() {
  const size_t N = 6;
  ring_buffer<int> a((ring_buffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) a.push((int)(i + 1)); // pushes 1..N

  ring_buffer<int> b(a); // copy ctor
  // modify b (via operator[])
  b[(ring_buffer<int>::size_type)0] = 9999;

  // original must remain unchanged (deep copy)
  NOYX_ASSERT_EQ(a[(ring_buffer<int>::size_type)0], 1);
  NOYX_ASSERT_EQ(b[(ring_buffer<int>::size_type)0], 9999);
}

// Test 4: copy assignment
void ring_buffer_test_copy_assignment() {
  const size_t N = 5;
  ring_buffer<int> a((ring_buffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) a.push((ring_buffer<int>::value_type)(i + 10)); // 10..14
  ring_buffer<int> b((ring_buffer<int>::size_type)N);
  b = a;
  // modify b
  b[(ring_buffer<int>::size_type)1] = 77;
  NOYX_ASSERT_EQ(a[(ring_buffer<int>::size_type)1], 11);
  NOYX_ASSERT_EQ(b[(ring_buffer<int>::size_type)1], 77);
}

// Test 5: move constructor and move assignment semantics (source.size() -> 0)
void ring_buffer_test_move_semantics() {
  const size_t N = 7;
  ring_buffer<int> a((ring_buffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) a.push((int)i * 2); // 0,2,4,...

  ring_buffer<int> moved(std::move(a)); // move ctor
  // source should be zeroed size (per your class design)
  NOYX_ASSERT_EQ(a.size(), (ring_buffer<int>::size_type)0);

  // moved should have original content
  for (size_t i = 0; i < N; ++i) NOYX_ASSERT_EQ(moved[(ring_buffer<int>::size_type)i], (int)i * 2);

  // move assignment
  ring_buffer<int> x((ring_buffer<int>::size_type)3);
  for (size_t i = 0; i < 3; ++i) x[(ring_buffer<int>::size_type)i] = (int)(100 + i);
  x = std::move(moved);
  NOYX_ASSERT_EQ(moved.size(), (ring_buffer<int>::size_type)0);
  NOYX_ASSERT_EQ(x.size(), (ring_buffer<int>::size_type)N);
  NOYX_ASSERT_EQ(x[(ring_buffer<int>::size_type)2], 4);
}

// Test 6: iterators and random-access operations
void ring_buffer_test_iterators() {
  const size_t N = 10;
  ring_buffer<int> a((ring_buffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) a.push(int(i + 1)); // 1..10

  // iterate from chead() to cend() — we expect to see N elements
  size_t idx = 0;
  for (auto it = a.chead(); it != a.cend(); ++it) {
    (void)*it; // dereference to ensure it works
    ++idx;
    if (idx > N + 5) { // defensive: avoid infinite loop if iterators are broken
      NOYX_FAIL_MESSAGE("Iterator loop exceeded expected bounds");
      break;
    }
  }
  NOYX_ASSERT_EQ((int)idx, (int)N);

  // explicit iterator arithmetic test (random access)
  auto it = a.chead();
  it += 5;
  NOYX_ASSERT_EQ(*it, 6);
  it -= 3;
  NOYX_ASSERT_EQ(*it, 3);

  // difference
  auto it2 = a.chead() + 7;
  NOYX_ASSERT_EQ((int)(it2 - a.chead()), 7);
}

// Test 7: storage of move-only types (unique_ptr)
void ring_buffer_test_move_only_type() {
  using U = std::unique_ptr<int>;
  const size_t N = 3;
  ring_buffer<U> arr((ring_buffer<U>::size_type)N);

  // assign via operator[] (we rely on default-constructed slots)
  arr[(ring_buffer<U>::size_type)0] = std::make_unique<int>(10);
  arr[(ring_buffer<U>::size_type)1] = std::make_unique<int>(20);
  arr[(ring_buffer<U>::size_type)2] = std::make_unique<int>(30);

  NOYX_ASSERT_TRUE(arr[(ring_buffer<U>::size_type)0] && *arr[(ring_buffer<U>::size_type)0] == 10);

  // move the whole buffer
  ring_buffer<U> moved(std::move(arr));
  NOYX_ASSERT_EQ(arr.size(), (ring_buffer<U>::size_type)0);
  NOYX_ASSERT_TRUE(moved[(ring_buffer<U>::size_type)2] && *moved[(ring_buffer<U>::size_type)2] == 30);
}

// Test 8: zero-sized buffer behaviour
void ring_buffer_test_zero_size() {
  ring_buffer<int> z((ring_buffer<int>::size_type)0);
  NOYX_ASSERT_EQ(z.size(), (ring_buffer<int>::size_type)0);
  // data() may be nullptr — only ensure no crash when obtaining iterators
  auto b = z.chead();
  auto e = z.cend();
  NOYX_ASSERT_TRUE(b == e);
}

// Test 9: basic swap via std::swap (relies on move semantics)
void ring_buffer_test_swap() {
  const size_t N = 4;
  ring_buffer<int> a((ring_buffer<int>::size_type)N);
  ring_buffer<int> b((ring_buffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) { a.push((int)(i + 1)); b.push((int)((i + 1) * 10)); }

  // remember original values
  int a0 = a[(ring_buffer<int>::size_type)0];
  int b0 = b[(ring_buffer<int>::size_type)0];

  std::swap(a, b);

  NOYX_ASSERT_EQ(a[(ring_buffer<int>::size_type)0], b0);
  NOYX_ASSERT_EQ(b[(ring_buffer<int>::size_type)0], a0);
}

// Test 10: destructor / RAII smoke test (run many allocations)
void ring_buffer_test_stress_alloc_dealloc() {
  for (int iter = 0; iter < 20000; ++iter) {
    ring_buffer<int> a((ring_buffer<int>::size_type)(iter % 100));
    if (a.size() > 0) a.push(iter);
  }
}

// Entry point grouping
NOYX_TEST(ring_buffer_test, unit_test) {
  using clk = std::chrono::steady_clock;
  using rep = std::chrono::duration<double, std::milli>; // milliseconds as double

  auto time_and_run = [&](auto&& fn, const char* name) {
    auto s = clk::now();
    fn();
    auto e = clk::now();
    rep d = e - s;
    std::cout.setf(std::ios::fixed); std::cout << std::setprecision(3);
    std::cout << "[TIMING] " << name << " : " << d.count() << " ms\n";
    std::cout.unsetf(std::ios::fixed);
    };

  time_and_run(ring_buffer_test_basic_int_behavior, "basic_int_behavior");
  time_and_run(ring_buffer_test_wraparound_push, "wraparound_push");
  time_and_run(ring_buffer_test_copy_constructor_deep, "copy_constructor_deep");
  time_and_run(ring_buffer_test_copy_assignment, "copy_assignment");
  time_and_run(ring_buffer_test_move_semantics, "move_semantics");
  time_and_run(ring_buffer_test_iterators, "iterators");
  time_and_run(ring_buffer_test_move_only_type, "move_only_type");
  time_and_run(ring_buffer_test_zero_size, "zero_size");
  time_and_run(ring_buffer_test_swap, "swap");
  time_and_run(ring_buffer_test_stress_alloc_dealloc, "stress_alloc_dealloc");

  return;
}