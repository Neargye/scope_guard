cmake_minimum_required(VERSION 3.15)

project(scope_guard_consumer LANGUAGES CXX)

if(SCOPE_GUARD_SOURCE_DIR)
    set(SCOPE_GUARD_OPT_BUILD_EXAMPLES ON CACHE BOOL "" FORCE)
    set(SCOPE_GUARD_OPT_BUILD_TESTS ON CACHE BOOL "" FORCE)
    set(SCOPE_GUARD_OPT_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${SCOPE_GUARD_SOURCE_DIR}" scope_guard)
else()
    find_package(scope_guard CONFIG REQUIRED)
endif()

add_executable(scope_guard_consumer main.cpp)
target_link_libraries(scope_guard_consumer PRIVATE scope_guard::scope_guard)
