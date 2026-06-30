// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#include <scope_guard.hpp>

struct RvalueOnlyAction {
  void operator() () && {}
};

int main() {
  auto sg = scope_guard::make_scope_exit(RvalueOnlyAction{});
  (void)sg;
}
