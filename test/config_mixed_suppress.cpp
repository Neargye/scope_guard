// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// Linked with config_mixed_test.cpp, which uses the default action setting.
#define SCOPE_GUARD_SUPPRESS_THROW_ACTION
#include <scope_guard.hpp>

void throwing_action();

bool suppressed_in_other_translation_unit() {
  try {
    {
      auto guard = scope_guard::make_scope_exit(&throwing_action);
      (void)guard;
    }
    return true;
  } catch (...) {
    return false;
  }
}
