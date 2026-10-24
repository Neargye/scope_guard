//   _____                         _____                     _    _____
//  / ____|                       / ____|                   | |  / ____|_     _
// | (___   ___ ___  _ __   ___  | |  __ _   _  __ _ _ __ __| | | |   _| |_ _| |_
//  \___ \ / __/ _ \| '_ \ / _ \ | | |_ | | | |/ _` | '__/ _` | | |  |_   _|_   _|
//  ____) | (_| (_) | |_) |  __/ | |__| | |_| | (_| | | | (_| | | |____|_|   |_|
// |_____/ \___\___/| .__/ \___|  \_____|\__,_|\__,_|_|  \__,_|  \_____|
//                  | | https://github.com/Neargye/scope_guard
//                  |_| version 0.9.5
//
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

#ifndef NEARGYE_SCOPE_GUARD_HPP
#define NEARGYE_SCOPE_GUARD_HPP

#define SCOPE_GUARD_VERSION_MAJOR 0
#define SCOPE_GUARD_VERSION_MINOR 9
#define SCOPE_GUARD_VERSION_PATCH 5

#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>

// scope_guard exception settings:
// SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE requires the action to be nothrow move-constructible.
// SCOPE_GUARD_MAY_THROW_ACTION allows exceptions from the action to propagate (default).
// SCOPE_GUARD_NO_THROW_ACTION requires the action to be noexcept.
// SCOPE_GUARD_SUPPRESS_THROW_ACTION suppresses exceptions thrown by the action.
// SCOPE_GUARD_CATCH_HANDLER is a non-throwing statement run when an action exception is caught. It is ignored unless SCOPE_GUARD_SUPPRESS_THROW_ACTION is defined.
// Configure these settings consistently in every translation unit before including this header.

#if !defined(SCOPE_GUARD_MAY_THROW_ACTION) && !defined(SCOPE_GUARD_NO_THROW_ACTION) && !defined(SCOPE_GUARD_SUPPRESS_THROW_ACTION)
#  define SCOPE_GUARD_MAY_THROW_ACTION
#elif (defined(SCOPE_GUARD_MAY_THROW_ACTION) + defined(SCOPE_GUARD_NO_THROW_ACTION) + defined(SCOPE_GUARD_SUPPRESS_THROW_ACTION)) > 1
#  error Only one of SCOPE_GUARD_MAY_THROW_ACTION and SCOPE_GUARD_NO_THROW_ACTION and SCOPE_GUARD_SUPPRESS_THROW_ACTION may be defined.
#endif

#if !defined(SCOPE_GUARD_CATCH_HANDLER)
#  define SCOPE_GUARD_CATCH_HANDLER /* Suppress exception.*/
#endif

#if !defined(_MSC_VER) && (defined(__clang__) || defined(__GNUC__)) && __cplusplus < 201703L
#  include <cxxabi.h>
// Clang reports __GNUC__ 4.2, so it also takes this branch and declares __cxa_get_globals.
#  if !defined(__FreeBSD__) && (defined(_LIBCPPABI_VERSION) || defined(__OpenBSD__) || \
      (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407) || \
      (defined(__QNXNTO__) && !defined(__GLIBCXX__) && !defined(__GLIBCPP__)))
namespace __cxxabiv1 {
struct __cxa_eh_globals;
#    if defined(__OpenBSD__)
extern "C" __cxa_eh_globals* __cxa_get_globals();
#    else
extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept;
#    endif
}
#  endif
#endif

