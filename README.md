# scope_guard c++

```text
     _       __             _____
    | |     / _|           / ____|_     _
  __| | ___| |_ ___ _ __  | |   _| |_ _| |_
 / _` |/ _ \  _/ _ \ '__| | |  |_   _|_   _|
| (_| |  __/ ||  __/ |    | |____|_|   |_|
 \__,_|\___|_| \___|_|     \_____|
```

Branch | Linux/OSX | Windows | License | Codacy
-------|-----------|---------|---------|-------
master |[![Build Status](https://travis-ci.org/Neargye/scope_guard.svg?branch=master)](https://travis-ci.org/Neargye/scope_guard)|[![Build status](https://ci.appveyor.com/api/projects/status/yi394vgtwd0i2kco/branch/master?svg=true)](https://ci.appveyor.com/project/Neargye/scope-guard/branch/master)|[![License](https://img.shields.io/github/license/Neargye/scope_guard.svg)](LICENSE)|[![Codacy Badge](https://api.codacy.com/project/badge/Grade/f5aa0553701f4f84bd51f2efda879972)](https://www.codacy.com/app/Neargye/scope_guard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Neargye/scope_guard&amp;utm_campaign=Badge_Grade)

C++ alternative to [defer](https://golang.org/ref/spec#Defer_statements) operator in [go](https://en.wikipedia.org/wiki/Go_(programming_language)).

A "DEFER" statement invokes a function whose execution is deferred to the moment the surrounding function returns, either because the surrounding function executed a return statement, reached the end of its function body, or because the exception was thrown.

## Features

* Simple syntax
* C++11
* Header-only
* Dependency-free

## [Example](example/example.cpp) & Key Use Cases

* File close

```cpp
std::ofstream file("test.txt");
DEFER{ file.close(); }; // File close when the enclosing scope exits or an error is thrown.
```

* Delete dynamic array

```cpp
int* dynamic_array = new int[10];
DEFER{ delete[] dynamic_array; }; // Array delete when the enclosing scope exits or an error is thrown.
```

* Custom defer

```cpp
persons.push_back(person); // Add the person to db.
MAKE_DEFER(custom_defer){ // Following block is executed when the enclosing scope exits or an error is thrown.
  persons.pop_back(); // If the db insertion that follows fails, we should rollback.
};
// ...
custom_defer.Dismiss(); // An exception was not thrown, so don't execute the defer.
```

## Integration

You need to add the single required file [scope_guard.hpp](include/scope_guard.hpp), and the necessary switches to enable C++11.

## Compiler compatibility

* GCC
* Clang
* MSVC

## References

[Andrei Alexandrescu "Systematic Error Handling in C++"](https://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Andrei-Alexandrescu-Systematic-Error-Handling-in-C)

[Andrei Alexandrescu “Declarative Control Flow"](https://youtu.be/WjTrfoiB0MQ)

## Licensed under the [MIT License](LICENSE)
