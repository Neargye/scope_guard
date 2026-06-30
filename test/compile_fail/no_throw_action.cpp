// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#define SCOPE_GUARD_NO_THROW_ACTION
#include <scope_guard.hpp>

int main() {
  auto sg = scope_guard::make_scope_exit([]() {});
  (void)sg;
}
