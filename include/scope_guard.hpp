// scope_guard c++ https://github.com/Neargye/scope_guard
// Vesion 0.2.1
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
 public:
  ScopeExit() = delete;
  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit& operator=(ScopeExit&&) = delete;

  inline ScopeExit(ScopeExit&& other) noexcept(std::is_nothrow_move_constructible<A>::value || std::is_nothrow_copy_constructible<A>::value)
      : execute_{false},
        action_{std::move_if_noexcept(other.action_)} {
    execute_ = other.execute_;
    other.execute_ = false;
  }

  template <class T, typename = typename std::enable_if<std::is_constructible<A, T>::value>::type>
  inline explicit ScopeExit(T&& action) noexcept(std::is_nothrow_constructible<A, T>::value)
      : execute_{true},
        action_{std::move_if_noexcept(action)} {}

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
using ScopeExitDecay = ScopeExit<typename std::decay<A>::type>;

struct ScopeExitTag {};

template <typename A>
inline ScopeExitDecay<A> operator+(ScopeExitTag, A&& action) noexcept(noexcept(ScopeExitDecay<A>{static_cast<A&&>(action)})) {
  return ScopeExitDecay<A>{std::forward<A>(action)};
}

} // namespace detail

template <typename A>
inline detail::ScopeExitDecay<A> MakeScopeExit(A&& action) noexcept(noexcept(detail::ScopeExitDecay<A>{static_cast<A&&>(action)})) {
  return detail::ScopeExitDecay<A>{std::forward<A>(action)};
}

} // namespace scope_guard

#if defined(__GNUC__) || defined(__clang__)

#if defined(__has_cpp_attribute)

#if (__cplusplus >= 201703L) && __has_cpp_attribute(maybe_unused)
#define SCOPE_GUARD_ATTRIBUTE_UNUSED [[maybe_unused]]
#endif

#endif

#if !defined(SCOPE_GUARD_ATTRIBUTE_UNUSED)
#define SCOPE_GUARD_ATTRIBUTE_UNUSED __attribute__((unused))
#endif

#elif defined(_MSC_VER)

#if _MSC_VER >= 1911 && _HAS_CXX17
#define SCOPE_GUARD_ATTRIBUTE_UNUSED [[maybe_unused]]
#else
#define SCOPE_GUARD_ATTRIBUTE_UNUSED __pragma(warning(suppress : 4100 4101 4189))
#endif

#endif

#if !defined(SCOPE_GUARD_ATTRIBUTE_UNUSED)
#define SCOPE_GUARD_ATTRIBUTE_UNUSED
#endif

#define SCOPE_GUARD_CONCAT_(s1, s2) s1##s2
#define SCOPE_GUARD_CONCAT(s1, s2) SCOPE_GUARD_CONCAT_(s1, s2)

#define DEFER_TYPE SCOPE_GUARD_ATTRIBUTE_UNUSED auto

#define MAKE_DEFER ::scope_guard::detail::ScopeExitTag{} + [&]() noexcept -> void

#if defined(__COUNTER__)
#define DEFER \
  SCOPE_GUARD_ATTRIBUTE_UNUSED const auto \
  SCOPE_GUARD_CONCAT(__defer__object__, __COUNTER__) = MAKE_DEFER
#elif defined(__LINE__)
#define DEFER \
  SCOPE_GUARD_ATTRIBUTE_UNUSED const auto \
  SCOPE_GUARD_CONCAT(__defer__object__, __LINE__) = MAKE_DEFER
#endif
