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
  static_assert(::std::is_same<void, decltype((::std::declval<A>())())>::value,
                "ScopeExit requirement no-argument callable returns void");

 public:
  ScopeExit() = delete;
  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit& operator=(ScopeExit&&) = delete;

  inline ScopeExit(ScopeExit&& other) noexcept(noexcept(A{::std::forward<A>(other.action_)}))
      : execute_{false},
        action_{::std::forward<A>(other.action_)} {
    execute_ = other.execute_;
    other.execute_ = false;
  }

  template <class T, typename = typename ::std::enable_if<::std::is_constructible<A, T>::value ||
                                                          ::std::is_constructible<A, T&>::value>::type>
  inline explicit ScopeExit(T&& action) noexcept(noexcept(A{::std::forward<T>(action)}))
      : execute_{true},
        action_{::std::forward<T>(action)} {}

  inline void Dismiss() noexcept {
    execute_ = false;
  }

  inline ~ScopeExit() noexcept {
    if (execute_)
      action_();
    execute_ = false;
  }

 private:
  bool execute_;
  A action_;
};

namespace detail {

template <typename A>
using ScopeExitDecay = ScopeExit<typename ::std::decay<A>::type>;

struct ScopeExitTag {};

template <typename A>
inline ScopeExitDecay<A> operator+(ScopeExitTag, A&& action) noexcept(noexcept(ScopeExitDecay<A>{static_cast<A&&>(action)})) {
  return ScopeExitDecay<A>{::std::forward<A>(action)};
}

} // namespace detail

template <typename A>
inline detail::ScopeExitDecay<A> MakeScopeExit(A&& action) noexcept(noexcept(detail::ScopeExitDecay<A>{static_cast<A&&>(action)})) {
  return detail::ScopeExitDecay<A>{::std::forward<A>(action)};
}

} // namespace scope_guard

#if !defined(CPP_HAS_ATTRIBUTE)
#  if defined(__has_cpp_attribute)
#    define CPP_HAS_ATTRIBUTE(x) __has_cpp_attribute(x)
#  else
#    define CPP_HAS_ATTRIBUTE(x) 0
#  endif
#endif

// CPP_ATTRIBUTE_UNUSED indicates that a function, variable or parameter might or might not be used.
#if !defined(CPP_ATTRIBUTE_UNUSED)
#  if defined(_MSC_VER)
#    if CPP_HAS_ATTRIBUTE(maybe_unused) || (_MSC_VER >= 1911 && _MSVC_LANG >= 201703L)
#      define CPP_ATTRIBUTE_UNUSED [[maybe_unused]]
#    else
#      define CPP_ATTRIBUTE_UNUSED __pragma(warning(suppress : 4100 4101 4189))
#    endif
#  elif defined(__GNUC__) || defined(__clang__)
#    if (__cplusplus >= 201703L) && CPP_HAS_ATTRIBUTE(maybe_unused)
#      define CPP_ATTRIBUTE_UNUSED [[maybe_unused]]
#    else
#      define CPP_ATTRIBUTE_UNUSED __attribute__((unused))
#    endif
#  else
#    define CPP_ATTRIBUTE_UNUSED
#  endif
#endif

#if !defined(STR_CONCAT_)
#  define STR_CONCAT_(s1, s2) s1##s2
#endif

#if !defined(STR_CONCAT)
#  define STR_CONCAT(s1, s2) STR_CONCAT_(s1, s2)
#endif

#define MAKE_DEFER(defer_name) \
  auto                         \
  defer_name =                 \
  ::scope_guard::detail::ScopeExitTag{} + [&]() noexcept -> void

#if defined(__COUNTER__)
#  define DEFER                                  \
    CPP_ATTRIBUTE_UNUSED const auto              \
    STR_CONCAT(__defer__object__, __COUNTER__) = \
    ::scope_guard::detail::ScopeExitTag{} + [&]() noexcept -> void
#elif defined(__LINE__)
#  define DEFER                               \
    CPP_ATTRIBUTE_UNUSED const auto           \
    STR_CONCAT(__defer__object__, __LINE__) = \
    ::scope_guard::detail::ScopeExitTag{} + [&]() noexcept -> void
#endif
