[![GitHub Releases](https://img.shields.io/github/release/Neargye/scope_guard.svg)](https://github.com/Neargye/scope_guard/releases)
[![License](https://img.shields.io/github/license/Neargye/scope_guard.svg)](LICENSE)

# Scope Guard & Defer C++

A scope guard runs a deferred action when its scope is left:

* `scope_exit` - executes the action on scope exit.

* `scope_fail` - executes the action if an exception leaves the scope.

* `scope_success` - executes the action if the scope exits normally.

Normal C++ control flow, including `return`, `break`, `continue`, and exceptions, still destroys the guard. This makes scope guards useful for cleanup and rollback across different exit paths.

## Features

* C++11
* Header-only
* Dependency-free
* Thin callback wrapping, no added std::function or virtual table penalties
* No silently ignored return values; callbacks must return `void`
* Defer or Scope Guard syntax and "With" syntax

## [Examples](example)

* [Scope Guard on exit and defer](example/scope_exit_example.cpp)

  ```cpp
  #include <fstream>
  #include <scope_guard.hpp>

  std::fstream file("test.txt");
  SCOPE_EXIT{ file.close(); }; // File closes when the enclosing scope is left.
  ```

* [Scope Guard on fail](example/scope_fail_example.cpp)

  ```cpp
  persons.push_back(person);
  SCOPE_FAIL{ persons.pop_back(); }; // Roll back if a later operation throws.
  ```

* [Scope Guard on success](example/scope_success_example.cpp)

  ```cpp
  Person person{/*...*/};
  // ...
  SCOPE_SUCCESS{ persons.push_back(person); }; // Add the person if the scope exits normally.
  ```

* Custom Scope Guard

  ```cpp
  persons.push_back(person);

  MAKE_SCOPE_EXIT(rollback) {
    persons.pop_back();
  };
  // ...
  rollback.dismiss(); // Commit the change and cancel the rollback.
  ```

  ```cpp
  persons.push_back(person);

  auto rollback = scope_guard::make_scope_exit([&]() { persons.pop_back(); });
  // ...
  rollback.dismiss(); // Commit the change and cancel the rollback.
  ```

* With Scope Guard

  ```cpp
  std::fstream file("test.txt");
  WITH_SCOPE_EXIT({ file.close(); }) { // File closes when this block is left.
    // ...
  }
  ```

  > Inside `WITH_SCOPE_*`, `break` and `continue` affect only this block, not an enclosing loop. Use a regular `SCOPE_*` guard to control an outer loop.

## Synopsis

### Reference

#### scope_exit

* `scope_guard::make_scope_exit(F&& action);` - returns a scope_exit guard with the action.
* `SCOPE_EXIT{action};` - macro for creating scope_exit with the action.
* `MAKE_SCOPE_EXIT(name) {action};` - macro for creating named scope_exit with the action.
* `WITH_SCOPE_EXIT({action}) {/*...*/}` - macro for creating a scope with scope_exit with the action.

#### scope_fail

* `scope_guard::make_scope_fail(F&& action);` - returns a scope_fail guard with the action.
* `SCOPE_FAIL{action};` - macro for creating scope_fail with the action.
* `MAKE_SCOPE_FAIL(name) {action};` - macro for creating named scope_fail with the action.
* `WITH_SCOPE_FAIL({action}) {/*...*/}` - macro for creating a scope with scope_fail with the action.

#### scope_success

* `scope_guard::make_scope_success(F&& action);` - returns a scope_success guard with the action.
* `SCOPE_SUCCESS{action};` - macro for creating scope_success with the action.
* `MAKE_SCOPE_SUCCESS(name) {action};` - macro for creating named scope_success with the action.
* `WITH_SCOPE_SUCCESS({action}) {/*...*/}` - macro for creating a scope with scope_success with the action.

#### defer

* `DEFER{action};` - macro for creating defer with the action.
* `MAKE_DEFER(name) {action};` - macro for creating named defer with the action.
* `WITH_DEFER({action}) {/*...*/}` - macro for creating a scope with defer with the action.

### Interface of scope_guard

Named guards provide `dismiss()`, which disables the action.

Guards can be move-constructed, but cannot be copied or assigned. Moving transfers responsibility for executing the action.

#### Exception settings

* `SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE` - requires a nothrow move-constructible action. It can be combined with any action policy.

* `SCOPE_GUARD_MAY_THROW_ACTION` - allows action exceptions to propagate.

* `SCOPE_GUARD_NO_THROW_ACTION` - requires `noexcept` callables and declares macro-generated actions `noexcept`.

* `SCOPE_GUARD_SUPPRESS_THROW_ACTION` - suppresses exceptions thrown by the action.

* By default, `SCOPE_GUARD_MAY_THROW_ACTION` is used. Action exceptions propagate normally. If an exception escapes an action during stack unwinding, the program terminates. Use `SCOPE_GUARD_NO_THROW_ACTION` or `SCOPE_GUARD_SUPPRESS_THROW_ACTION` for cleanup paths that must not throw.

* `SCOPE_GUARD_CATCH_HANDLER` - a non-throwing statement run when an action exception is caught. It is ignored unless `SCOPE_GUARD_SUPPRESS_THROW_ACTION` is defined.

  ```cpp
  #define SCOPE_GUARD_SUPPRESS_THROW_ACTION
  #define SCOPE_GUARD_CATCH_HANDLER /* log cleanup failure */ ;
  #include <scope_guard.hpp>
  ```

Define exception settings consistently in every translation unit before including `scope_guard.hpp`. `SCOPE_GUARD_CATCH_HANDLER` must not throw.

### Remarks

* Factories accept only rvalue callables and store them by value. Pass a temporary or use `std::move`:

  ```cpp
  auto action = [&]() { /* cleanup */ };
  auto guard = scope_guard::make_scope_exit(std::move(action)); // OK
  // auto guard = scope_guard::make_scope_exit(action); // compile error
  ```

* Actions take no arguments and must return `void`.

* Macro-generated actions use `[&]` lambda capture. Use a factory function with an explicit lambda capture when different ownership is required.

* Guards execute in reverse construction order.

## Integration

`SCOPE_GUARD_OPT_BUILD_EXAMPLES`, `SCOPE_GUARD_OPT_BUILD_TESTS`, and `SCOPE_GUARD_OPT_INSTALL` default to `ON` when scope_guard is the top-level project and `OFF` when it is a subproject.

For manual integration, copy [scope_guard.hpp](include/scope_guard.hpp) into your project.

For CMake integration, add this project as a subdirectory and link the interface target:

```cmake
add_subdirectory(scope_guard)
target_link_libraries(your_target PRIVATE scope_guard::scope_guard)
```

If scope_guard is installed as a CMake package:

```cmake
find_package(scope_guard CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE scope_guard::scope_guard)
```

## References

* [Andrei Alexandrescu "Systematic Error Handling in C++"](https://www.youtube.com/watch?v=kaI4R0Ng4E8)
* [Andrei Alexandrescu "Declarative Control Flow"](https://youtu.be/WjTrfoiB0MQ)

## Licensed under the [MIT License](LICENSE)