namespace scope_guard {

namespace detail {

#if defined(SCOPE_GUARD_SUPPRESS_THROW_ACTION) && (defined(__cpp_exceptions) || defined(_CPPUNWIND))
#  define NEARGYE_SCOPE_GUARD_NOEXCEPT(...) noexcept
#  define NEARGYE_SCOPE_GUARD_TRY           try {
#  define NEARGYE_SCOPE_GUARD_CATCH         } catch (...) { SCOPE_GUARD_CATCH_HANDLER }
#else
#  define NEARGYE_SCOPE_GUARD_NOEXCEPT(...) noexcept(__VA_ARGS__)
#  define NEARGYE_SCOPE_GUARD_TRY
#  define NEARGYE_SCOPE_GUARD_CATCH
#endif

#define NEARGYE_SCOPE_GUARD_MOV(...) static_cast<typename std::remove_reference<decltype(__VA_ARGS__)>::type&&>(__VA_ARGS__)
#define NEARGYE_SCOPE_GUARD_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

// NEARGYE_SCOPE_GUARD_NODISCARD encourages the compiler to issue a warning if the return value is discarded.
#if defined(__clang__) || defined(__GNUC__)
#  if __cplusplus >= 201703L
#    define NEARGYE_SCOPE_GUARD_NODISCARD [[nodiscard]]
#  else
#    define NEARGYE_SCOPE_GUARD_NODISCARD __attribute__((__warn_unused_result__))
#  endif
#elif defined(_MSC_VER)
#  define NEARGYE_SCOPE_GUARD_NODISCARD [[nodiscard]]
#else
#  define NEARGYE_SCOPE_GUARD_NODISCARD
#endif

#if !defined(_MSC_VER) && (defined(__clang__) || defined(__GNUC__)) && __cplusplus < 201703L
inline int uncaught_exceptions() noexcept {
  // __cxa_eh_globals starts with the caughtExceptions pointer, followed by uncaughtExceptions.
  return static_cast<int>(*reinterpret_cast<const unsigned int*>(reinterpret_cast<const char*>(::__cxxabiv1::__cxa_get_globals()) + sizeof(void*)));
}
#else
inline int uncaught_exceptions() noexcept {
  return std::uncaught_exceptions();
}
#endif

class on_exit_policy {
  bool execute_;

 public:
  explicit on_exit_policy(bool execute) noexcept : execute_{execute} {}

  void dismiss() noexcept {
    execute_ = false;
  }

  bool should_execute() const noexcept {
    return execute_;
  }

  template <typename F>
  static void construction_failed(F& action) {
    action();
  }
};

// Failure means more exceptions are in flight than at construction. The count is per thread.
class on_fail_policy {
  int ec_;

 public:
  explicit on_fail_policy(bool execute) noexcept : ec_{execute ? uncaught_exceptions() : -1} {}

  void dismiss() noexcept {
    ec_ = -1;
  }

  bool should_execute() const noexcept {
    return ec_ != -1 && ec_ < uncaught_exceptions();
  }

  template <typename F>
  static void construction_failed(F& action) {
    action();
  }
};

class on_success_policy {
  int ec_;

 public:
  explicit on_success_policy(bool execute) noexcept : ec_{execute ? uncaught_exceptions() : -1} {}

  void dismiss() noexcept {
    ec_ = -1;
  }

  bool should_execute() const noexcept {
    return ec_ != -1 && ec_ >= uncaught_exceptions();
  }

  template <typename F>
  static void construction_failed(F&) noexcept {}
};

template <typename T, typename = void>
struct is_noarg_returns_void_action
    : std::false_type {};

template <typename T>
struct is_noarg_returns_void_action<T, decltype((std::declval<T>())())>
    : std::true_type {};

template <typename T, bool = is_noarg_returns_void_action<T>::value>
struct is_nothrow_invocable_action
    : std::false_type {};

template <typename T>
struct is_nothrow_invocable_action<T, true>
    : std::integral_constant<bool, noexcept((std::declval<T>())())> {};

template <typename F>
struct is_storable_action
    : std::integral_constant<bool, std::is_convertible<F&&, typename std::decay<F>::type&&>::value && std::is_move_constructible<typename std::decay<F>::type>::value> {};

template <typename F, typename P>
class scope_guard {
  using A = typename std::decay<F>::type;

  static_assert(is_noarg_returns_void_action<A&>::value,
                "scope_guard requires no-argument action, that returns void.");
  static_assert(std::is_same<P, on_exit_policy>::value || std::is_same<P, on_fail_policy>::value || std::is_same<P, on_success_policy>::value,
                "scope_guard requires on_exit_policy, on_fail_policy or on_success_policy.");
#if defined(SCOPE_GUARD_NO_THROW_ACTION)
  static_assert(is_nothrow_invocable_action<A&>::value,
                "scope_guard requires noexcept invocable action.");
#endif
#if defined(SCOPE_GUARD_NO_THROW_ACTION) || defined(SCOPE_GUARD_SUPPRESS_THROW_ACTION)
  static_assert(std::is_nothrow_destructible<A>::value,
                "scope_guard requires nothrow destructible action.");
#endif
#if defined(SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE)
  static_assert(std::is_nothrow_move_constructible<A>::value,
                "scope_guard requires nothrow move-constructible action.");
#endif

  P policy_;
  A action_;

