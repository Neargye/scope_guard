// probe: R3-nodiscard-clangcl NEARGYE_SCOPE_GUARD_NODISCARD on ClangCL (__clang__ and _MSC_VER both defined): the __clang__ branch is taken, so discarding make_scope_exit() is diagnosed in every standard ([[nodiscard]] from C++17, __attribute__((__warn_unused_result__)) before)
// where: clangcl
// std: c++14 c++17 c++20 c++23
// expect: build-fail "ignoring return value"
// meaning: build-fail "ignoring return value" = the clang attribute branch works under clang-cl; build-ok = the discarded guard is not diagnosed on ClangCL for that standard (R3 holds: wrong branch or attribute ignored).

#include <scope_guard.hpp>

int main() {
  scope_guard::make_scope_exit([]() {});
  return 0;
}
