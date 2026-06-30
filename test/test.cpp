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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#define SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE
#include <scope_guard.hpp>

#include <stdexcept>
#include <type_traits>
#include <utility>

struct ExecutionCounter {
  void Execute() {
    ++count;
  }

  int count = 0;
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

struct LvalueNoexceptRvalueThrow {
  void operator() () & noexcept {}
  void operator() () && {}
};

struct RvalueOnly {
  void operator() () && {}
};

struct ReturnsInt {
  int operator() () {
    return 0;
  }
};

static_assert(scope_guard::detail::is_noarg_returns_void_action<LvalueNoexceptRvalueThrow&>::value,
              "scope_guard should validate the stored action as an lvalue.");
static_assert(!scope_guard::detail::is_noarg_returns_void_action<RvalueOnly&>::value,
              "scope_guard should reject actions that cannot be called as stored lvalues.");
static_assert(!scope_guard::detail::is_noarg_returns_void_action<ReturnsInt&>::value,
              "scope_guard should reject actions that do not return void.");
static_assert(std::is_nothrow_destructible<decltype(scope_guard::make_scope_exit(LvalueNoexceptRvalueThrow{}))>::value,
              "scope_guard should compute noexcept from the stored lvalue action.");

int with_scope_return_count = 0;
int function_pointer_count = 0;

void function_pointer_cleanup() {
  ++function_pointer_count;
}

int return_from_with_scope_exit() {
  WITH_SCOPE_EXIT({ ++with_scope_return_count; }) {
    return 1;
  }

  return 0;
}

int return_from_with_defer() {
  WITH_DEFER({ ++with_scope_return_count; }) {
    return 1;
  }

  return 0;
}

TEST_CASE("called on scope leave") {
  SUBCASE("scope_exit") {
    ExecutionCounter m;

    REQUIRE_NOTHROW([&]() {
      SCOPE_EXIT{ m.Execute(); };
    }());
    REQUIRE(m.count == 1);
  }

  SUBCASE("scope_fail") {
    ExecutionCounter m;

    REQUIRE_NOTHROW([&]() {
      SCOPE_FAIL{ m.Execute(); };
    }());
    REQUIRE(m.count == 0);
  }

  SUBCASE("scope_success") {
    ExecutionCounter m;

    REQUIRE_NOTHROW([&]() {
      SCOPE_SUCCESS{ m.Execute(); };
    }());
    REQUIRE(m.count == 1);
  }
}

TEST_CASE("factory functions create guards") {
  SUBCASE("make_scope_exit") {
    int count = 0;

    REQUIRE_NOTHROW([&]() {
      auto sg = scope_guard::make_scope_exit([&]() { ++count; });
    }());
    REQUIRE(count == 1);
  }

  SUBCASE("make_scope_fail") {
    int count = 0;

    REQUIRE_THROWS([&]() {
      auto sg = scope_guard::make_scope_fail([&]() { ++count; });

      throw std::exception{};
    }());
    REQUIRE(count == 1);
  }

  SUBCASE("make_scope_success") {
    int count = 0;

    REQUIRE_NOTHROW([&]() {
      auto sg = scope_guard::make_scope_success([&]() { ++count; });
    }());
    REQUIRE(count == 1);
  }

  SUBCASE("function pointer action") {
    function_pointer_count = 0;

    REQUIRE_NOTHROW([&]() {
      auto sg = scope_guard::make_scope_exit(&function_pointer_cleanup);
    }());
    REQUIRE(function_pointer_count == 1);
  }
}

TEST_CASE("move transfers execution ownership") {
  int count = 0;

  REQUIRE_NOTHROW([&]() {
    auto sg1 = scope_guard::make_scope_exit([&]() { ++count; });
    auto sg2 = std::move(sg1);
  }());

  REQUIRE(count == 1);
}

TEST_CASE("default action exceptions propagate") {
  REQUIRE_THROWS_AS([&]() {
    SCOPE_EXIT{ throw std::runtime_error{"cleanup failure"}; };
  }(), std::runtime_error);
}

TEST_CASE("with scope guard executes on scope leave") {
  SUBCASE("scope_exit normal leave") {
    int count = 0;

    WITH_SCOPE_EXIT({ ++count; }) {
      REQUIRE(count == 0);
    }

    REQUIRE(count == 1);
  }

  SUBCASE("scope_exit break") {
    int count = 0;

    WITH_SCOPE_EXIT({ ++count; }) {
      break;
    }

    REQUIRE(count == 1);
  }

  SUBCASE("scope_exit exception") {
    int count = 0;

    REQUIRE_THROWS([&]() {
      WITH_SCOPE_EXIT({ ++count; }) {
        throw std::exception{};
      }
    }());

    REQUIRE(count == 1);
  }

  SUBCASE("scope_exit return") {
    with_scope_return_count = 0;

    REQUIRE(return_from_with_scope_exit() == 1);
    REQUIRE(with_scope_return_count == 1);
  }

  SUBCASE("scope_fail exception") {
    int count = 0;

    REQUIRE_THROWS([&]() {
      WITH_SCOPE_FAIL({ ++count; }) {
        throw std::exception{};
      }
    }());

    REQUIRE(count == 1);
  }

  SUBCASE("scope_fail normal leave") {
    int count = 0;

    WITH_SCOPE_FAIL({ ++count; }) {
      REQUIRE(count == 0);
    }

    REQUIRE(count == 0);
  }

  SUBCASE("scope_success normal leave") {
    int count = 0;

    WITH_SCOPE_SUCCESS({ ++count; }) {
      REQUIRE(count == 0);
    }

    REQUIRE(count == 1);
  }

  SUBCASE("scope_success exception") {
    int count = 0;

    REQUIRE_THROWS([&]() {
      WITH_SCOPE_SUCCESS({ ++count; }) {
        throw std::exception{};
      }
    }());

    REQUIRE(count == 0);
  }
}

TEST_CASE("defer aliases scope_exit") {
  SUBCASE("defer") {
    int count = 0;

    REQUIRE_NOTHROW([&]() {
      DEFER{ ++count; };
    }());
    REQUIRE(count == 1);
  }

  SUBCASE("make_defer") {
    int count = 0;

    REQUIRE_NOTHROW([&]() {
      MAKE_DEFER(sg){ ++count; };
      sg.dismiss();
    }());
    REQUIRE(count == 0);
  }

  SUBCASE("with_defer return") {
    with_scope_return_count = 0;

    REQUIRE(return_from_with_defer() == 1);
    REQUIRE(with_scope_return_count == 1);
  }
}

TEST_CASE("called on exception") {
  SUBCASE("scope_exit") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      SCOPE_EXIT{ m.Execute(); };

      throw std::exception{};
    }());
    REQUIRE(m.count == 1);
  }

  SUBCASE("scope_fail") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      SCOPE_FAIL{ m.Execute(); };

      throw std::exception{};
    }());
    REQUIRE(m.count == 1);
  }

  SUBCASE("scope_success") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      SCOPE_SUCCESS{ m.Execute(); };

      throw std::exception{};
    }());
    REQUIRE(m.count == 0);
  }
}

