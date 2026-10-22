// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// CMake builds this source once for each expected failure selected by SCOPE_GUARD_TEST_*.

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

#elif defined(SCOPE_GUARD_TEST_REJECT_LVALUE_ACTION) || defined(SCOPE_GUARD_TEST_REJECT_CONST_RVALUE_ACTION) || defined(SCOPE_GUARD_TEST_REJECT_NON_MOVABLE_ACTION)

struct Action {
  void operator() () {}
};

struct NonMovableAction {
  NonMovableAction() = default;
  NonMovableAction(NonMovableAction&&) = delete;
  void operator() () {}
};

int main() {
#  if defined(SCOPE_GUARD_TEST_REJECT_LVALUE_ACTION)
  Action action;
  auto guard = scope_guard::SCOPE_GUARD_TEST_FACTORY(action);
#  elif defined(SCOPE_GUARD_TEST_REJECT_CONST_RVALUE_ACTION)
  const Action action{};
  auto guard = scope_guard::make_scope_exit(std::move(action));
#  else
  auto guard = scope_guard::make_scope_exit(NonMovableAction{});
#  endif
  (void)guard;
}

#elif defined(SCOPE_GUARD_TEST_DISCARD_RESULT)

int main() {
  scope_guard::SCOPE_GUARD_TEST_FACTORY([]() {});
}

#elif defined(SCOPE_GUARD_TEST_UNUSED_ACTION_VARIABLE)

int main() {
#  if defined(SCOPE_GUARD_TEST_WITH)
  WITH_SCOPE_EXIT({
    int unused_in_action = 0;
  }) {}
#  else
  SCOPE_EXIT{
    int unused_in_action = 0;
  };
#  endif
}

#elif defined(SCOPE_GUARD_TEST_REJECT_THROWING_DESTRUCTOR)

struct ThrowingDestructorAction {
  ThrowingDestructorAction() = default;
  ThrowingDestructorAction(ThrowingDestructorAction&&) noexcept {}
  ~ThrowingDestructorAction() noexcept(false) {}
  void operator() () noexcept {}
};

int main() {
  auto guard = scope_guard::make_scope_exit(ThrowingDestructorAction{});
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

#else
#  error "Select a scope_guard failure test."
#endif
