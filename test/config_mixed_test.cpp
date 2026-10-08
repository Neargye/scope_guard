// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// Linked with config_mixed_suppress.cpp, which uses SCOPE_GUARD_SUPPRESS_THROW_ACTION.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <scope_guard.hpp>

#include <stdexcept>

void throwing_action() {
  throw std::runtime_error{"cleanup failure"};
}

bool suppressed_in_other_translation_unit();

TEST_CASE("translation units with different action settings do not share guard destructors") {
  REQUIRE_THROWS_AS([]() {
    auto guard = scope_guard::make_scope_exit(&throwing_action);
    (void)guard;
  }(), std::runtime_error);

  REQUIRE(suppressed_in_other_translation_unit());
}
