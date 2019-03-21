include(CheckCXXCompilerFlag)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(OPTIONS "")

if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(OPTIONS -Wall -Wextra -pedantic-errors)
    set(OPTIONS ${OPTIONS} -std=c++11)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(OPTIONS /W4)
    check_cxx_compiler_flag(/permissive HAS_PERMISSIVE_FLAG)
    if(HAS_PERMISSIVE_FLAG)
        set(OPTIONS ${OPTIONS} /permissive-)
    endif()
endif()

add_executable(scope_exit_example
                    ${CMAKE_SOURCE_DIR}/include/scope_guard.hpp
                    scope_exit_example.cpp)
target_compile_options(scope_exit_example PRIVATE ${OPTIONS})

add_executable(scope_fail_example
                    ${CMAKE_SOURCE_DIR}/include/scope_guard.hpp
                    scope_fail_example.cpp)
target_compile_options(scope_fail_example PRIVATE ${OPTIONS})

add_executable(scope_succes_example
                    ${CMAKE_SOURCE_DIR}/include/scope_guard.hpp
                    scope_succes_example.cpp)
target_compile_options(scope_succes_example PRIVATE ${OPTIONS})
