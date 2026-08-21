// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#include <scope_guard.hpp>

int main() {
  int count = 0;

  {
    SCOPE_EXIT{ ++count; };
    SCOPE_FAIL{ ++count; };
    SCOPE_SUCCESS{ ++count; };
  }

  return count == 2 ? 0 : 1;
}
