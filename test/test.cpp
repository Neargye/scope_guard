// scope_guard c++11 test
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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <scope_guard.hpp>
#include <cstddef>
#include <fstream>
#include <stdexcept>

class ExecutionCounter {
  std::size_t counter = 0;

public:
  void Execute() { ++counter; }

  std::size_t GetCounter() const { return counter; }
};

TEST_CASE("called on scope leave") {
  SECTION("DEFER") {
    ExecutionCounter m;
    {
      DEFER{ m.Execute(); };
    }
    REQUIRE(m.GetCounter() == 1);
  }

  SECTION("custom") {
    ExecutionCounter m;
    {
      DEFER_TYPE custom_defer = MAKE_DEFER{ m.Execute(); };
    }
    REQUIRE(m.GetCounter() == 1);
  }
}

TEST_CASE("called on exception") {
  SECTION("DEFER") {
    ExecutionCounter m;
    {
      DEFER{ m.Execute(); };
      REQUIRE_THROWS(throw std::exception{});
    }
    REQUIRE(m.GetCounter() == 1);
  }

  SECTION("custom") {
    ExecutionCounter m;
    {
      DEFER_TYPE custom_defer = MAKE_DEFER{ m.Execute(); };
      REQUIRE_THROWS(throw std::exception{});
    }
    REQUIRE(m.GetCounter() == 1);
  }
}

TEST_CASE("dismiss before scope leave") {
  ExecutionCounter m;
  {
    DEFER_TYPE custom_defer = MAKE_DEFER{ m.Execute(); };
    custom_defer.Dismiss();
  }
  REQUIRE(m.GetCounter() == 0);
}

TEST_CASE("dismiss before exception") {
  ExecutionCounter m;
  {
    DEFER_TYPE custom_defer = MAKE_DEFER{ m.Execute(); };
    custom_defer.Dismiss();
    REQUIRE_THROWS(throw std::exception{});
  }
  REQUIRE(m.GetCounter() == 0);
}

TEST_CASE("called on for") {
  ExecutionCounter m;
  static const std::size_t execute_times = 10;
  for (std::size_t i = 0; i < execute_times; ++i)
    DEFER{ m.Execute(); };
  REQUIRE(m.GetCounter() == execute_times);
}

TEST_CASE("file") {
  SECTION("close on scope leave") {
    std::ofstream file;
    ExecutionCounter m;
    {
      file.open("test.txt", std::fstream::out | std::ofstream::trunc);
      DEFER{
        file.close();
        m.Execute();
      };
      file << "write to file" << std::endl;
    }
    REQUIRE(!file.is_open());
    REQUIRE(m.GetCounter() == 1);
  }

  SECTION("close on exceptione") {
    std::ofstream file;
    ExecutionCounter m;
    {
      file.open("test.txt", std::fstream::out | std::ofstream::trunc);
      DEFER{
        file.close();
        m.Execute();
      };
      file << "write to file" << std::endl;
      REQUIRE_THROWS(throw std::exception{});
    }
    REQUIRE(!file.is_open());
    REQUIRE(m.GetCounter() == 1);
  }

  SECTION("close if not") {
    std::ofstream file;
    ExecutionCounter m;
    {
      file.open("test.txt", std::fstream::out | std::ofstream::trunc);
      DEFER{
        if (file.is_open()) {
          file.close();
          m.Execute();
        }
      };
      file << "write to file" << std::endl;
      file.close();
    }
    REQUIRE(!file.is_open());
    REQUIRE(m.GetCounter() == 0);
  }
}
