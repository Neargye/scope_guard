# scope_guard c++

```text
     _       __             _____
    | |     / _|           / ____|_     _
  __| | ___| |_ ___ _ __  | |   _| |_ _| |_
 / _` |/ _ \  _/ _ \ '__| | |  |_   _|_   _|
| (_| |  __/ ||  __/ |    | |____|_|   |_|
 \__,_|\___|_| \___|_|     \_____|
```

Branch | Linux/OSX | Windows
-------|-----------|---------
master |[![Build Status](https://travis-ci.org/Neargye/scope_guard.svg?branch=master)](https://travis-ci.org/Neargye/scope_guard)|[![Build status](https://ci.appveyor.com/api/projects/status/yi394vgtwd0i2kco/branch/master?svg=true)](https://ci.appveyor.com/project/Neargye/scope-guard/branch/master)

C++ alternative to [defer](https://golang.org/ref/spec#Defer_statements) operator in [go](https://en.wikipedia.org/wiki/Go_(programming_language)).

## Features

* Simple syntax
* C++11
* Header-only
* Dependency-free

## [Example](example/example.cpp) & Key Use Cases

```cpp
std::ofstream file("test.txt");
DEFER{ file.close(); }; // File close when the enclosing scope exits.
```

```cpp
bool commit = false;
persons.push_back(person); // Add the person to db.
DEFER{ // Following block is executed when the enclosing scope exits.
  if(!commit)
    persons.pop_back(); // If the db insertion that follows fails, we should rollback.
};
// ...
commit = true; // An exception was not thrown, so don't execute the defer.
```

## Integration

You need to add the single required file [scope_guard.hpp](include/scope_guard.hpp), and the necessary switches to enable C++11.

## Compiler compatibility

* GCC
* Clang
* MSVC

## References

[Andrei Alexandrescu “Declarative Control Flow"](https://github.com/CppCon/CppCon2015/blob/master/Presentations/Declarative%20Control%20Flow/Declarative%20Control%20Flow%20-%20Andrei%20Alexandrescu%20-%20CppCon%202015.pdf)

## Licensed under the [MIT License](LICENSE)