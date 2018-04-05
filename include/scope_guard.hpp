// scope_guard c++ https://github.com/Neargye/scope_guard
// Vesion 0.1.0
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

#pragma once
#include <utility>
#include <type_traits>

namespace scope_guard {

template <typename EF>
struct ScopeExit {
  ScopeExit() = delete;
  ScopeExit(const ScopeExit&) = delete;
  ScopeExit(ScopeExit&&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit& operator=(ScopeExit&&) = delete;

  inline constexpr ScopeExit(EF&& exit_function) noexcept : exit_function{std::forward<EF>(exit_function)} {}

  ~ScopeExit() noexcept { exit_function(); }

 private:
  EF exit_function;
};

template <typename EF>
inline constexpr ScopeExit<typename std::decay<EF>::type> MakeScopeExit(EF&& exit_function) noexcept {
  return {std::forward<EF>(exit_function)};
}

struct ScopeExitTag {};

template <typename EF>
inline constexpr ScopeExit<typename std::decay<EF>::type> operator+(ScopeExitTag, EF&& exit_function) noexcept {
  return {std::forward<EF>(exit_function)};

}

} // namespace scope_guard


#if defined(__has_cpp_attribute)
  #if __has_cpp_attribute(maybe_unused)
  #define SCOPE_GUARD_ATTRIBUTE_UNUSED [[maybe_unused]]
  #elif __has_cpp_attribute(gnu::unused)
  #define SCOPE_GUARD_ATTRIBUTE_UNUSED [[gnu::unused]]
  #endif
#elif defined(__GNUG__) || defined(__clang__)
#define SCOPE_GUARD_ATTRIBUTE_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#define SCOPE_GUARD_ATTRIBUTE_UNUSED __pragma(warning(suppress:4100 4101 4189))
#else
#define SCOPE_GUARD_ATTRIBUTE_UNUSED
#endif

#define SCOPE_GUARD_CONCAT_(s1, s2) s1##s2
#define SCOPE_GUARD_CONCAT(s1, s2) SCOPE_GUARD_CONCAT_(s1, s2)

#if defined(__COUNTER__)
#define DEFER \
  SCOPE_GUARD_ATTRIBUTE_UNUSED const auto& \
  SCOPE_GUARD_CONCAT(defer_object_, __COUNTER__) = ::scope_guard::ScopeExitTag{} + [&]() noexcept
#elif defined(__LINE__)
#define DEFER \
  SCOPE_GUARD_ATTRIBUTE_UNUSED const auto& \
  SCOPE_GUARD_CONCAT(defer_object_, __LINE__) = ::scope_guard::ScopeExitTag{} + [&]() noexcept
#else
#error ""
#endif