  // Guards need automatic storage because the exception baseline belongs to the creating scope.
  void* operator new(std::size_t) = delete;
  void* operator new[](std::size_t) = delete;
  void operator delete(void*) = delete;
  void operator delete[](void*) = delete;

  scope_guard(A&& action, std::true_type) noexcept
      : policy_{true},
        action_(NEARGYE_SCOPE_GUARD_MOV(action)) {}

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  // GCC 9-10 and MSVC rethrow the wrong exception after a handler nested in a constructor try block, so suppress in a helper.
  static void handle_construction_failure(A& action) {
    NEARGYE_SCOPE_GUARD_TRY
      P::construction_failed(action);
    NEARGYE_SCOPE_GUARD_CATCH
  }

  // A failed action move still runs the action, except for scope_success.
  scope_guard(A&& action, std::false_type) try
      : policy_{true},
        action_(NEARGYE_SCOPE_GUARD_MOV(action)) {
  } catch (...) {
    handle_construction_failure(action);
  }
#else
  scope_guard(A&& action, std::false_type)
      : policy_{true},
        action_(NEARGYE_SCOPE_GUARD_MOV(action)) {}
#endif

 public:
  scope_guard() = delete;
  scope_guard(const scope_guard&) = delete;
  scope_guard& operator=(const scope_guard&) = delete;
  scope_guard& operator=(scope_guard&&) = delete;

  // The new guard stays inactive until the action move succeeds, so a failed move leaves only the source active.
  scope_guard(scope_guard&& other) noexcept(std::is_nothrow_move_constructible<A>::value)
      : policy_{false},
        action_(NEARGYE_SCOPE_GUARD_MOV(other.action_)) {
    policy_ = NEARGYE_SCOPE_GUARD_MOV(other.policy_);
    other.policy_.dismiss();
  }

  scope_guard(const A& action) = delete;
  scope_guard(A& action) = delete;

  explicit scope_guard(A&& action) noexcept(std::is_nothrow_move_constructible<A>::value)
      : scope_guard(NEARGYE_SCOPE_GUARD_MOV(action), typename std::is_nothrow_move_constructible<A>::type{}) {}

  void dismiss() noexcept {
    policy_.dismiss();
  }

  ~scope_guard() NEARGYE_SCOPE_GUARD_NOEXCEPT(is_nothrow_invocable_action<A&>::value && std::is_nothrow_destructible<A>::value) {
    if (policy_.should_execute()) {
      NEARGYE_SCOPE_GUARD_TRY
        action_();
      NEARGYE_SCOPE_GUARD_CATCH
    }
  }
};

template <typename F>
using scope_exit = scope_guard<F, on_exit_policy>;

template <typename F, typename std::enable_if<is_noarg_returns_void_action<typename std::decay<F>::type&>::value, int>::type = 0>
NEARGYE_SCOPE_GUARD_NODISCARD scope_exit<F> make_scope_exit(F&& action) noexcept(noexcept(scope_exit<F>{NEARGYE_SCOPE_GUARD_FWD(action)})) {
  static_assert(is_storable_action<F>::value, "make_scope_exit requires a function or a movable rvalue action");
  return scope_exit<F>{NEARGYE_SCOPE_GUARD_FWD(action)};
}

template <typename F>
using scope_fail = scope_guard<F, on_fail_policy>;

template <typename F, typename std::enable_if<is_noarg_returns_void_action<typename std::decay<F>::type&>::value, int>::type = 0>
NEARGYE_SCOPE_GUARD_NODISCARD scope_fail<F> make_scope_fail(F&& action) noexcept(noexcept(scope_fail<F>{NEARGYE_SCOPE_GUARD_FWD(action)})) {
  static_assert(is_storable_action<F>::value, "make_scope_fail requires a function or a movable rvalue action");
  return scope_fail<F>{NEARGYE_SCOPE_GUARD_FWD(action)};
}

template <typename F>
using scope_success = scope_guard<F, on_success_policy>;

template <typename F, typename std::enable_if<is_noarg_returns_void_action<typename std::decay<F>::type&>::value, int>::type = 0>
NEARGYE_SCOPE_GUARD_NODISCARD scope_success<F> make_scope_success(F&& action) noexcept(noexcept(scope_success<F>{NEARGYE_SCOPE_GUARD_FWD(action)})) {
  static_assert(is_storable_action<F>::value, "make_scope_success requires a function or a movable rvalue action");
  return scope_success<F>{NEARGYE_SCOPE_GUARD_FWD(action)};
}

struct scope_exit_tag {};

template <typename F, typename std::enable_if<is_noarg_returns_void_action<typename std::decay<F>::type&>::value, int>::type = 0>
scope_exit<F> operator<<(scope_exit_tag, F&& action) noexcept(noexcept(scope_exit<F>{NEARGYE_SCOPE_GUARD_FWD(action)})) {
  return scope_exit<F>{NEARGYE_SCOPE_GUARD_FWD(action)};
}

struct scope_fail_tag {};

template <typename F, typename std::enable_if<is_noarg_returns_void_action<typename std::decay<F>::type&>::value, int>::type = 0>
scope_fail<F> operator<<(scope_fail_tag, F&& action) noexcept(noexcept(scope_fail<F>{NEARGYE_SCOPE_GUARD_FWD(action)})) {
  return scope_fail<F>{NEARGYE_SCOPE_GUARD_FWD(action)};
}

struct scope_success_tag {};

template <typename F, typename std::enable_if<is_noarg_returns_void_action<typename std::decay<F>::type&>::value, int>::type = 0>
scope_success<F> operator<<(scope_success_tag, F&& action) noexcept(noexcept(scope_success<F>{NEARGYE_SCOPE_GUARD_FWD(action)})) {
  return scope_success<F>{NEARGYE_SCOPE_GUARD_FWD(action)};
}

#undef NEARGYE_SCOPE_GUARD_MOV
#undef NEARGYE_SCOPE_GUARD_FWD
#undef NEARGYE_SCOPE_GUARD_NOEXCEPT
#undef NEARGYE_SCOPE_GUARD_TRY
#undef NEARGYE_SCOPE_GUARD_CATCH
#undef NEARGYE_SCOPE_GUARD_NODISCARD

} // namespace scope_guard::detail

using detail::make_scope_exit;
using detail::make_scope_fail;
using detail::make_scope_success;

} // namespace scope_guard

