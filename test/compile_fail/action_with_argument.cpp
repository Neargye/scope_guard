// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#include <scope_guard.hpp>

struct ActionWithArgument {
  void operator() (int) {}
};

int main() {
  auto sg = scope_guard::make_scope_exit(ActionWithArgument{});
  (void)sg;
}
