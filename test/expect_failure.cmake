if(BUILD_TARGET)
    set(config_args)
    if(BUILD_CONFIG)
        list(APPEND config_args --config "${BUILD_CONFIG}")
    endif()

    set(command "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --target "${BUILD_TARGET}" ${config_args})
    set(subject "Compilation of ${BUILD_TARGET}")
elseif(TEST_PROGRAM)
    set(command "${TEST_PROGRAM}")
    set(subject "Program ${TEST_PROGRAM}")
else()
    message(FATAL_ERROR "BUILD_TARGET or TEST_PROGRAM is required.")
endif()

execute_process(COMMAND ${command} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)

set(combined_output "${output}\n${error}")

if("${result}" STREQUAL "0")
    message(FATAL_ERROR "${subject} was expected to fail, but succeeded.")
endif()

if(EXPECTED_DIAGNOSTIC AND NOT "${combined_output}" MATCHES "${EXPECTED_DIAGNOSTIC}")
    message(FATAL_ERROR "${subject} failed without expected diagnostic '${EXPECTED_DIAGNOSTIC}'.\n${combined_output}")
endif()

if(EXPECTED_DIAGNOSTIC)
    message(STATUS "Observed expected failure: ${EXPECTED_DIAGNOSTIC}")
else()
    message(STATUS "Observed expected failure.")
endif()