// NEARGYE_SCOPE_GUARD_MAYBE_UNUSED suppresses compiler warnings on unused entities, if any.
#if defined(__clang__) || defined(__GNUC__)
#  if __cplusplus >= 201703L
#    define NEARGYE_SCOPE_GUARD_MAYBE_UNUSED [[maybe_unused]]
#  else
#    define NEARGYE_SCOPE_GUARD_MAYBE_UNUSED __attribute__((__unused__))
#  endif
#elif defined(_MSC_VER) && _MSVC_LANG >= 201703L
#  define NEARGYE_SCOPE_GUARD_MAYBE_UNUSED [[maybe_unused]]
#else
#  define NEARGYE_SCOPE_GUARD_MAYBE_UNUSED
#endif

#define NEARGYE_SCOPE_GUARD_STR_CONCAT_(s1, s2) s1##s2
#define NEARGYE_SCOPE_GUARD_STR_CONCAT(s1, s2)  NEARGYE_SCOPE_GUARD_STR_CONCAT_(s1, s2)

#if defined(SCOPE_GUARD_NO_THROW_ACTION)
#  define NEARGYE_SCOPE_GUARD_ACTION [&]() noexcept -> void
#else
#  define NEARGYE_SCOPE_GUARD_ACTION [&]() -> void
#endif

#define NEARGYE_SCOPE_GUARD_MAKE_SCOPE_EXIT    ::scope_guard::detail::scope_exit_tag{}    << NEARGYE_SCOPE_GUARD_ACTION
#define NEARGYE_SCOPE_GUARD_MAKE_SCOPE_FAIL    ::scope_guard::detail::scope_fail_tag{}    << NEARGYE_SCOPE_GUARD_ACTION
#define NEARGYE_SCOPE_GUARD_MAKE_SCOPE_SUCCESS ::scope_guard::detail::scope_success_tag{} << NEARGYE_SCOPE_GUARD_ACTION

// Clang 22 warns about __COUNTER__ with -pedantic. This silences it for guards that are not inside another macro's arguments.
#if defined(__clang__) && __clang_major__ == 22
#  define NEARGYE_SCOPE_GUARD_COUNTER_PUSH _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wc2y-extensions\"")
#  define NEARGYE_SCOPE_GUARD_COUNTER_POP  _Pragma("clang diagnostic pop")
#else
#  define NEARGYE_SCOPE_GUARD_COUNTER_PUSH
#  define NEARGYE_SCOPE_GUARD_COUNTER_POP
#endif

