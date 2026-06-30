// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#define SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE
#include <scope_guard.hpp>

struct ThrowingMoveConstructible {
  ThrowingMoveConstructible() = default;
  ThrowingMoveConstructible(ThrowingMoveConstructible&&) noexcept(false) {}

  void operator() () & noexcept {}
};

int main() {
  auto sg = scope_guard::make_scope_exit(ThrowingMoveConstructible{});
  (void)sg;
}
