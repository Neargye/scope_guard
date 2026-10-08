// probe: R1 pre-C++17 uncaught_exceptions() via __cxa_get_globals tracks nested in-flight exceptions; SCOPE_FAIL/SCOPE_SUCCESS fire correctly at every depth (profile risk R1)
// where: macos-appleclang clang-libcxx gcc-latest gcc-m32
// std: c++11 c++14
// expect: run-ok
// meaning: run-fail means the fixed offset reads the wrong field of __cxa_eh_globals for this ABI library (wrong count, or disagreement with std::uncaught_exception()), so scope_fail/scope_success misfire in C++11/14 builds.
#include <scope_guard.hpp>

#include <cstdio>
#include <exception>

static int failures = 0;

static void check(int expected, const char* where) {
  const int actual = scope_guard::detail::uncaught_exceptions();
  const bool any = std::uncaught_exception();
  if (actual != expected || any != (expected > 0)) {
    std::printf("%s: expected %d, got %d (std::uncaught_exception()=%d)\n", where, expected, actual, any ? 1 : 0);
    ++failures;
  }
}

// Throws during unwinding while `depth` exceptions are already in flight, down to depth `max`.
struct nested {
  int depth;
  int max;
  int* fail_runs;
  int* success_runs;
  ~nested() {
    check(depth, "nested destructor");
    if (depth >= max) {
      return;
    }
    try {
      SCOPE_FAIL{ ++*fail_runs; };
      SCOPE_SUCCESS{ ++*success_runs; };
      nested inner = {depth + 1, max, fail_runs, success_runs};
      throw depth;
    } catch (int) {
    }
    check(depth, "after inner catch");
    {
      // Created and destroyed while `depth` exceptions are in flight, with no new exception: success, not fail.
      SCOPE_FAIL{ ++*fail_runs; };
      SCOPE_SUCCESS{ ++*success_runs; };
    }
  }
};

int main() {
  check(0, "main start");

  const int max = 4;
  int fail_runs = 0;
  int success_runs = 0;
  try {
    nested outer = {1, max, &fail_runs, &success_runs};
    throw 0;
  } catch (int) {
  }
  check(0, "main after catch");
  // Depths 1..max-1 each run one throwing scope (fail) and one quiet scope (success).
  if (fail_runs != max - 1 || success_runs != max - 1) {
    std::printf("fail_runs=%d success_runs=%d, expected %d each\n", fail_runs, success_runs, max - 1);
    ++failures;
  }

  // A guard moved out of a function keeps its baseline (0) and sees the exception thrown later.
  int moved_fail = 0;
  try {
    auto guard = scope_guard::make_scope_fail([&moved_fail]() { ++moved_fail; });
    auto moved = std::move(guard);
    throw 1;
  } catch (int) {
  }
  if (moved_fail != 1) {
    std::printf("moved scope_fail runs=%d, expected 1\n", moved_fail);
    ++failures;
  }

  std::printf(failures == 0 ? "ok\n" : "failed\n");
  return failures == 0 ? 0 : 1;
}
