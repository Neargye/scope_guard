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

#include <scope_guard.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void basic_scope_fail() {
  std::vector<std::string> persons;

  try {
    persons.push_back("Ada");
    SCOPE_FAIL{
      persons.pop_back();
      std::cout << "rolled back Ada\n";
    };

    throw std::runtime_error{"database update failed"};
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }

  std::cout << "persons after rollback: " << persons.size() << '\n';
}

void named_scope_fail() {
  std::vector<std::string> persons;

  try {
    persons.push_back("Grace");
    MAKE_SCOPE_FAIL(rollback) {
      persons.pop_back();
      std::cout << "rolled back Grace\n";
    };

    rollback.dismiss();
    throw std::runtime_error{"later operation failed"};
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }

  std::cout << "persons after dismiss: " << persons.size() << '\n';
}

void factory_scope_fail() {
  bool rolled_back = false;

  try {
    auto rollback = scope_guard::make_scope_fail([&]() {
      rolled_back = true;
      std::cout << "factory rollback\n";
    });
    (void)rollback;

    throw std::runtime_error{"factory operation failed"};
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }

  std::cout << "factory rolled back: " << (rolled_back ? "yes" : "no") << '\n';
}

void with_scope_fail() {
  try {
    WITH_SCOPE_FAIL({ std::cout << "leave WITH_SCOPE_FAIL on failure\n"; }) {
      std::cout << "inside WITH_SCOPE_FAIL\n";
      throw std::runtime_error{"WITH_SCOPE_FAIL operation failed"};
    }
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }
}

} // namespace

int main() {
  basic_scope_fail();
  named_scope_fail();
  factory_scope_fail();
  with_scope_fail();
}
