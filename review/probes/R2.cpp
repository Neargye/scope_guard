// probe: R2 scope_guard.hpp's __cxa_get_globals forward declaration is compatible with this platform's <cxxabi.h>, in either include order (profile risk R2)
// where: macos-appleclang clang-libcxx gcc-latest gcc-m32
// std: c++11 c++14 c++17
// expect: run-ok
// with: R2_last.cpp
// meaning: build-fail ("exception specification in declaration does not match" / "different exception specifier" / "conflicting types") means the redeclaration at hpp:64-75 conflicts with the ABI header; run-fail means scope_fail/scope_success misfire in a TU that includes <cxxabi.h> first or last. c++17 is the control (the block is skipped).
#include <cxxabi.h>
#include <scope_guard.hpp>

#include <cstdio>

int cxxabi_last_fail_count();  // R2_last.cpp: <scope_guard.hpp> first, then <cxxabi.h>

int main() {
  int fail = 0;
  try {
    SCOPE_FAIL{ ++fail; };
    throw 1;
  } catch (int) {
  }
  {
    SCOPE_FAIL{ ++fail; };
  }
  const int last = cxxabi_last_fail_count();
  if (fail != 1 || last != 1) {
    std::printf("cxxabi first: fail=%d, cxxabi last: fail=%d, expected 1 each\n", fail, last);
    return 1;
  }
  std::printf("ok\n");
  return 0;
}
