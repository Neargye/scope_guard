// scope_guard example
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// Copyright (c) 2018 Daniil Goncharov <neargye@gmail.com>.
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
#include <fstream>

int Foo() { return 42; }

int main() {
  MAKE_DEFER(custom_defer1) {
    Foo();
    std::cout << "custom defer 1" << std::endl;
  };
  auto custom_defer2 = scope_guard::MakeScopeExit([&]() {
    std::cout << "custom defer 2" << std::endl;
    Foo();
  });

  std::fstream file;
  file.open("test.txt", std::fstream::out | std::fstream::trunc);
  DEFER {
    file.close();
    std::cout << "close file" << std::endl;
  };

  file << "example" << std::endl;
  std::cout << "write to file" << std::endl;

  custom_defer1.Dismiss();
  custom_defer2.Dismiss();

  // prints "write to file"
  // prints "close file"

  return 0;
}
