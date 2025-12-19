#pragma once

#if defined(_WIN64)
  // Windows 64-bit
#include <windows.h>
#define WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
// iOS Simulator
#elif TARGET_OS_IPHONE
// iOS device
#elif TARGET_OS_MAC
#define macOS
#else
// other Apple platform
#endif

#elif defined(__linux__)
#define LINUX
#else
#error "Unknown platform"
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#define X64
#elif defined(__aarch64__) || defined(_M_ARM64)
#define X64_ARM
#else
#error "unsupported architecture"
#endif
#pragma once

#include <cstdlib>
#include <cstdio>

#define LOCATION_FILE_LINE __FILE__, __LINE__

#if defined(_MSC_VER) && defined(_DEBUG)
#include <intrin.h>
#define DEBUG_BREAK() __debugbreak()
#elif defined(_DEBUG) || defined(DEBUG)
#include <csignal>
#define DEBUG_BREAK() std::raise(SIGTRAP)
#else
#define DEBUG_BREAK() ((void)0)
#endif


#if defined(_MSC_VER)
#include <csignal>
#define ABORT_IMMEDIATELY() __fastfail(FAST_FAIL_INVALID_ARG)
#elif defined(__linux__) && !defined(__ANDROID__)
#include <unistd.h>
#include <sys/syscall.h>
static inline void linux_abort_with_core() {
  pid_t pid = getpid();
  pid_t tid = (pid_t)syscall(SYS_gettid);
  syscall(SYS_tgkill, pid, tid, SIGABRT);
  std::abort();
}
#define ABORT_IMMEDIATELY() linux_abort_with_core()
#elif defined(__ANDROID__)
#include <csignal>
#define ABORT_IMMEDIATELY() std::raise(SIGABRT)
#else
#include <csignal>
#define ABORT_IMMEDIATELY() std::abort()
#endif


#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>

#define report_debug(level, file, line, fmt, ...) \
      _CrtDbgReport(level, file, line, fmt, __VA_ARGS__)
#else
#include <cstdio>
#define report_debug(level, file, line, fmt, ...) do { \
      std::fprintf(stderr, "%s(%d): " fmt "\n", file, line, ##__VA_ARGS__); \
  } while (0)
#endif


#define ASSERT_ABORT(cond, msg) do {                                    \
    if (!(cond)) {                                                      \
        report_debug(2, LOCATION_FILE_LINE, "%s", (msg));               \
        DEBUG_BREAK();                                                  \
        ABORT_IMMEDIATELY();                                            \
    }                                                                   \
} while (0)

#if defined(_MSC_VER)
#  define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define FORCE_INLINE inline __attribute__((always_inline))
#else
#  define FORCE_INLINE inline
#endif

#define CONSTEXPR constexpr
#define NODISCARD [[nodiscard]]

#if defined(FORCE_DISABLE_DEBUG)
#  if defined(DEBUG)           // скасувати локальні дефайни
#    undef DEBUG
#  endif
#  if defined(DEBUG_ITERATOR)
#    undef DEBUG_ITERATOR
#  endif
#elif defined(FORCE_ENABLE_DEBUG)
#  if !defined(DEBUG)
#    define DEBUG
#  endif
#  if !defined(DEBUG_ITERATOR)
#    define DEBUG_ITERATOR
#  endif
#endif

#if !defined(NDEBUG) && !defined(DEBUG) && !defined(FORCE_DISABLE_DEBUG)
#  define DEBUG
#endif

#if defined(_MSC_VER)
#  if defined(_DEBUG) && !defined(FORCE_DISABLE_DEBUG)
#    if !defined(DEBUG)
#      define DEBUG
#    endif
#    if !defined(DEBUG_ITERATORS)
#      define DEBUG_ITERATORS
#    endif
#  endif
#endif // _MSC_VER

#if defined(__GLIBCXX__) || defined(__GLIBCXX_DEBUG)
#  if defined(_GLIBCXX_DEBUG) && !defined(FORCE_DISABLE_DEBUG)
#    if !defined(DEBUG)
#      define DEBUG
#    endif
#    if !defined(DEBUG_ITERATOR)
#      define DEBUG_ITERATOR
#    endif
#  endif
#endif

#if defined(_LIBCPP_VERSION) || defined(__LIBCPP__)

#  if defined(_LIBCPP_DEBUG) && (_LIBCPP_DEBUG > 0) && !defined(FORCE_DISABLE_DEBUG)
#    if !defined(DEBUG)
#      define DEBUG
#    endif
#    if !defined(DEBUG_ITERATOR)
#      define DEBUG_ITERATOR
#    endif
#  endif
#endif

#if defined(DEBUG_ITERATOR)
#  pragma message("DEBUG_ITERATOR is ON (iterator debug checks enabled). Performance may be affected.")
#endif

#if defined(DEBUG)
#  pragma message("DEBUG is ON")
#endif

#ifdef X64
  #ifndef mm_pause
    #define mm_pause() _mm_pause()
  #endif // !mm_pause()
#elif defined(X64_ARM)
  #ifndef mm_pause
    #define mm_pause() _mm_pause()
  #endif // !mm_pause()
#endif // def X64

