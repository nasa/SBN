# Exercise the receive state machine with the cFE/OSAL UT stubs.
add_executable(sbn_tcp-recv-length-testrunner recv_length_test.c)
target_include_directories(sbn_tcp-recv-length-testrunner PRIVATE
    ${osal_MISSION_DIR}/ut_assert/inc
    ${SBN_APP_SOURCE_DIR}/fsw/platform_inc
)
target_compile_options(sbn_tcp-recv-length-testrunner PRIVATE ${UT_COVERAGE_COMPILE_FLAGS})
target_link_libraries(sbn_tcp-recv-length-testrunner
    ${UT_COVERAGE_LINK_FLAGS} ut_core_api_stubs ut_assert
)
add_test(sbn_tcp-recv-length sbn_tcp-recv-length-testrunner)
