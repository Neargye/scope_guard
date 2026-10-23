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
  // Use file here. It is closed when this scope is left.
}
```

| When the action runs | Macro | Factory |
|---|---|---|
| On any scope exit | `SCOPE_EXIT { action; };` | `scope_guard::make_scope_exit(action)` |
| When an exception leaves the scope | `SCOPE_FAIL { action; };` | `scope_guard::make_scope_fail(action)` |
| When the scope exits normally | `SCOPE_SUCCESS { action; };` | `scope_guard::make_scope_success(action)` |

`return`, `break` and `continue` count as normal exits. Guards run in reverse order. Macro actions capture by reference (`[&]`), so referenced objects must outlive the guard.

### Named guards and cancellation

Use `MAKE_SCOPE_EXIT(name)`, `MAKE_SCOPE_FAIL(name)` or `MAKE_SCOPE_SUCCESS(name)` to name a guard. Call `dismiss()` to cancel its action:

```cpp
persons.push_back(person);
MAKE_SCOPE_EXIT(rollback) { persons.pop_back(); };
// ...
rollback.dismiss(); // Keep the change.
```

Guards can be move-constructed, transferring the action to the new guard. They cannot be copied or assigned.

### Factories

```cpp
auto guard = scope_guard::make_scope_exit([&]() { std::fclose(file); });
```

Actions take no arguments and return `void`. Pass a function name, a temporary callable or `std::move(action)`. Use `std::ref(action)` to keep a reference.

### Guards for a block

`WITH_SCOPE_EXIT`, `WITH_SCOPE_FAIL` and `WITH_SCOPE_SUCCESS` apply their action to the following block:

```cpp
WITH_SCOPE_EXIT({ std::fclose(file); }) {
  // Use file here. It is closed when this block is left.
}
```

Inside `WITH_SCOPE_*`, `break` and `continue` affect this block, not an enclosing loop.

`DEFER`, `MAKE_DEFER(name)` and `WITH_DEFER` are aliases for the corresponding `SCOPE_EXIT` macros.

See the [exit/defer](example/scope_exit_example.cpp), [failure](example/scope_fail_example.cpp) and [success](example/scope_success_example.cpp) examples.

## Exception settings

Define settings before including `scope_guard.hpp`, consistently in every translation unit. Choose at most one action policy:

| Macro | Behavior |
|---|---|
| `SCOPE_GUARD_MAY_THROW_ACTION` (default) | Exceptions from actions propagate. |
| `SCOPE_GUARD_NO_THROW_ACTION` | Requires `noexcept` actions. |
| `SCOPE_GUARD_SUPPRESS_THROW_ACTION` | Catches exceptions thrown by actions. |

With the default policy, an action that throws during stack unwinding terminates the program.

`SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE` additionally requires a nothrow move-constructible action and can be combined with any policy.

With the suppress policy, `SCOPE_GUARD_CATCH_HANDLER` sets a non-throwing statement to run when an action throws:

```cpp
void log_cleanup_failure() noexcept;
#define SCOPE_GUARD_SUPPRESS_THROW_ACTION
#define SCOPE_GUARD_CATCH_HANDLER log_cleanup_failure();
#include <scope_guard.hpp>
```

## Integration

**Single header:** copy [scope_guard.hpp](include/scope_guard.hpp) into your project.

**CMake subdirectory:**

```cmake
add_subdirectory(scope_guard)
target_link_libraries(your_target PRIVATE scope_guard::scope_guard)
```

**Installed CMake package:**

```cmake
find_package(scope_guard CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE scope_guard::scope_guard)
```

CMake 3.15 or newer is required. `SCOPE_GUARD_OPT_BUILD_EXAMPLES`, `SCOPE_GUARD_OPT_BUILD_TESTS` and `SCOPE_GUARD_OPT_INSTALL` default to `ON` for standalone builds and `OFF` for subprojects.

## Compiler compatibility

Requires C++11 or newer with Clang 9+, GCC 9+ or Visual Studio 2019+ (v142).

On Clang 22 with `-pedantic`, unnamed guards inside another macro's arguments (for example a test assertion) warn about `__COUNTER__`. Use a named guard, a factory or Clang 23 there.

## Licensed under the [MIT License](LICENSE)
