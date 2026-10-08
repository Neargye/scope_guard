// Second translation unit of the R2 probe (compiled together with R2.cpp): <cxxabi.h> after scope_guard.hpp.

#include <scope_guard.hpp>
#include <cxxabi.h>

#include <cstdio>
#include <stdexcept>

namespace {

struct InFlight {
  int* seen;

  ~InFlight() {
    *seen = scope_guard::detail::uncaught_exceptions();
  }
};

} // namespace

int r2_last();

int r2_last() {
  int fails = 0;
  int successes = 0;
  int seen = -1;
  try {
    SCOPE_FAIL{ ++fails; };
    SCOPE_SUCCESS{ ++successes; };
    InFlight in_flight{&seen};
    (void)in_flight;
    throw std::runtime_error{"last"};
  } catch (const std::runtime_error&) {
  }
  const bool ok = fails == 1 && successes == 0 && seen == 1 && scope_guard::detail::uncaught_exceptions() == 0 &&
                  abi::__cxa_get_globals() != nullptr;
  std::printf("cxxabi last: fails=%d successes=%d in-flight=%d -> %s\n", fails, successes, seen, ok ? "ok" : "WRONG");
  return ok ? 0 : 1;
}
