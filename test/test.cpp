// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2019 Daniil Goncharov <neargye@gmail.com>.
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

#define TROMPELOEIL_SANITY_CHECKS
#include <trompeloeil.hpp>

#include <scope_guard.hpp>

#include <cstddef>
#include <fstream>
#include <stdexcept>

struct ExecutionCounter {
  MAKE_MOCK0(Execute, void());
};

class F {
public:
  F() = default;
  F(F&&) = default;
  F(const F&) = default;
  ~F() = default;
  F& operator=(const F&) = default;
  F& operator=(F&&) = default;

  void operator() () {}
};

TEST_CASE("called on scope leave") {
  SECTION("defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(1));

    REQUIRE_NOTHROW([&]() {
      DEFER{ m.Execute(); };
    }());
  }

  SECTION("custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(1));

    REQUIRE_NOTHROW([&]() {
      MAKE_DEFER(custom_defer){ m.Execute(); };
    }());
  }

  SECTION("multi defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(3));

    REQUIRE_NOTHROW([&]() {
      DEFER{ m.Execute(); };
      DEFER{ m.Execute(); };
      DEFER{ m.Execute(); };
    }());
  }

  SECTION("multi custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(3));

    REQUIRE_NOTHROW([&]() {
      MAKE_DEFER(custom_defer_1){ m.Execute(); };
      MAKE_DEFER(custom_defer_2){ m.Execute(); };
      MAKE_DEFER(custom_defer_3){ m.Execute(); };
    }());
  }
}

TEST_CASE("called on exception") {
  SECTION("defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(1));

    REQUIRE_THROWS([&]() {
      DEFER{ m.Execute(); };

      throw std::exception{};
    }());
  }

  SECTION("custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(1));

    REQUIRE_THROWS([&]() {
      MAKE_DEFER(custom_defer){ m.Execute(); };

      throw std::exception{};
    }());
  }

  SECTION("multi defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(3));

    REQUIRE_THROWS([&]() {
      DEFER{ m.Execute(); };
      DEFER{ m.Execute(); };
      DEFER{ m.Execute(); };

      throw std::exception{};
    }());
  }

  SECTION("multi custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(3));

    REQUIRE_THROWS([&]() {
      MAKE_DEFER(custom_defer_1){ m.Execute(); };
      MAKE_DEFER(custom_defer_2){ m.Execute(); };
      MAKE_DEFER(custom_defer_3){ m.Execute(); };

      throw std::exception{};
    }());
  }
}

TEST_CASE("dismiss before scope leave") {
  SECTION("custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(0));

    REQUIRE_NOTHROW([&]() {
      MAKE_DEFER(custom_defer){ m.Execute(); };
      custom_defer.Dismiss();
    }());
  }

  SECTION("multi custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(3));

    REQUIRE_NOTHROW([&]() {
      MAKE_DEFER(custom_defer_1){ m.Execute(); };
      MAKE_DEFER(custom_defer_2){ m.Execute(); };
      MAKE_DEFER(custom_defer_3){ m.Execute(); };
    }());
  }
}

TEST_CASE("dismiss before exception") {
  SECTION("custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(0));

    REQUIRE_THROWS([&]() {
      MAKE_DEFER(custom_defer){ m.Execute(); };

      custom_defer.Dismiss();

      throw std::exception{};
    }());
  }

  SECTION("multi custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(0));

    REQUIRE_THROWS([&]() {
      MAKE_DEFER(custom_defer_1){ m.Execute(); };
      MAKE_DEFER(custom_defer_2){ m.Execute(); };
      MAKE_DEFER(custom_defer_3){ m.Execute(); };

      custom_defer_1.Dismiss();
      custom_defer_2.Dismiss();
      custom_defer_3.Dismiss();

      throw std::exception{};
    }());
  }
}

TEST_CASE("called on exception, dismiss after exception") {
  SECTION("custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(1));

    REQUIRE_THROWS([&]() {
      MAKE_DEFER(custom_defer){ m.Execute(); };

      throw std::exception{};

      custom_defer.Dismiss();
    }());
  }

  SECTION("multi custom defer") {
    ExecutionCounter m;
    REQUIRE_CALL_V(m, Execute(),
                   .TIMES(3));

    REQUIRE_THROWS([&]() {
      MAKE_DEFER(custom_defer_1){ m.Execute(); };
      MAKE_DEFER(custom_defer_2){ m.Execute(); };
      MAKE_DEFER(custom_defer_3){ m.Execute(); };

      throw std::exception{};

      custom_defer_1.Dismiss();
      custom_defer_2.Dismiss();
      custom_defer_3.Dismiss();
    }());
  }
}

TEST_CASE("called on for") {
  ExecutionCounter m;
  static const std::size_t execute_times = 10;
  REQUIRE_CALL_V(m, Execute(),
                 .TIMES(execute_times));

  for (std::size_t i = 0; i < execute_times; ++i) {
    DEFER{ m.Execute(); };
  }
}
