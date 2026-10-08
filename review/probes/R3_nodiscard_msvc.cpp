// probe: R3-nodiscard-msvc NEARGYE_SCOPE_GUARD_NODISCARD on MSVC: discarding make_scope_exit() is diagnosed with [[nodiscard]] (C++17 and later); in C++14 the _Check_return_ fallback is expected to be silent without /analyze
// where: msvc-x64 msvc-x86
// std: c++14 c++17 c++20 c++23
// expect: build-fail "C4834"
// expect c++14: build-ok
// meaning: c++17+ build-fail "C4834" = [[nodiscard]] is active on MSVC as intended; c++17+ build-ok = the MSVC NODISCARD branch is not taken (e.g. _MSVC_LANG/_MSC_VER condition wrong) and a discarded guard is not diagnosed; c++14 build-ok = _Check_return_ has no effect outside /analyze (known limitation of the fallback), c++14 build-fail = the fallback does diagnose.

#include <scope_guard.hpp>

int main() {
  scope_guard::make_scope_exit([]() {});
  return 0;
}
