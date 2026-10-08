// probe: R2 the header's own __cxxabiv1::__cxa_get_globals declaration (emitted for libc++abi via _LIBCPPABI_VERSION, OpenBSD, QNX and, per H4, every Clang) coexists with <cxxabi.h> in both include orders (<cxxabi.h> first: this TU, last: R2_last.cpp) and the pre-C++17 counter works in both TUs
// where: macos-appleclang clang-libcxx gcc-latest gcc-m32
// std: c++11 c++14
// expect: run-ok
// with: R2_last.cpp
// meaning: build-fail with "exception specification" / "conflicting" / "redeclar" = the header's declaration clashes with this ABI library's <cxxabi.h> (risk R2 confirmed there); run-fail = it builds but SCOPE_FAIL/SCOPE_SUCCESS or the counter misbehave in one include order; run-ok = no R2 problem on that platform. The printed "own declaration" field says whether the header's declaration was compiled (expected yes on libc++abi and on any Clang, no on GCC + libsupc++).

#include <cxxabi.h>
#include <scope_guard.hpp>

#include <cstdio>
#include <stdexcept>

#if !defined(__FreeBSD__) && (defined(_LIBCPPABI_VERSION) || defined(__OpenBSD__) || \
    (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407) || \
    (defined(__QNXNTO__) && !defined(__GLIBCXX__) && !defined(__GLIBCPP__)))
#  define R2_OWN_DECLARATION "yes"
#else
#  define R2_OWN_DECLARATION "no"
#endif

int r2_last();

namespace {

struct InFlight {
  int* seen;

  ~InFlight() {
    *seen = scope_guard::detail::uncaught_exceptions();
  }
};

int r2_first() {
  int fails = 0;
  int successes = 0;
  int seen = -1;
  try {
    SCOPE_FAIL{ ++fails; };
    SCOPE_SUCCESS{ ++successes; };
    InFlight in_flight{&seen};
    (void)in_flight;
    throw std::runtime_error{"first"};
  } catch (const std::runtime_error&) {
  }
  const bool ok = fails == 1 && successes == 0 && seen == 1 && scope_guard::detail::uncaught_exceptions() == 0 &&
                  abi::__cxa_get_globals() != nullptr;
  std::printf("cxxabi first: fails=%d successes=%d in-flight=%d -> %s\n", fails, successes, seen, ok ? "ok" : "WRONG");
  return ok ? 0 : 1;
}

} // namespace

int main() {
  const int errors = r2_first() + r2_last();
  std::printf("R2 %s; own declaration: %s; _LIBCPPABI_VERSION: %s\n", errors == 0 ? "ok" : "FAILED", R2_OWN_DECLARATION,
#if defined(_LIBCPPABI_VERSION)
              "defined"
#else
              "not defined"
#endif
  );
  return errors == 0 ? 0 : 1;
}
