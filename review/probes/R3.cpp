// probe: R3 every public macro (SCOPE_*, DEFER, MAKE_*, WITH_*) builds warning-free with the MSVC/ClangCL attribute branches of NEARGYE_SCOPE_GUARD_NODISCARD/MAYBE_UNUSED (MSVC C++14: _Check_return_ and __pragma(warning(suppress ...)); ClangCL: the __clang__ branch with _MSC_VER defined) and the guards run correctly
// where: msvc-x64 msvc-x86 clangcl
// std: c++14 c++17 c++20 c++23
// expect: run-ok
// meaning: run-ok = the MSVC/ClangCL attribute branches compile cleanly under /W4 /WX /permissive- and the macros behave as documented; build-fail = one of these branches is ill-formed or warns there (the key line names the warning, e.g. C4189/C4101/C4100 or a clang -W flag), i.e. R3 holds; run-fail = wrong execution counts are printed.

#include <scope_guard.hpp>

#include <cstdio>
#include <stdexcept>

namespace {

struct Counts {
  int exits;
  int fails;
  int successes;
  int defers;
  int named;
  int with;
};

Counts run(bool fail) {
  Counts c = {0, 0, 0, 0, 0, 0};
  try {
    SCOPE_EXIT{ ++c.exits; };
    SCOPE_FAIL{ ++c.fails; };
    SCOPE_SUCCESS{ ++c.successes; };
    DEFER{ ++c.defers; };
    MAKE_SCOPE_EXIT(named_exit){ ++c.named; };
    MAKE_SCOPE_FAIL(named_fail){ ++c.named; };
    MAKE_SCOPE_SUCCESS(named_success){ ++c.named; };
    MAKE_DEFER(named_defer){ ++c.named; };
    named_defer.dismiss();
    WITH_SCOPE_EXIT({ ++c.with; }) {
      ++c.with;
    }
    WITH_SCOPE_FAIL({ ++c.with; }) {}
    WITH_SCOPE_SUCCESS({ ++c.with; }) {}
    WITH_DEFER({ ++c.with; }) {}
    if (fail) {
      throw std::runtime_error{"fail"};
    }
  } catch (const std::runtime_error&) {
  }
  return c;
}

bool same(const Counts& a, const Counts& b) {
  return a.exits == b.exits && a.fails == b.fails && a.successes == b.successes && a.defers == b.defers &&
         a.named == b.named && a.with == b.with;
}

} // namespace

int main(int argc, char**) {
  // argc keeps the throw path opaque to the optimizer (no C4702 "unreachable code").
  const bool quiet = argc > 100;
  const Counts ok = run(quiet);
  const Counts failed = run(!quiet);
  // named: exit and success (or exit and fail) run, the dismissed MAKE_DEFER does not; with: 2 + fail/success + defer.
  const Counts want_ok = {1, 0, 1, 1, 2, 4};
  const Counts want_failed = {1, 1, 0, 1, 2, 4};
  const bool good = same(ok, want_ok) && same(failed, want_failed);
  std::printf("R3 %s: ok-run exits=%d fails=%d successes=%d defers=%d named=%d with=%d; fail-run exits=%d fails=%d successes=%d defers=%d named=%d with=%d\n",
              good ? "ok" : "FAILED", ok.exits, ok.fails, ok.successes, ok.defers, ok.named, ok.with,
              failed.exits, failed.fails, failed.successes, failed.defers, failed.named, failed.with);
  return good ? 0 : 1;
}
