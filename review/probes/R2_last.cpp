// Extra translation unit for R2.cpp (no probe tag, so the runner skips it): <scope_guard.hpp> before <cxxabi.h>.
#include <scope_guard.hpp>
#include <cxxabi.h>

int cxxabi_last_fail_count();

int cxxabi_last_fail_count() {
  int fail = 0;
  try {
    SCOPE_FAIL{ ++fail; };
    throw 1;
  } catch (int) {
  }
  {
    SCOPE_FAIL{ ++fail; };
  }
  return fail;
}
