[![GitHub Releases](https://img.shields.io/github/release/Neargye/scope_guard.svg)](https://github.com/Neargye/scope_guard/releases)
[![License](https://img.shields.io/github/license/Neargye/scope_guard.svg)](LICENSE)

# Scope Guard & Defer C++

A header-only C++11 library for cleanup and rollback when leaving a scope. No dependencies.

## Usage

```cpp
#include <cstdio>
#include <scope_guard.hpp>

void read_file() {
  auto* file = std::fopen("test.txt", "r");
  if (!file) {
    return;
  }
  SCOPE_EXIT { std::fclose(file); };
  // Use file here. It is closed on scope exit.
}
```

| When the action runs | Macro | Factory |
|---|---|---|
| On any scope exit | `SCOPE_EXIT { action; };` | `scope_guard::make_scope_exit(action)` |
| When an exception leaves the scope | `SCOPE_FAIL { action; };` | `scope_guard::make_scope_fail(action)` |
| When the scope exits normally | `SCOPE_SUCCESS { action; };` | `scope_guard::make_scope_success(action)` |

`return`, `break` and `continue` count as normal exits. Exceptions caught within the scope do not trigger `SCOPE_FAIL`.
Failure means the uncaught exception count has increased since guard construction. The count is per thread, so do not keep `SCOPE_FAIL` or `SCOPE_SUCCESS` guards across `co_await` or move them to another thread.

Guards run in reverse order of construction. Macro actions capture by reference (`[&]`). Referenced objects must outlive the guard.

### Named guards and cancellation

Use `MAKE_SCOPE_EXIT(name)`, `MAKE_SCOPE_FAIL(name)` or `MAKE_SCOPE_SUCCESS(name)` for a named guard. Cancel its action with `dismiss()`:

```cpp
persons.push_back(person);
MAKE_SCOPE_EXIT(rollback) { persons.pop_back(); };
// ...
rollback.dismiss(); // Keep the change.
```

Successful move construction transfers responsibility for the action. Guards cannot be copied or assigned.

### Factories

```cpp
auto guard = scope_guard::make_scope_exit([&]() { std::fclose(file); });
```

Actions must be movable and callable as lvalues with no arguments and a `void` return. Pass a function name, a non-const temporary or `std::move(action)` to store it by value. Use `std::ref(action)` from `<functional>` to keep a reference. Keep the returned guard alive until cleanup.

### Guards for a block

`WITH_SCOPE_EXIT`, `WITH_SCOPE_FAIL` and `WITH_SCOPE_SUCCESS` guard the following block:

```cpp
WITH_SCOPE_EXIT({ std::fclose(file); }) {
  // Use file here. It is closed on block exit.
}
```

Inside `WITH_SCOPE_*`, `break` and `continue` affect this block, not an enclosing loop.

`DEFER`, `MAKE_DEFER(name)` and `WITH_DEFER` alias the corresponding exit macros.

More examples: [exit/defer](example/scope_exit_example.cpp), [failure](example/scope_fail_example.cpp), [success](example/scope_success_example.cpp).

## Exception settings

Define settings before including `scope_guard.hpp`, consistently in every translation unit. Choose at most one action policy:

| Macro | Behavior |
|---|---|
| `SCOPE_GUARD_MAY_THROW_ACTION` (default) | Exceptions from actions propagate. |
| `SCOPE_GUARD_NO_THROW_ACTION` | Requires `noexcept` actions. Macro lambdas are declared `noexcept`. |
| `SCOPE_GUARD_SUPPRESS_THROW_ACTION` | Catches action invocation exceptions when C++ exceptions are enabled. |

With the default policy, an action that throws during stack unwinding terminates the program.

Both non-default policies require non-throwing action destructors. Suppression does not cover construction or move exceptions.

Before C++17, `noexcept` is not part of a function pointer type. With `SCOPE_GUARD_NO_THROW_ACTION`, wrap function names in a `noexcept` lambda.

If storing the action throws, `scope_exit` and `scope_fail` run it before the exception propagates. `scope_success` does not.

`SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE` additionally requires a nothrow move-constructible action with any policy.

Suppressed exceptions are ignored unless a non-throwing `SCOPE_GUARD_CATCH_HANDLER` statement is defined:

```cpp
void log_cleanup_failure() noexcept;
#define SCOPE_GUARD_SUPPRESS_THROW_ACTION
#define SCOPE_GUARD_CATCH_HANDLER log_cleanup_failure();
#include <scope_guard.hpp>
```

## Integration

Copy [scope_guard.hpp](include/scope_guard.hpp) into your project, or use CMake:

```cmake
add_subdirectory(scope_guard)
# Or, for an installed package:
# find_package(scope_guard CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE scope_guard::scope_guard)
```

CMake 3.15 or newer is required. `SCOPE_GUARD_OPT_BUILD_EXAMPLES`, `SCOPE_GUARD_OPT_BUILD_TESTS` and `SCOPE_GUARD_OPT_INSTALL` default to `ON` for standalone builds and `OFF` for subprojects.

## Compiler compatibility

Requires C++11 or newer with Clang 9+, GCC 9+ or Visual Studio 2019+ (v142).

On Clang 22 with `-pedantic`, unnamed guards inside another macro's arguments warn about `__COUNTER__`. Use a named guard, a factory or Clang 23 there.

## Licensed under the [MIT License](LICENSE)
