# Scope Guard & Defer C++

```text
  _____                         _____                     _    _____
 / ____|                       / ____|                   | |  / ____|_     _
| (___   ___ ___  _ __   ___  | |  __ _   _  __ _ _ __ __| | | |   _| |_ _| |_
 \___ \ / __/ _ \| '_ \ / _ \ | | |_ | | | |/ _` | '__/ _` | | |  |_   _|_   _|
 ____) | (_| (_) | |_) |  __/ | |__| | |_| | (_| | | | (_| | | |____|_|   |_|
|_____/ \___\___/| .__/ \___|  \_____|\__,_|\__,_|_|  \__,_|  \_____|
                 | |
                 |_|
```

Branch | Linux/OSX | Windows | License | Codacy
-------|-----------|---------|---------|-------
master |[![Build Status](https://travis-ci.org/Neargye/scope_guard.svg?branch=master)](https://travis-ci.org/Neargye/scope_guard)|[![Build status](https://ci.appveyor.com/api/projects/status/yi394vgtwd0i2kco/branch/master?svg=true)](https://ci.appveyor.com/project/Neargye/scope-guard/branch/master)|[![License](https://img.shields.io/github/license/Neargye/scope_guard.svg)](LICENSE)|[![Codacy Badge](https://api.codacy.com/project/badge/Grade/f5aa0553701f4f84bd51f2efda879972)](https://www.codacy.com/app/Neargye/scope_guard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Neargye/scope_guard&amp;utm_campaign=Badge_Grade)

Scope Guard statement invokes a function with deferred execution until surrounding function returns in cases:

* Surrounding function executed return statemeent
* This function reached the end of its body
* Erros occure

Program control transferring does not influence Scope Guard statement execution. Hence, Scope Guard statement can be used to perform manual resource management, such as file descriptors closing, and to perform actions even if an error occure.

## Features

* Simple syntax
* C++11
* Header-only
* Dependency-free
* Thin callback wrapping: no added std::function or virtual table penalties
* No implicitly ignored return, check callback return void
* Defer or Scope Guard syntax

## [Examples](example/example.cpp)

* Scope Guard syntax

```cpp
std::fstream file("test.txt");
SCOPE_EXIT{ file.close(); }; // File closes when exit the enclosing scope or errors occure.
```

* Custom Scope Guard

```cpp
persons.push_back(person); // Add the person to db.
MAKE_SCOPE_EXIT(scope_exit){ // Following block is executed when exit the enclosing scope or errors occure.
  persons.pop_back(); // If the db insertion fails, we should roll back.
};
// ...
scope_exit.Dismiss(); // An exception was not thrown, so don't execute the scope_exit.
```

* Defer syntax

```cpp
std::fstream file("test.txt");
DEFER{ file.close(); }; // File closes when exit the enclosing scope or errors occure.
```

* Custom Defer

```cpp
persons.push_back(person); // Add the person to db.
MAKE_DEFER(defer){ // Following block is executed when exit the enclosing scope or errors occure.
  persons.pop_back(); // If the db insertion fails, we should roll back.
};
// ...
defer.Dismiss(); // An exception was not thrown, so don't execute the defer.
```

## Remarks

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

You should add required file [scope_guard.hpp](include/scope_guard.hpp) and switch to C++11

## Compiler compatibility

* GCC
* Clang
* MSVC

## References

[Andrei Alexandrescu "Systematic Error Handling in C++"](https://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Andrei-Alexandrescu-Systematic-Error-Handling-in-C)

[Andrei Alexandrescu “Declarative Control Flow"](https://youtu.be/WjTrfoiB0MQ)

## Licensed under the [MIT License](LICENSE)