TEST_CASE("dismiss before scope leave") {
  SUBCASE("scope_exit") {
    ExecutionCounter m;

    REQUIRE_NOTHROW([&]() {
      MAKE_SCOPE_EXIT(sg){ m.Execute(); };
      sg.dismiss();
    }());
    REQUIRE(m.count == 0);
  }

  SUBCASE("scope_fail") {
    ExecutionCounter m;

    REQUIRE_NOTHROW([&]() {
      MAKE_SCOPE_FAIL(sg){ m.Execute(); };
      sg.dismiss();
    }());
    REQUIRE(m.count == 0);
  }

  SUBCASE("scope_success") {
    ExecutionCounter m;

    REQUIRE_NOTHROW([&]() {
      MAKE_SCOPE_SUCCESS(sg){ m.Execute(); };
      sg.dismiss();
    }());
    REQUIRE(m.count == 0);
  }
}

TEST_CASE("dismiss before exception") {
  SUBCASE("scope_exit") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      MAKE_SCOPE_EXIT(sg){ m.Execute(); };

      sg.dismiss();

      throw std::exception{};
    }());
    REQUIRE(m.count == 0);
  }

  SUBCASE("scope_fail") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      MAKE_SCOPE_FAIL(sg){ m.Execute(); };

      sg.dismiss();

      throw std::exception{};
    }());
    REQUIRE(m.count == 0);
  }

  SUBCASE("scope_success") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      MAKE_SCOPE_SUCCESS(sg){ m.Execute(); };

      sg.dismiss();

      throw std::exception{};
    }());
    REQUIRE(m.count == 0);
  }
}

TEST_CASE("called on exception, dismiss after exception") {
  SUBCASE("scope_exit") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      MAKE_SCOPE_EXIT(sg){ m.Execute(); };

      throw std::exception{};

      sg.dismiss();
    }());
    REQUIRE(m.count == 1);
  }

  SUBCASE("scope_fail") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      MAKE_SCOPE_FAIL(sg){ m.Execute(); };

      throw std::exception{};

      sg.dismiss();
    }());
    REQUIRE(m.count == 1);
  }

  SUBCASE("scope_success") {
    ExecutionCounter m;

    REQUIRE_THROWS([&]() {
      MAKE_SCOPE_SUCCESS(sg){ m.Execute(); };

      throw std::exception{};

      sg.dismiss();
    }());
    REQUIRE(m.count == 0);
  }
}
