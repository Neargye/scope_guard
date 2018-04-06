# scope_guard c++11

C++ alternative to [defer](https://golang.org/ref/spec#Defer_statements) operator in [go](https://en.wikipedia.org/wiki/Go_(programming_language)).

Branch | Linux/OSX | Windows
-------|-----------|---------
master |[![Build Status](https://travis-ci.org/Neargye/scope_guard.svg?branch=master)](https://travis-ci.org/Neargye/scope_guard)|[![Build status](https://ci.appveyor.com/api/projects/status/yi394vgtwd0i2kco/branch/master?svg=true)](https://ci.appveyor.com/project/Neargye/scope-guard/branch/master)

## Features

* C++11
* Header-only
* Dependency-free

## Example

```cpp
#include <scope_guard.hpp>
#include <iostream>
#include <fstream>

int main() {
  DEFER_TYPE custom_defer = MAKE_DEFER{ std::cout << "custom defer" << std::endl; };
  DEFER_TYPE custom_defer_not_call = MAKE_DEFER{ std::cout << "not call" << std::endl; };

  std::cout << "{ ";
  for(int i = 0; i < 4; ++i)
    DEFER{ std::cout << i << " "; };
  std::cout << "}" << std::endl;

  std::ofstream file;
  file.open("test.txt", std::fstream::out | std::ofstream::trunc);
  DEFER{
         file.close();
         std::cout << "close file" << std::endl;
        };

  file << "example" << std::endl;
  std::cout << "write to file" << std::endl;

  custom_defer_not_call.Dismiss();

  // prints "{ 0 1 2 3 }"
  // prints "write to file"
  // prints "close file"
  // prints "custom defer"

  return 0;
}
```

## License MIT