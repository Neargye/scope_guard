// probe: R7 clang-cl (both _MSC_VER and __clang__) takes the __clang__ NODISCARD/MAYBE_UNUSED branches and the std::uncaught_exceptions path; all public macros compile at /W4 /WX and SCOPE_FAIL/SCOPE_SUCCESS work (profile risk R7)
// where: clangcl
// std: c++14 c++17 c++20
// expect: run-ok
// meaning: build-fail on c++14 only means the __attribute__ fallbacks or the C++14 std::uncaught_exceptions() call do not work with clang-cl's __cplusplus/_MSVC_LANG; build-fail "-Wunused" means MAYBE_UNUSED is ineffective; run-fail means wrong scope_fail/scope_success. The run prints __cplusplus for the report.
#if !defined(__clang__) || !defined(_MSC_VER)
#  error R7 must be built with clang-cl
#endif
#include <scope_guard.hpp>

#include <cstdio>
#include <utility>

static int failures = 0;

// Throws only conditionally, so MSVC flow analysis does not report C4702 for code after a call.
static void throw_if_nonzero(int value) {
  if (value != 0) {
    throw value;
  }
}

static void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("FAILED: %s\n", what);
    ++failures;
  }
}

// Guards created in a destructor that runs while one exception is in flight.
struct during_unwind {
  int* fail;
  int* success;
  ~during_unwind() {
    try {
      SCOPE_FAIL{ ++*fail; };
      SCOPE_SUCCESS{ ++*success; };
      throw_if_nonzero(2);
    } catch (int) {
    }
    {
      SCOPE_FAIL{ ++*fail; };
      SCOPE_SUCCESS{ ++*success; };
    }
  }
};

int main() {
  int exit_count = 0;
  int fail_count = 0;
  int success_count = 0;
  {
    SCOPE_EXIT{ ++exit_count; };
    DEFER{ ++exit_count; };
    SCOPE_FAIL{ ++fail_count; };
    SCOPE_SUCCESS{ ++success_count; };
  }
  expect(exit_count == 2 && fail_count == 0 && success_count == 1, "normal scope exit");

  try {
    SCOPE_EXIT{ ++exit_count; };
    SCOPE_FAIL{ ++fail_count; };
    SCOPE_SUCCESS{ ++success_count; };
    throw_if_nonzero(1);
  } catch (int) {
  }
  expect(exit_count == 3 && fail_count == 1 && success_count == 1, "scope exit by exception");

  {
    MAKE_SCOPE_EXIT(dismissed){ ++exit_count; };
    dismissed.dismiss();
    MAKE_DEFER(kept){ ++exit_count; };
    auto moved = std::move(kept);
    (void)moved;
  }
  expect(exit_count == 4, "dismiss and move");

  WITH_SCOPE_EXIT({ ++exit_count; }) {
    ++exit_count;
  }
  WITH_DEFER({ ++exit_count; }) {
  }
  WITH_SCOPE_SUCCESS({ ++success_count; }) {
  }
  try {
    WITH_SCOPE_FAIL({ ++fail_count; }) {
      throw_if_nonzero(1);
    }
  } catch (int) {
  }
  expect(exit_count == 7 && fail_count == 2 && success_count == 2, "WITH_* macros");

  int nested_fail = 0;
  int nested_success = 0;
  try {
    during_unwind d = {&nested_fail, &nested_success};
    (void)d;
    throw_if_nonzero(1);
  } catch (int) {
  }
  expect(nested_fail == 1 && nested_success == 1, "guards in a destructor during unwinding");

  std::printf("__cplusplus=%ld %s\n", static_cast<long>(__cplusplus), failures == 0 ? "ok" : "failed");
  return failures == 0 ? 0 : 1;
}
