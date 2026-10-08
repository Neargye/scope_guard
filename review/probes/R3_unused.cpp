// probe: R3-unused unmasking of the MSVC MAYBE_UNUSED fallback: with NEARGYE_SCOPE_GUARD_MAYBE_UNUSED defined empty (no attribute, no __pragma(warning(suppress : 4100 4101 4189))), unnamed SCOPE_* guards still build warning-free under /W4 /WX
// where: msvc-x64 msvc-x86 clangcl
// std: c++14 c++17 c++20 c++23
// expect: build-ok
// meaning: build-ok = the attribute/__pragma hides nothing on this compiler (as on GCC 13 / Clang 18, evidence O3), so the suppression is not load-bearing; build-fail = the key line names the warning (C4189/C4101/C4100 or clang -Wunused-variable) that NEARGYE_SCOPE_GUARD_MAYBE_UNUSED is needed for on that standard.

#define NEARGYE_SCOPE_GUARD_MAYBE_UNUSED
#include <scope_guard.hpp>

int main() {
  int n = 0;
  {
    SCOPE_EXIT{ ++n; };
    SCOPE_FAIL{ ++n; };
    SCOPE_SUCCESS{ ++n; };
    DEFER{ ++n; };
  }
  return n == 3 ? 0 : 1;
}
