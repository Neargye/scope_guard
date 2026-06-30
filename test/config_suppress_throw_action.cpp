// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <stdexcept>

int scope_guard_suppressed_exceptions = 0;

#define SCOPE_GUARD_SUPPRESS_THROW_ACTION
#define SCOPE_GUARD_CATCH_HANDLER ++scope_guard_suppressed_exceptions;
#include <scope_guard.hpp>

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
