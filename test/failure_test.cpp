// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// CMake builds this source once for each expected compile-time or runtime failure.
#if (defined(SCOPE_GUARD_TEST_MULTIPLE_THROW_POLICIES) + defined(SCOPE_GUARD_TEST_REJECT_THROWING_ACTION) + defined(SCOPE_GUARD_TEST_REJECT_THROWING_MOVE) + defined(SCOPE_GUARD_TEST_REJECT_LVALUE_ACTION) + defined(SCOPE_GUARD_TEST_THROW_DURING_UNWINDING)) != 1
#  error "Exactly one scope_guard failure test must be selected."
#endif

#if defined(SCOPE_GUARD_TEST_MULTIPLE_THROW_POLICIES)
#  define SCOPE_GUARD_MAY_THROW_ACTION
#  define SCOPE_GUARD_NO_THROW_ACTION
#elif defined(SCOPE_GUARD_TEST_REJECT_THROWING_ACTION)
#  define SCOPE_GUARD_NO_THROW_ACTION
#elif defined(SCOPE_GUARD_TEST_REJECT_THROWING_MOVE)
#  define SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE
#endif

#include <scope_guard.hpp>

#if defined(SCOPE_GUARD_TEST_MULTIPLE_THROW_POLICIES)

int main() {
  return 0;
}

#elif defined(SCOPE_GUARD_TEST_REJECT_THROWING_ACTION)

int main() {
  auto guard = scope_guard::make_scope_exit([]() {});
  (void)guard;
}

#elif defined(SCOPE_GUARD_TEST_REJECT_THROWING_MOVE)

struct ThrowingMoveConstructible {
  ThrowingMoveConstructible() = default;
  ThrowingMoveConstructible(ThrowingMoveConstructible&&) noexcept(false) {}

  void operator() () & noexcept {}
};

int main() {
  auto guard = scope_guard::make_scope_exit(ThrowingMoveConstructible{});
  (void)guard;
}

#elif defined(SCOPE_GUARD_TEST_REJECT_LVALUE_ACTION)

struct Action {
  void operator() () {}
};

int main() {
  Action action;
  auto guard = scope_guard::make_scope_exit(action);
  (void)guard;
}

#elif defined(SCOPE_GUARD_TEST_THROW_DURING_UNWINDING)

#  include <cstdio>
#  include <cstdlib>
#  include <exception>
#  include <stdexcept>

bool scope_fail_action_entered = false;

int main() {
  std::set_terminate([]() {
    if (!scope_fail_action_entered) {
      std::_Exit(43);
    }
    std::fputs("scope_fail action terminated\n", stderr);
    std::fflush(stderr);
    std::_Exit(42);
  });

  try {
    SCOPE_FAIL{
      scope_fail_action_entered = true;
      throw std::runtime_error{"cleanup failure"};
    };
    throw std::runtime_error{"body failure"};
  } catch (const std::runtime_error&) {
    // Reaching the handler means the action did not terminate during unwinding.
    return 0;
  }
}

#endif
