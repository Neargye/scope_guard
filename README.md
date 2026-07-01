[![GitHub Releases](https://img.shields.io/github/release/Neargye/scope_guard.svg)](https://github.com/Neargye/scope_guard/releases)
[![License](https://img.shields.io/github/license/Neargye/scope_guard.svg)](LICENSE)

# Scope Guard & Defer C++

Scope Guard statement invokes a function with deferred execution when the surrounding scope is left:

* scope_exit - executes the action on scope exit.

* scope_fail - executes the action on scope exit if the scope is left during exception unwinding.

* scope_success - executes the action on scope exit if the scope is not left during exception unwinding.

Normal C++ control flow such as `return`, `break`, `continue`, and exceptions does not bypass guard destruction; actions run according to the selected guard policy. Hence, Scope Guard statement can be used to perform manual resource management, such as file descriptors closing, and to perform actions even if an error occurs.

## Features

* C++11
* Header-only
* Dependency-free
* Thin callback wrapping, no added std::function or virtual table penalties
* No implicitly ignored return, callbacks must return void
* Defer or Scope Guard syntax and "With" syntax

## [Examples](example)

* [Scope Guard on exit](example/scope_exit_example.cpp)

  ```cpp
  #include <scope_guard.hpp>

  std::fstream file("test.txt");
  SCOPE_EXIT{ file.close(); }; // File closes when the enclosing scope is left.
  ```

* [Scope Guard on fail](example/scope_fail_example.cpp)

  ```cpp
  persons.push_back(person); // Add the person to db.
  SCOPE_FAIL{ persons.pop_back(); }; // If errors occur, we should roll back.
  ```

* [Scope Guard on success](example/scope_success_example.cpp)

  ```cpp
  person = new Person{/*...*/};
  // ...
  SCOPE_SUCCESS{ persons.push_back(person); }; // If no errors occur, we should add the person to db.
  ```

* Custom Scope Guard

  ```cpp
  persons.push_back(person); // Add the person to db.

  MAKE_SCOPE_EXIT(scope_exit) { // The action is executed when the enclosing scope is left.
    persons.pop_back(); // If the db insertion fails, roll back.
  };
  // MAKE_SCOPE_EXIT(name) {action} - macro is used to create a new scope_exit object.
  scope_exit.dismiss(); // An exception was not thrown, so don't execute the scope_exit.
  ```

  ```cpp
  persons.push_back(person); // Add the person to db.

  auto scope_exit = scope_guard::make_scope_exit([]() { persons.pop_back(); });
  // make_scope_exit(F&& action) - function is used to create a new scope_exit object. It accepts an rvalue callable: a lambda expression, an rvalue std::function<void()>, an rvalue functor, or a void(*)() function pointer. Lvalue callables are intentionally rejected; use std::move if needed.
  // ...
  scope_exit.dismiss(); // An exception was not thrown, so don't execute the scope_exit.
  ```

* With Scope Guard

  ```cpp
  std::fstream file("test.txt");
  WITH_SCOPE_EXIT({ file.close(); }) { // File closes when the enclosing with scope is left.
    // ...
  }
  ```

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

Guards returned by `scope_guard::make_scope_exit`, `scope_guard::make_scope_fail`, `scope_guard::make_scope_success`, and guards created by macros implement the scope_guard interface.

* `dismiss()` - disables executing the action on scope exit.

#### Throwable settings

* `SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE` - define this to require a nothrow move-constructible action.

* `SCOPE_GUARD_MAY_THROW_ACTION` - define this to allow the action to throw exceptions.

* `SCOPE_GUARD_NO_THROW_ACTION` - define this to require a noexcept action.

* `SCOPE_GUARD_SUPPRESS_THROW_ACTION` - define this to suppress exceptions thrown by the action.

* By default, `SCOPE_GUARD_MAY_THROW_ACTION` is used. If an action throws while another exception is being unwound, the program may terminate. Define `SCOPE_GUARD_NO_THROW_ACTION` or `SCOPE_GUARD_SUPPRESS_THROW_ACTION` for cleanup paths that must not throw.

* `SCOPE_GUARD_CATCH_HANDLER` - define this to add an exception handler statement. If `SCOPE_GUARD_SUPPRESS_THROW_ACTION` is not defined, it does nothing.

  ```cpp
  #define SCOPE_GUARD_SUPPRESS_THROW_ACTION
  #define SCOPE_GUARD_CATCH_HANDLER /* log cleanup failure */ ;
  #include <scope_guard.hpp>
  ```

### Remarks

* `make_scope_exit`, `make_scope_fail`, and `make_scope_success` only accept rvalue callables. Lvalue callables are intentionally rejected to prevent dangling references. Pass a temporary or use `std::move`:

  ```cpp
  auto action = [&]() { /* cleanup */ };
  auto guard = scope_guard::make_scope_exit(std::move(action)); // OK
  // auto guard = scope_guard::make_scope_exit(action); // compile error
  ```

* If multiple Scope Guard statements appear in the same scope, the order they appear is the reverse of the order they are executed.

  ```cpp
  void f() {
    SCOPE_EXIT{ std::cout << "First" << std::endl; };
    SCOPE_EXIT{ std::cout << "Second" << std::endl; };
    SCOPE_EXIT{ std::cout << "Third" << std::endl; };
    ... // Other code.
    // Prints "Third".
    // Prints "Second".
    // Prints "First".
  }
  ```

## Integration

For manual integration, add the required file [scope_guard.hpp](include/scope_guard.hpp).

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
