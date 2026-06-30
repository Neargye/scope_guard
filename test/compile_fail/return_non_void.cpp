// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#include <scope_guard.hpp>

struct ReturnNonVoid {
  int operator() () {
    return 0;
  }
};

int main() {
  auto sg = scope_guard::make_scope_exit(ReturnNonVoid{});
  (void)sg;
}
