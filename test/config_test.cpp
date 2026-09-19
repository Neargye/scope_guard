// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// CMake builds this source once for each isolated header configuration.
#if (defined(SCOPE_GUARD_TEST_NO_THROW_ACTION) + defined(SCOPE_GUARD_TEST_SUPPRESS_THROW_ACTION) + defined(SCOPE_GUARD_TEST_THROWING_MOVE_CONSTRUCTION) + defined(SCOPE_GUARD_TEST_CXXABI)) != 1
#  error "Exactly one scope_guard configuration test must be selected."
#endif

#if defined(SCOPE_GUARD_TEST_CXXABI_FIRST)
#  include <cxxabi.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <stdexcept>
#include <type_traits>
#include <utility>

#if defined(SCOPE_GUARD_TEST_NO_THROW_ACTION)
#  define SCOPE_GUARD_NO_THROW_ACTION
#elif defined(SCOPE_GUARD_TEST_SUPPRESS_THROW_ACTION)
int scope_guard_suppressed_exceptions = 0;
#  define SCOPE_GUARD_SUPPRESS_THROW_ACTION
#  define SCOPE_GUARD_CATCH_HANDLER ++scope_guard_suppressed_exceptions;
#endif

#include <scope_guard.hpp>

#if defined(SCOPE_GUARD_TEST_CXXABI) && !defined(SCOPE_GUARD_TEST_CXXABI_FIRST)
#  include <cxxabi.h>
#endif

#if defined(SCOPE_GUARD_TEST_NO_THROW_ACTION)

struct NoThrowAction {
  void operator() () & noexcept {}
};

static_assert(std::is_nothrow_destructible<decltype(scope_guard::make_scope_exit(NoThrowAction{}))>::value,
              "SCOPE_GUARD_NO_THROW_ACTION should produce a noexcept destructor for noexcept actions.");

TEST_CASE("SCOPE_GUARD_NO_THROW_ACTION accepts noexcept actions") {
  int count = 0;

  REQUIRE_NOTHROW([&]() {
    SCOPE_EXIT{ ++count; };
  }());

  REQUIRE(count == 1);
}

#elif defined(SCOPE_GUARD_TEST_SUPPRESS_THROW_ACTION)

TEST_CASE("SCOPE_GUARD_SUPPRESS_THROW_ACTION suppresses cleanup exceptions") {
  int count = 0;
  scope_guard_suppressed_exceptions = 0;

  REQUIRE_NOTHROW([&]() {
    SCOPE_EXIT{
      ++count;
      throw std::runtime_error{"cleanup failure"};
    };
  }());

  REQUIRE(count == 1);
  REQUIRE(scope_guard_suppressed_exceptions == 1);
}

TEST_CASE("suppressed cleanup exceptions preserve the exception being unwound") {
  int exits = 0;
  int failures = 0;
  int successes = 0;
  scope_guard_suppressed_exceptions = 0;

  REQUIRE_THROWS_AS([&]() {
    SCOPE_EXIT{
      ++exits;
      throw std::logic_error{"exit cleanup failure"};
    };
    SCOPE_FAIL{
      ++failures;
      throw std::logic_error{"rollback failure"};
    };
    SCOPE_SUCCESS{ ++successes; };
    throw std::runtime_error{"body failure"};
  }(), std::runtime_error);

  REQUIRE(exits == 1);
  REQUIRE(failures == 1);
  REQUIRE(successes == 0);
  REQUIRE(scope_guard_suppressed_exceptions == 2);
}

#elif defined(SCOPE_GUARD_TEST_THROWING_MOVE_CONSTRUCTION)

struct ThrowingMoveAction {
  ThrowingMoveAction(int& executions, bool& throw_on_move)
      : executions_{&executions},
        throw_on_move_{&throw_on_move} {}
  ThrowingMoveAction(const ThrowingMoveAction&) = delete;
  ThrowingMoveAction(ThrowingMoveAction&& other)
      : executions_{other.executions_},
        throw_on_move_{other.throw_on_move_} {
    if (*throw_on_move_) {
      throw std::runtime_error{"move"};
    }
  }

  void operator() () {
    ++*executions_;
  }

  int* executions_;
  bool* throw_on_move_;
};

TEST_CASE("a throwing action move does not establish a guard") {
  int executions = 0;
  bool throw_on_move = true;

  REQUIRE_THROWS_AS([&]() {
    auto guard = scope_guard::make_scope_exit(ThrowingMoveAction{executions, throw_on_move});
    (void)guard;
  }(), std::runtime_error);
  REQUIRE(executions == 0);
}

TEST_CASE("a failed guard move leaves the source guard active") {
  int executions = 0;
  bool throw_on_move = false;

  {
    auto source = scope_guard::make_scope_exit(ThrowingMoveAction{executions, throw_on_move});
    throw_on_move = true;

    REQUIRE_THROWS_AS([&]() {
      auto destination = std::move(source);
      (void)destination;
    }(), std::runtime_error);
    REQUIRE(executions == 0);
  }

  REQUIRE(executions == 1);
}

TEST_CASE("a failed scope_fail move executes the source during unwinding") {
  int executions = 0;
  bool throw_on_move = false;

  REQUIRE_THROWS_AS([&]() {
    auto source = scope_guard::make_scope_fail(ThrowingMoveAction{executions, throw_on_move});
    throw_on_move = true;
    auto destination = std::move(source);
    (void)destination;
  }(), std::runtime_error);

  REQUIRE(executions == 1);
}

TEST_CASE("a failed scope_success move leaves the source active after the exception is caught") {
  int executions = 0;
  bool throw_on_move = false;

  {
    auto source = scope_guard::make_scope_success(ThrowingMoveAction{executions, throw_on_move});
    throw_on_move = true;

    REQUIRE_THROWS_AS([&]() {
      auto destination = std::move(source);
      (void)destination;
    }(), std::runtime_error);
    REQUIRE(executions == 0);
  }

  REQUIRE(executions == 1);
}

#elif defined(SCOPE_GUARD_TEST_CXXABI)

TEST_CASE("cxxabi header compatibility preserves exception counting") {
  int failures = 0;
  int successes = 0;

  REQUIRE_THROWS_AS([&]() {
    SCOPE_FAIL{ ++failures; };
    SCOPE_SUCCESS{ ++successes; };
    throw std::runtime_error{"body failure"};
  }(), std::runtime_error);

  REQUIRE(failures == 1);
  REQUIRE(successes == 0);
}

#endif
