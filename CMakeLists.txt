cmake_minimum_required(VERSION 3.6)

project(scope_guard VERSION "0.4.0" LANGUAGES CXX)

option(SCOPE_GUARD_OPT_BUILD_EXAMPLES "Build scope_guard examples" ON)
option(SCOPE_GUARD_OPT_BUILD_TESTS "Build and perform scope_guard tests" OFF)

if(SCOPE_GUARD_OPT_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(SCOPE_GUARD_OPT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
