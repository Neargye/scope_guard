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

void basic_scope_success() {
  std::vector<std::string> persons;

  {
    SCOPE_SUCCESS{
      persons.push_back("Ada");
      std::cout << "published Ada\n";
    };

    std::cout << "validated Ada\n";
  }

  std::cout << "persons after success: " << persons.size() << '\n';
}

void named_scope_success() {
  bool notified = false;

  {
    MAKE_SCOPE_SUCCESS(notify) {
      notified = true;
    };

    notify.dismiss();
  }

  std::cout << "notification sent after dismiss: " << (notified ? "yes" : "no") << '\n';
}

void factory_scope_success() {
  bool notified = false;

  {
    auto notify = scope_guard::make_scope_success([&]() {
      notified = true;
      std::cout << "factory success action\n";
    });
    (void)notify;
  }

  std::cout << "factory notification sent: " << (notified ? "yes" : "no") << '\n';
}

void with_scope_success() {
  WITH_SCOPE_SUCCESS({ std::cout << "leave WITH_SCOPE_SUCCESS normally\n"; }) {
    std::cout << "inside WITH_SCOPE_SUCCESS\n";
  }
}

void exceptional_scope_success() {
  bool notified = false;

  try {
    SCOPE_SUCCESS{
      notified = true;
    };

    throw std::runtime_error{"operation failed"};
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }

  std::cout << "notification sent after failure: " << (notified ? "yes" : "no") << '\n';
}

} // namespace

int main() {
  basic_scope_success();
  named_scope_success();
  factory_scope_success();
  with_scope_success();
  exceptional_scope_success();
}
