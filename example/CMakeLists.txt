include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCE_EXAMPLE
    example.cpp
    ${CMAKE_SOURCE_DIR}/include/scope_guard.hpp)

add_executable(${PROJECT_NAME}_example ${SOURCE_EXAMPLE})