// Both loops share one flag and run once, so break and continue stay inside this block.
#define NEARGYE_SCOPE_GUARD_WITH_(i, j, ...) for (bool i = true; i; i = false) for (auto j = __VA_ARGS__; i; i = false)
#define NEARGYE_SCOPE_GUARD_WITH(...)        NEARGYE_SCOPE_GUARD_COUNTER_PUSH NEARGYE_SCOPE_GUARD_WITH_(NEARGYE_SCOPE_GUARD_STR_CONCAT(NEARGYE_SCOPE_GUARD_FLAG_, NEARGYE_SCOPE_GUARD_COUNTER), NEARGYE_SCOPE_GUARD_STR_CONCAT(NEARGYE_SCOPE_GUARD_OBJECT_, NEARGYE_SCOPE_GUARD_COUNTER), __VA_ARGS__) NEARGYE_SCOPE_GUARD_COUNTER_POP

// SCOPE_EXIT executes the action on scope exit.
#define MAKE_SCOPE_EXIT(name)  auto name = NEARGYE_SCOPE_GUARD_MAKE_SCOPE_EXIT
#define SCOPE_EXIT             NEARGYE_SCOPE_GUARD_COUNTER_PUSH NEARGYE_SCOPE_GUARD_MAYBE_UNUSED const MAKE_SCOPE_EXIT(NEARGYE_SCOPE_GUARD_STR_CONCAT(NEARGYE_SCOPE_GUARD_SCOPE_EXIT_, NEARGYE_SCOPE_GUARD_COUNTER)) NEARGYE_SCOPE_GUARD_COUNTER_POP
#define WITH_SCOPE_EXIT(...)   NEARGYE_SCOPE_GUARD_WITH(NEARGYE_SCOPE_GUARD_MAKE_SCOPE_EXIT{ __VA_ARGS__ })

// SCOPE_FAIL executes the action when the scope is left during exception unwinding.
#define MAKE_SCOPE_FAIL(name)  auto name = NEARGYE_SCOPE_GUARD_MAKE_SCOPE_FAIL
#define SCOPE_FAIL             NEARGYE_SCOPE_GUARD_COUNTER_PUSH NEARGYE_SCOPE_GUARD_MAYBE_UNUSED const MAKE_SCOPE_FAIL(NEARGYE_SCOPE_GUARD_STR_CONCAT(NEARGYE_SCOPE_GUARD_SCOPE_FAIL_, NEARGYE_SCOPE_GUARD_COUNTER)) NEARGYE_SCOPE_GUARD_COUNTER_POP
#define WITH_SCOPE_FAIL(...)   NEARGYE_SCOPE_GUARD_WITH(NEARGYE_SCOPE_GUARD_MAKE_SCOPE_FAIL{ __VA_ARGS__ })

// SCOPE_SUCCESS executes the action when the scope is not left during exception unwinding.
#define MAKE_SCOPE_SUCCESS(name)  auto name = NEARGYE_SCOPE_GUARD_MAKE_SCOPE_SUCCESS
#define SCOPE_SUCCESS             NEARGYE_SCOPE_GUARD_COUNTER_PUSH NEARGYE_SCOPE_GUARD_MAYBE_UNUSED const MAKE_SCOPE_SUCCESS(NEARGYE_SCOPE_GUARD_STR_CONCAT(NEARGYE_SCOPE_GUARD_SCOPE_SUCCESS_, NEARGYE_SCOPE_GUARD_COUNTER)) NEARGYE_SCOPE_GUARD_COUNTER_POP
#define WITH_SCOPE_SUCCESS(...)   NEARGYE_SCOPE_GUARD_WITH(NEARGYE_SCOPE_GUARD_MAKE_SCOPE_SUCCESS{ __VA_ARGS__ })

// DEFER executes the action on scope exit.
#define MAKE_DEFER(name)  MAKE_SCOPE_EXIT(name)
#define DEFER             SCOPE_EXIT
#define WITH_DEFER(...)   WITH_SCOPE_EXIT(__VA_ARGS__)

// Must stay last: system_header marks the rest of the file, so Clang 23+ skips __COUNTER__ warnings from this macro.
#if defined(__clang__) && __clang_major__ >= 23 && __INCLUDE_LEVEL__ > 0
#  pragma clang system_header
#endif
#define NEARGYE_SCOPE_GUARD_COUNTER __COUNTER__

#endif // NEARGYE_SCOPE_GUARD_HPP
