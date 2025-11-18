#include <tests_details.h>
#include <containers/ring_buffer.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <type_traits>

using containers::RingBuffer;

// Test 1: construct, size, data, push, operator[]
void RingBuffer_test_basic_int_behavior() {
  const size_t N = 8;
  RingBuffer<int> rb((RingBuffer<int>::size_type)N);
  NOYX_ASSERT_EQ(rb.size(), (RingBuffer<int>::size_type)N);

  // push values 0..N-1
  for (size_t i = 0; i < N; ++i) rb.push(static_cast<int>(i));

  // data() pointer check
  auto p = rb.data();
  NOYX_ASSERT_TRUE(p != nullptr);

  // After N pushes into an empty buffer starting at head=0 we expect physical layout to contain 0..N-1
  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(rb[(RingBuffer<int>::size_type)i], (int)i);
    NOYX_ASSERT_EQ(p[i], (int)i);
  }
}

// Test 2: wrap-around behavior of push (overwrite oldest entries)
void RingBuffer_test_wraparound_push() {
  const size_t N = 5;
  RingBuffer<int> rb((RingBuffer<int>::size_type)N);

  // push 0..N-1
  for (int i = 0; i < (int)N; ++i) rb.push(i);

  // push additional values to overwrite: N..2N-1
  for (int i = (int)N; i < (int)2 * (int)N; ++i) rb.push(i);

  // After 2N pushes buffer should contain last N values: N..2N-1 in physical order (assuming push writes at head index)
  for (size_t i = 0; i < N; ++i) {
    NOYX_ASSERT_EQ(rb[(RingBuffer<int>::size_type)i], (int)(N + i));
  }
}

// Test 3: copy constructor produces deep copy (modifying copy does not change original)
void RingBuffer_test_copy_constructor_deep() {
  const size_t N = 6;
  RingBuffer<int> a((RingBuffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) a.push((int)(i + 1)); // pushes 1..N

  RingBuffer<int> b(a); // copy ctor
  // modify b (via operator[])
  b[(RingBuffer<int>::size_type)0] = 9999;

  // original must remain unchanged (deep copy)
  NOYX_ASSERT_EQ(a[(RingBuffer<int>::size_type)0], 1);
  NOYX_ASSERT_EQ(b[(RingBuffer<int>::size_type)0], 9999);
}

// Test 4: copy assignment
void RingBuffer_test_copy_assignment() {
  const size_t N = 5;
  RingBuffer<int> a((RingBuffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) a.push((int)i + 10); // 10..14
  RingBuffer<int> b((RingBuffer<int>::size_type)N);
  b = a;
  // modify b
  b[(RingBuffer<int>::size_type)1] = 77;
  NOYX_ASSERT_EQ(a[(RingBuffer<int>::size_type)1], 11);
  NOYX_ASSERT_EQ(b[(RingBuffer<int>::size_type)1], 77);
}

// Test 5: move constructor and move assignment semantics (source.size() -> 0)
void RingBuffer_test_move_semantics() {
  const size_t N = 7;
  RingBuffer<int> a((RingBuffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) a.push((int)i * 2); // 0,2,4,...

  RingBuffer<int> moved(std::move(a)); // move ctor
  // source should be zeroed size (per your class design)
  NOYX_ASSERT_EQ(a.size(), (RingBuffer<int>::size_type)0);

  // moved should have original content
  for (size_t i = 0; i < N; ++i) NOYX_ASSERT_EQ(moved[(RingBuffer<int>::size_type)i], (int)i * 2);

  // move assignment
  RingBuffer<int> x((RingBuffer<int>::size_type)3);
  for (size_t i = 0; i < 3; ++i) x[(RingBuffer<int>::size_type)i] = (int)(100 + i);
  x = std::move(moved);
  NOYX_ASSERT_EQ(moved.size(), (RingBuffer<int>::size_type)0);
  NOYX_ASSERT_EQ(x.size(), (RingBuffer<int>::size_type)N);
  NOYX_ASSERT_EQ(x[(RingBuffer<int>::size_type)2], 4);
}

// Test 6: iterators and random-access operations
void RingBuffer_test_iterators() {
  const size_t N = 10;
  RingBuffer<int> a((RingBuffer<int>::size_type)N);
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
void RingBuffer_test_move_only_type() {
  using U = std::unique_ptr<int>;
  const size_t N = 3;
  RingBuffer<U> arr((RingBuffer<U>::size_type)N);

  // assign via operator[]
  arr[(RingBuffer<U>::size_type)0] = std::make_unique<int>(10);
  arr[(RingBuffer<U>::size_type)1] = std::make_unique<int>(20);
  arr[(RingBuffer<U>::size_type)2] = std::make_unique<int>(30);

  NOYX_ASSERT_TRUE(arr[(RingBuffer<U>::size_type)0] && *arr[(RingBuffer<U>::size_type)0] == 10);

  // move the whole buffer
  RingBuffer<U> moved(std::move(arr));
  NOYX_ASSERT_EQ(arr.size(), (RingBuffer<U>::size_type)0);
  NOYX_ASSERT_TRUE(moved[(RingBuffer<U>::size_type)2] && *moved[(RingBuffer<U>::size_type)2] == 30);
}

// Test 8: zero-sized buffer behaviour
void RingBuffer_test_zero_size() {
  RingBuffer<int> z((RingBuffer<int>::size_type)0);
  NOYX_ASSERT_EQ(z.size(), (RingBuffer<int>::size_type)0);
  // data() may be nullptr — only ensure no crash when obtaining iterators
  auto b = z.chead();
  auto e = z.cend();
  NOYX_ASSERT_TRUE(b == e);
}

// Test 9: basic swap via std::swap (relies on move semantics)
void RingBuffer_test_swap() {
  const size_t N = 4;
  RingBuffer<int> a((RingBuffer<int>::size_type)N);
  RingBuffer<int> b((RingBuffer<int>::size_type)N);
  for (size_t i = 0; i < N; ++i) { a.push((int)(i + 1)); b.push((int)((i + 1) * 10)); }

  // remember original values
  int a0 = a[(RingBuffer<int>::size_type)0];
  int b0 = b[(RingBuffer<int>::size_type)0];

  std::swap(a, b);

  NOYX_ASSERT_EQ(a[(RingBuffer<int>::size_type)0], b0);
  NOYX_ASSERT_EQ(b[(RingBuffer<int>::size_type)0], a0);
}

// Test 10: destructor / RAII smoke test (run many allocations)
void RingBuffer_test_stress_alloc_dealloc() {
  for (int iter = 0; iter < 20000; ++iter) {
    RingBuffer<int> a((RingBuffer<int>::size_type)(iter % 100));
    if (a.size() > 0) a.push(iter);
  }
}

// Entry point grouping
NOYX_TEST(RingBuffer_test, unit_test) {
  RingBuffer_test_basic_int_behavior();
  RingBuffer_test_wraparound_push();
  RingBuffer_test_copy_constructor_deep();
  RingBuffer_test_copy_assignment();
  RingBuffer_test_move_semantics();
  RingBuffer_test_iterators();
  RingBuffer_test_move_only_type();
  RingBuffer_test_zero_size();
  RingBuffer_test_swap();
  RingBuffer_test_stress_alloc_dealloc();
  return;
}
