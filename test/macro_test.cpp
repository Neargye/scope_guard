// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// Plain program without a test framework: guard macros are expanded as ordinary statements.
// CMake also builds it with exceptions disabled.
#include <scope_guard.hpp>

struct NoThrowAction {
  void operator() () noexcept {}
};

static_assert(std::is_nothrow_destructible<decltype(scope_guard::make_scope_exit(NoThrowAction{}))>::value,
              "A nothrow action must produce a nothrow guard under every policy.");

#if !defined(SCOPE_GUARD_NO_THROW_ACTION)
struct PotentiallyThrowingAction {
  void operator() () {}
};
#  if defined(SCOPE_GUARD_SUPPRESS_THROW_ACTION) && (defined(__cpp_exceptions) || defined(_CPPUNWIND))
static_assert(std::is_nothrow_destructible<decltype(scope_guard::make_scope_exit(PotentiallyThrowingAction{}))>::value,
              "SUPPRESS with exceptions must produce a nothrow guard.");
#  else
static_assert(!std::is_nothrow_destructible<decltype(scope_guard::make_scope_exit(PotentiallyThrowingAction{}))>::value,
              "Without an active SUPPRESS catch, guard noexcept follows the action.");
#  endif
#endif

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
#  include <stdexcept>
#endif

int main() {
  int order = 0;
  {
    SCOPE_EXIT{ order = order * 10 + 1; }; DEFER{ order = order * 10 + 2; };
    SCOPE_SUCCESS{ order = order * 10 + 3; }; SCOPE_FAIL{ order = -1; };
    WITH_SCOPE_EXIT({ order = order * 10 + 4; }) { WITH_DEFER({ order = order * 10 + 5; }) {} }
  }
  if (order != 54321) {
    return 1;
  }

  int successes = 0;
  WITH_SCOPE_SUCCESS({ ++successes; }) {}
  WITH_SCOPE_FAIL({ successes = -1; }) {}
  {
    MAKE_SCOPE_EXIT(exit_guard) { successes = -1; };
    MAKE_SCOPE_SUCCESS(success_guard) { successes = -1; };
    MAKE_SCOPE_FAIL(fail_guard) { successes = -1; };
    MAKE_DEFER(defer_guard) { successes = -1; };
    exit_guard.dismiss();
    success_guard.dismiss();
    fail_guard.dismiss();
    defer_guard.dismiss();
  }
  if (successes != 1) {
    return 3;
  }

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  int failures = 0;
  try {
    SCOPE_FAIL{ ++failures; }; SCOPE_SUCCESS{ failures = -1; };
    WITH_SCOPE_FAIL({ ++failures; }) { throw std::runtime_error{"body failure"}; }
  } catch (const std::runtime_error&) {
  }
  if (failures != 2) {
    return 2;
  }
#endif

  return 0;
}
