cmake_minimum_required(VERSION 3.15)

project(scope_guard VERSION "0.9.4" LANGUAGES CXX)

include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}" IS_TOPLEVEL_PROJECT)

option(SCOPE_GUARD_OPT_BUILD_EXAMPLES "Build scope_guard examples" ${IS_TOPLEVEL_PROJECT})
option(SCOPE_GUARD_OPT_BUILD_TESTS "Build scope_guard tests" ${IS_TOPLEVEL_PROJECT})
option(SCOPE_GUARD_OPT_INSTALL "Generate scope_guard install rules" ${IS_TOPLEVEL_PROJECT})

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)

if(SCOPE_GUARD_OPT_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(SCOPE_GUARD_OPT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(SCOPE_GUARD_OPT_INSTALL)
    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY AnyNewerVersion ARCH_INDEPENDENT)

    install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Config)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

    install(EXPORT ${PROJECT_NAME}Config NAMESPACE ${PROJECT_NAME}:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})

    export(EXPORT ${PROJECT_NAME}Config NAMESPACE ${PROJECT_NAME}::)
endif()
