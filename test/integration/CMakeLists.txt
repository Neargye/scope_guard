function(add_integration_test name script)
    add_test(NAME ${PROJECT_NAME}-${name}.t COMMAND ${CMAKE_COMMAND}
             "-DSCOPE_GUARD_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
             "-DSCOPE_GUARD_INTEGRATION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
             "-DSCOPE_GUARD_INTEGRATION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}"
             "-DSCOPE_GUARD_GENERATOR=${CMAKE_GENERATOR}"
             "-DSCOPE_GUARD_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
             "-DSCOPE_GUARD_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
             "-DSCOPE_GUARD_BUILD_CONFIG=$<CONFIG>"
             "-DSCOPE_GUARD_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}"
             ${ARGN} -P "${CMAKE_CURRENT_SOURCE_DIR}/${script}")
endfunction()

add_integration_test(install-consumer-default run_consumer.cmake -DSCOPE_GUARD_INTEGRATION_MODE=install -DSCOPE_GUARD_INSTALL_INCLUDEDIR=include)

add_integration_test(install-consumer-custom-includedir run_consumer.cmake -DSCOPE_GUARD_INTEGRATION_MODE=install -DSCOPE_GUARD_INSTALL_INCLUDEDIR=custom_headers)

add_integration_test(subproject-consumer run_consumer.cmake -DSCOPE_GUARD_INTEGRATION_MODE=subproject)
