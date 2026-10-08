// probe: H1 pre-C++17 detail::uncaught_exceptions() equals the real count and drives SCOPE_FAIL/SCOPE_SUCCESS (profile R1)
// where: macos-appleclang clang-libcxx gcc-m32 gcc-latest
// std: c++11 c++14
// expect: run-ok
// meaning: run-ok means the fixed offset sizeof(void*) reads __cxa_eh_globals::uncaughtExceptions on this ABI library; run-fail ("expected N, got M" or "fail=/success=") confirms H1 for this job.
#include <scope_guard.hpp>

#include <cstdio>

static int failures = 0;

struct check_on_unwind {
  int expected;
  ~check_on_unwind() {
    const int actual = scope_guard::detail::uncaught_exceptions();
    if (actual != expected) {
      std::printf("expected %d, got %d\n", expected, actual);
      ++failures;
    }
  }
};

struct nested_thrower {
  ~nested_thrower() {
    try {
      check_on_unwind check = {2};
      throw 2;
    } catch (int) {
    }
  }
};

int main() {
  {
    check_on_unwind check = {0};
  }
  try {
    check_on_unwind check = {1};
    throw 1;
  } catch (int) {
  }
  try {
    nested_thrower thrower;
    throw 1;
  } catch (int) {
  }

  int fail = 0;
  int success = 0;
  try {
    SCOPE_FAIL{ ++fail; };
    SCOPE_SUCCESS{ ++success; };
    throw 1;
  } catch (int) {
  }
  {
    SCOPE_FAIL{ ++fail; };
    SCOPE_SUCCESS{ ++success; };
  }
  if (fail != 1 || success != 1) {
    std::printf("fail=%d success=%d\n", fail, success);
    ++failures;
  }

  std::printf(failures == 0 ? "ok\n" : "failed\n");
  return failures == 0 ? 0 : 1;
}
