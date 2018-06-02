//   _____                         _____                     _    _____
//  / ____|                       / ____|                   | |  / ____|_     _
// | (___   ___ ___  _ __   ___  | |  __ _   _  __ _ _ __ __| | | |   _| |_ _| |_
//  \___ \ / __/ _ \| '_ \ / _ \ | | |_ | | | |/ _` | '__/ _` | | |  |_   _|_   _|
//  ____) | (_| (_) | |_) |  __/ | |__| | |_| | (_| | | | (_| | | |____|_|   |_|
// |_____/ \___\___/| .__/ \___|  \_____|\__,_|\__,_|_|  \__,_|  \_____|
//                  | | https://github.com/Neargye/scope_guard
//                  |_| vesion 0.2.7
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

#include <type_traits>
#include <utility>

namespace scope_guard {

template <typename A>
class ScopeExit final {
  using F = typename std::decay<A>::type;

  static_assert(std::is_same<void, decltype((std::declval<A>())())>::value,
                "ScopeExit requirement no-argument callable returns void");

 public:
  ScopeExit() = delete;
  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit& operator=(ScopeExit&&) = delete;

  inline ScopeExit(ScopeExit&& other) noexcept(noexcept(F{std::move(other.action_)}))
      : execute_{false},
        action_{std::move(other.action_)} {
    execute_ = other.execute_;
    other.execute_ = false;
  }

  template <class T, typename = typename std::enable_if<std::is_constructible<F, T>::value ||
                                                        std::is_constructible<F, T&>::value>::type>
  inline explicit ScopeExit(T&& action) noexcept(noexcept(F{std::forward<T>(action)}))
      : execute_{true},
        action_{std::forward<T>(action)} {}

  inline void Dismiss() noexcept {
    execute_ = false;
  }

  inline ~ScopeExit() noexcept {
    if (execute_) {
      action_();
      execute_ = false;
    }
  }

 private:
  bool execute_;
  F action_;
};

namespace detail {

struct ScopeExitTag {};

template <typename A>
inline ScopeExit<A> operator+(ScopeExitTag, A&& action) noexcept(noexcept(ScopeExit<A>{std::forward<A>(action)})) {
  return ScopeExit<A>{std::forward<A>(action)};
}

} // namespace detail

template <typename A>
inline ScopeExit<A> MakeScopeExit(A&& action) noexcept(noexcept(ScopeExit<A>{std::forward<A>(action)})) {
  return ScopeExit<A>{std::forward<A>(action)};
}

} // namespace scope_guard

// CPP_ATTRIBUTE_MAYBE_UNUSED indicates that a function, variable or parameter might or might not be used.
#if !defined(CPP_ATTRIBUTE_MAYBE_UNUSED)
#  if defined(_MSC_VER)
#    if (_MSC_VER >= 1911 && _MSVC_LANG >= 201703L)
#      define CPP_ATTRIBUTE_MAYBE_UNUSED [[maybe_unused]]
#    else
#      define CPP_ATTRIBUTE_MAYBE_UNUSED __pragma(warning(suppress : 4100 4101 4189))
#    endif
#  elif defined(__clang__)
#    if ((__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 9)) && __cplusplus >= 201703L)
#      define CPP_ATTRIBUTE_MAYBE_UNUSED [[maybe_unused]]
#    else
#      define CPP_ATTRIBUTE_MAYBE_UNUSED __attribute__((__unused__))
#    endif
#  elif defined(__GNUC__)
#    if (__GNUC__ > 7 && __cplusplus >= 201703L)
#      define CPP_ATTRIBUTE_MAYBE_UNUSED [[maybe_unused]]
#    else
#      define CPP_ATTRIBUTE_MAYBE_UNUSED __attribute__((__unused__))
#    endif
#  else
#    define CPP_ATTRIBUTE_MAYBE_UNUSED
#  endif
#endif

#if !defined(STR_CONCAT_)
#  define STR_CONCAT_(s1, s2) s1##s2
#endif

#if !defined(STR_CONCAT)
#  define STR_CONCAT(s1, s2) STR_CONCAT_(s1, s2)
#endif

#define MAKE_SCOPE_EXIT(name) \
  auto name = ::scope_guard::detail::ScopeExitTag{} + [&]() noexcept -> void

#if defined(__COUNTER__)
#  define SCOPE_EXIT                 \
    CPP_ATTRIBUTE_MAYBE_UNUSED const \
    MAKE_SCOPE_EXIT(STR_CONCAT(__scope_exit__object__, __COUNTER__))
#elif defined(__LINE__)
#  define SCOPE_EXIT                 \
    CPP_ATTRIBUTE_MAYBE_UNUSED const \
    MAKE_SCOPE_EXIT(STR_CONCAT(__scope_exit__object__, __LINE__))
#endif

#define DEFER SCOPE_EXIT

#define MAKE_DEFER(name) MAKE_SCOPE_EXIT(name)
