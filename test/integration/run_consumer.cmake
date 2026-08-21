function(run_checked)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result)
        message(FATAL_ERROR "Command failed (${result}): ${ARGN}\n${output}\n${error}")
    endif()
endfunction()

file(REMOVE_RECURSE "${SCOPE_GUARD_INTEGRATION_BINARY_DIR}")

set(generator_args -G "${SCOPE_GUARD_GENERATOR}")
if(SCOPE_GUARD_GENERATOR_PLATFORM)
    list(APPEND generator_args -A "${SCOPE_GUARD_GENERATOR_PLATFORM}")
endif()
if(SCOPE_GUARD_GENERATOR_TOOLSET)
    list(APPEND generator_args -T "${SCOPE_GUARD_GENERATOR_TOOLSET}")
endif()

set(config_args)
set(build_config_args)
if(SCOPE_GUARD_BUILD_CONFIG)
    list(APPEND config_args "-DCMAKE_BUILD_TYPE=${SCOPE_GUARD_BUILD_CONFIG}")
    list(APPEND build_config_args --config "${SCOPE_GUARD_BUILD_CONFIG}")
endif()

set(consumer_build_dir "${SCOPE_GUARD_INTEGRATION_BINARY_DIR}/consumer-build")

if(SCOPE_GUARD_INTEGRATION_MODE STREQUAL "install")
    set(package_build_dir "${SCOPE_GUARD_INTEGRATION_BINARY_DIR}/package-build")
    set(install_prefix "${SCOPE_GUARD_INTEGRATION_BINARY_DIR}/install")

    run_checked("${CMAKE_COMMAND}" -S "${SCOPE_GUARD_SOURCE_DIR}" -B "${package_build_dir}" ${generator_args} ${config_args} -DSCOPE_GUARD_OPT_BUILD_EXAMPLES=OFF -DSCOPE_GUARD_OPT_BUILD_TESTS=OFF -DSCOPE_GUARD_OPT_INSTALL=ON "-DCMAKE_INSTALL_INCLUDEDIR=${SCOPE_GUARD_INSTALL_INCLUDEDIR}" "-DCMAKE_INSTALL_DATADIR=${SCOPE_GUARD_INSTALL_DATADIR}")

    run_checked("${CMAKE_COMMAND}" --install "${package_build_dir}" --prefix "${install_prefix}" ${build_config_args})

    set(installed_license "${install_prefix}/${SCOPE_GUARD_INSTALL_DATADIR}/scope_guard/LICENSE")
    if(NOT EXISTS "${installed_license}")
        message(FATAL_ERROR "Installed LICENSE was not found: ${installed_license}")
    endif()

    set(consumer_args "-DCMAKE_PREFIX_PATH=${install_prefix}")
elseif(SCOPE_GUARD_INTEGRATION_MODE STREQUAL "subproject")
    set(consumer_args "-DSCOPE_GUARD_SOURCE_DIR=${SCOPE_GUARD_SOURCE_DIR}")
else()
    message(FATAL_ERROR "Unknown SCOPE_GUARD_INTEGRATION_MODE: ${SCOPE_GUARD_INTEGRATION_MODE}")
endif()

run_checked("${CMAKE_COMMAND}" -S "${SCOPE_GUARD_INTEGRATION_SOURCE_DIR}/consumer" -B "${consumer_build_dir}" ${generator_args} ${config_args} "${consumer_args}")

run_checked("${CMAKE_COMMAND}" --build "${consumer_build_dir}" ${build_config_args})
