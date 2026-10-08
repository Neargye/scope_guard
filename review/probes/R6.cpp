// probe: R6 all public macros compile cleanly at /W4 /WX on MSVC, including the pre-C++17 __pragma(warning(suppress ...)) MAYBE_UNUSED and _Check_return_ NODISCARD branches, and SCOPE_FAIL/SCOPE_SUCCESS work on x86 and x64 (profile risk R6)
// where: msvc-x64 msvc-x86
// std: c++14 c++17 c++20
// expect: run-ok
// meaning: build-fail on c++14 only points at the MSVC pre-C++17 branches (hpp:109-116, 326-331): C4189/C4101 means the pragma does not cover the guard, a syntax error means __pragma before "const auto" is misplaced; C4702 means the /wd4702 in test and example CMake hides a warning users get. run-fail means wrong scope_fail/scope_success on this target (x86 EH differs from x64).
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
