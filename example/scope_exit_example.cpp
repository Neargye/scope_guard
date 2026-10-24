// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cstdlib>
#include <iostream>
#include <stdexcept>

// Suppress and report exceptions from cleanup actions, as shown in suppressed_action_exception().
#define SCOPE_GUARD_SUPPRESS_THROW_ACTION
#define SCOPE_GUARD_CATCH_HANDLER std::cerr << "scope guard action failed\n";

#include <scope_guard.hpp>

namespace {

void basic_scope_exit() {
  void* buffer = std::malloc(64);
  if (!buffer) {
    std::cerr << "allocation failed\n";
    return;
  }

  SCOPE_EXIT{
    std::free(buffer);
    std::cout << "buffer freed\n";
  };

  std::cout << "buffer used\n";
}

void named_scope_exit() {
  MAKE_SCOPE_EXIT(rollback) {
    std::cout << "named rollback\n";
  };

  std::cout << "operation committed\n";
  rollback.dismiss();
}

void factory_scope_exit() {
  auto cleanup = scope_guard::make_scope_exit([]() {
    std::cout << "factory cleanup\n";
  });

  std::cout << "factory body\n";
}

void function_cleanup() {
  std::cout << "function cleanup\n";
}

void function_scope_exit() {
  auto cleanup = scope_guard::make_scope_exit(function_cleanup);

  std::cout << "function body\n";
}

void with_scope_exit() {
  WITH_SCOPE_EXIT({ std::cout << "leave WITH_SCOPE_EXIT\n"; }) {
    std::cout << "inside WITH_SCOPE_EXIT\n";
  }
}

void defer() {
  DEFER{
    std::cout << "deferred action\n";
  };

  std::cout << "before deferred action\n";
}

void suppressed_action_exception() {
  SCOPE_EXIT{
    std::cout << "throwing scope guard action\n";
    throw std::runtime_error{"cleanup failed"};
  };
}

} // namespace

int main() {
  basic_scope_exit();
  named_scope_exit();
  factory_scope_exit();
  function_scope_exit();
  with_scope_exit();
  defer();
  suppressed_action_exception();
}
