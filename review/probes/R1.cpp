// probe: R1 pre-C++17 detail::uncaught_exceptions() (fixed offset sizeof(void*) into __cxa_eh_globals, D1) returns the real number of in-flight exceptions; SCOPE_FAIL/SCOPE_SUCCESS decide correctly at nesting depth 0..64, on rethrow and on std::rethrow_exception
// where: macos-appleclang clang-libcxx gcc-m32 gcc-latest
// std: c++11 c++14 c++17
// expect: run-ok
// meaning: run-ok on c++11/c++14 = the D1 offset matches this ABI library and bitness (libc++abi on macOS arm64 / Linux, libsupc++ ILP32 on gcc-m32, libsupc++ LP64 on gcc-latest as control); run-fail = "MISMATCH"/"MISFIRE" lines give the expected and the read value, i.e. the pre-C++17 path miscounts there and SCOPE_FAIL/SCOPE_SUCCESS misfire (risk R1 / D1 confirmed on that platform); c++17 uses std::uncaught_exceptions() and is the control (a failure there means the probe itself is wrong).

#include <scope_guard.hpp>

#include <cstdio>
#include <exception>
#include <stdexcept>

namespace {

int checks = 0;
int errors = 0;

void expect_count(const char* where, int depth, int expected) {
  ++checks;
  const int actual = scope_guard::detail::uncaught_exceptions();
  if (actual != expected) {
    ++errors;
    std::printf("MISMATCH %s, depth %d: expected %d uncaught, read %d\n", where, depth, expected, actual);
  }
}

void expect_runs(const char* where, int depth, int fails, int successes, int want_fails, int want_successes) {
  ++checks;
  if (fails != want_fails || successes != want_successes) {
    ++errors;
    std::printf("MISFIRE %s, depth %d: scope_fail ran %d (want %d), scope_success ran %d (want %d)\n",
                where, depth, fails, want_fails, successes, want_successes);
  }
}

// Checks the counter from a destructor that runs while `inflight` exceptions are in flight.
struct Probe {
  const char* where;
  int depth;
  int inflight;

  ~Probe() {
    expect_count(where, depth, inflight);
  }
};

void level(int depth, int limit);

// Destroyed during unwinding while `depth` exceptions are in flight; opens the next level there.
struct Nest {
  int depth;
  int limit;

  ~Nest() {
    expect_count("destructor during unwinding", depth, depth);
    int fails = 0;
    int successes = 0;
    {
      // No new exception leaves this scope: scope_success must run, scope_fail must not.
      SCOPE_FAIL{ ++fails; };
      SCOPE_SUCCESS{ ++successes; };
    }
    expect_runs("quiet scope inside unwinding", depth, fails, successes, 0, 1);
    if (depth < limit) {
      level(depth, limit);
    }
  }
};

// Runs while `depth` exceptions are already in flight.
void level(int depth, int limit) {
  int fails = 0;
  int successes = 0;
  try {
    SCOPE_FAIL{ ++fails; };
    SCOPE_SUCCESS{ ++successes; };
    Nest nest{depth + 1, limit};
    Probe probe{"probe during throw", depth, depth + 1};
    (void)nest;
    (void)probe;
    throw std::runtime_error{"level"};
  } catch (const std::runtime_error&) {
    expect_count("catch handler", depth, depth);
    try {
      Probe probe{"probe during rethrow", depth, depth + 1};
      (void)probe;
      throw;
    } catch (const std::runtime_error&) {
      expect_count("rethrown exception caught", depth, depth);
    }
  }
  expect_runs("throwing scope", depth, fails, successes, 1, 0);

  std::exception_ptr ptr = std::make_exception_ptr(std::logic_error{"ptr"});
  expect_count("after make_exception_ptr", depth, depth);
  try {
    Probe probe{"probe during rethrow_exception", depth, depth + 1};
    (void)probe;
    std::rethrow_exception(ptr);
  } catch (const std::logic_error&) {
    expect_count("rethrow_exception caught", depth, depth);
  }
}

} // namespace

int main() {
  expect_count("no exception", 0, 0);
  level(0, 64);
  expect_count("after all levels", 0, 0);

  int fails = 0;
  int successes = 0;
  try {
    [&]() {
      SCOPE_FAIL{ ++fails; };
      SCOPE_SUCCESS{ ++successes; };
      throw 1;
    }();
  } catch (int) {
  }
  expect_runs("exception escaping a lambda", 0, fails, successes, 1, 0);

#if !defined(_MSC_VER) && (defined(__clang__) || defined(__GNUC__)) && __cplusplus < 201703L
  const char* path = "custom __cxa_get_globals() + sizeof(void*)";
#else
  const char* path = "std::uncaught_exceptions()";
#endif
  std::printf("R1 %s: %d checks, %d errors; path: %s; sizeof(void*) = %u\n",
              errors == 0 ? "ok" : "FAILED", checks, errors, path, static_cast<unsigned>(sizeof(void*)));
  return errors == 0 ? 0 : 1;
}
