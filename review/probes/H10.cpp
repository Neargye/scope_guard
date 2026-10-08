// probe: H10 a discarded make_scope_exit result is not diagnosed on MSVC in C++14 mode (NODISCARD is _Check_return_ there)
// where: msvc-x64 msvc-x86
// std: c++14 c++17 c++20
// expect: build-fail "C4834"
// expect c++14: build-ok
// meaning: c++14 build-ok confirms H10 (no warning without /analyze, so a discarded guard silently runs its action at once); c++14 build-fail "C4834" (or C6031) rejects H10. c++17/c++20 build-fail "C4834" is the control ([[nodiscard]] works); build-ok there makes the probe inconclusive.
#include <scope_guard.hpp>

int main() {
  scope_guard::make_scope_exit([]() {});
}
