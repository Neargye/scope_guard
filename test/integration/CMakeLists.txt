# Nested builds inherit the selected compiler, toolchain and flags.
set(integration_cache "${CMAKE_CURRENT_BINARY_DIR}/compiler-cache.cmake")
file(WRITE "${integration_cache}" "")
set(compiler_variables
    CMAKE_TOOLCHAIN_FILE CMAKE_CXX_COMPILER_TARGET CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN
    CMAKE_SYSROOT CMAKE_SYSROOT_COMPILE CMAKE_SYSROOT_LINK
    CMAKE_OSX_ARCHITECTURES CMAKE_OSX_SYSROOT CMAKE_OSX_DEPLOYMENT_TARGET
    CMAKE_TRY_COMPILE_CONFIGURATION CMAKE_MSVC_RUNTIME_LIBRARY
    CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS)
set(configurations Debug Release RelWithDebInfo MinSizeRel ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
foreach(configuration IN LISTS configurations)
    string(TOUPPER "${configuration}" configuration)
    list(APPEND compiler_variables CMAKE_CXX_FLAGS_${configuration} CMAKE_EXE_LINKER_FLAGS_${configuration})
endforeach()
list(REMOVE_DUPLICATES compiler_variables)
foreach(variable IN LISTS compiler_variables)
    if(DEFINED ${variable})
        set(delimiter "=")
        while("${${variable}}" MATCHES "\\]${delimiter}\\]")
            string(APPEND delimiter "=")
        endwhile()
        file(APPEND "${integration_cache}" "set(${variable} [${delimiter}[${${variable}}]${delimiter}] CACHE STRING \"\" FORCE)\n")
    endif()
endforeach()

function(add_integration_test name script)
    add_test(NAME ${PROJECT_NAME}-${name}.t COMMAND ${CMAKE_COMMAND}
             "-DSCOPE_GUARD_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
             "-DSCOPE_GUARD_INTEGRATION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
             "-DSCOPE_GUARD_INTEGRATION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}"
             "-DSCOPE_GUARD_GENERATOR=${CMAKE_GENERATOR}"
             "-DSCOPE_GUARD_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
             "-DSCOPE_GUARD_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
             "-DSCOPE_GUARD_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
             "-DSCOPE_GUARD_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
             "-DSCOPE_GUARD_COMPILER_CACHE=${integration_cache}"
             "-DSCOPE_GUARD_BUILD_CONFIG=$<CONFIG>"
             "-DSCOPE_GUARD_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}"
             ${ARGN} -P "${CMAKE_CURRENT_SOURCE_DIR}/${script}")
endfunction()

add_integration_test(install-consumer-default run_consumer.cmake -DSCOPE_GUARD_INTEGRATION_MODE=install -DSCOPE_GUARD_INSTALL_INCLUDEDIR=include)

add_integration_test(install-consumer-custom-includedir run_consumer.cmake -DSCOPE_GUARD_INTEGRATION_MODE=install -DSCOPE_GUARD_INSTALL_INCLUDEDIR=custom_headers)

add_integration_test(subproject-consumer run_consumer.cmake -DSCOPE_GUARD_INTEGRATION_MODE=subproject)

add_integration_test(subproject-defaults run_consumer.cmake -DSCOPE_GUARD_INTEGRATION_MODE=subproject-defaults)
