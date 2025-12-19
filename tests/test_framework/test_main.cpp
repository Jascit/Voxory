#include <tests_details.h>
#include <chrono>
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    pragma message("ASAN: __has_feature(address_sanitizer) == 1")
#  else
#    pragma message("ASAN: __has_feature(address_sanitizer) == 0")
#  endif
#elif defined(__SANITIZE_ADDRESS__)
#  pragma message("ASAN: __SANITIZE_ADDRESS__ defined")
#else
#  pragma message("ASAN: NOT enabled")
#endif

int main() {
  auto& registry = TestRegistry::instance().getRegistry();

  std::cout << "Running " << registry.size() << " tests:\n";
  
  for (auto& info : registry) {
    std::cout << info.suiteName << "." << info.testName << " ... ";
    auto start = std::chrono::steady_clock::now();
    info.testFunc();
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (!(info.flag & FAILED)) {
      std::cout << "OK (" << ms << " ms)\n";
      TestingSystem::instance()->success();
    }
    else {
      std::cout << "FAILED\n";
    }
    ++TestRegistry::instance();
  }

  return (TestingSystem::instance()->GetFailedCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}