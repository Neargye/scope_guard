// probe: H11 the pre-C++17 MSVC MAYBE_UNUSED pragma warning(suppress: 4100 4101 4189) in SCOPE_EXIT hides C4189 on the first line of the user's action body
// where: msvc-x64 msvc-x86
// std: c++14 c++17
// expect: build-fail "C4189"
// expect c++14: build-ok
// meaning: c++14 build-ok while c++17 fails with C4189 confirms H11 (the pragma leaks into the user's code); c++14 build-fail "C4189" rejects H11. c++17 is the control: [[maybe_unused]] is used there, so C4189 must be reported; build-ok on c++17 makes the probe inconclusive.
#include <scope_guard.hpp>

int main() {
  SCOPE_EXIT{
    int unused_in_action = 0;
  };
}
