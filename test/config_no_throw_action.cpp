// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#define SCOPE_GUARD_NO_THROW_ACTION
#include <scope_guard.hpp>

#include <type_traits>

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
