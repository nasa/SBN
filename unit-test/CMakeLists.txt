##################################################################
#
# Coverage Unit Test build recipe
#
# This CMake file contains the recipe for building the SBN unit tests.
# It is invoked from the parent directory when unit tests are enabled.
#
##################################################################

#
#
# NOTE on the subdirectory structures here:
#
# - "inc" provides local header files shared between the coveragetest,
#    wrappers, and overrides source code units
# - "coveragetest" contains source code for the actual unit test cases
#    The primary objective is to get line/path coverage on the FSW
#    code units.
# - "wrappers" contains wrappers for the FSW code.  The wrapper adds
#    any UT-specific scaffolding to facilitate the coverage test, and
#    includes the unmodified FSW source file.
#

set(UT_NAME sbn)

# Use the UT assert public API, and allow direct
# inclusion of source files that are normally private
include_directories(${PROJECT_SOURCE_DIR}/fsw/inc)
include_directories(${PROJECT_SOURCE_DIR}/fsw/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)


#set(UT_SRC_FILES)
#foreach(SRCFILE ${APP_SRC_FILES})
#    file(RELATIVE_PATH SRCPATH ${CMAKE_CURRENT_SOURCE_DIR} ${SBN_APP_SOURCE_DIR}/${SRCFILE})
#    list(APPEND UT_SRC_FILES ${SRCPATH})
#endforeach()
#
## Compile the source unit under test as a OBJECT
#add_library(ut_sbn_object OBJECT ${UT_SRC_FILES})
#
#target_compile_options(ut_sbn_object PRIVATE ${UT_COVERAGE_COMPILE_FLAGS})
#target_link_libraries(ut_sbn_object ut_core_api_stubs)

# Compile the source unit under test as a OBJECT
add_library(ut_sbn_common STATIC coveragetest/sbn_coveragetest_common.c)
target_link_libraries(ut_sbn_common ut_assert ut_core_api_stubs)

# Generate a dedicated "testrunner" executable that executes the tests for each FSW code unit
# Although sbn has only one source file, this is done in a loop such that
# the general pattern should work for several files as well.
foreach(SRCFILE ${APP_SRC_FILES})
    get_filename_component(UNITNAME "${SRCFILE}" NAME_WE)

    set(TESTNAME                "${UT_NAME}-${UNITNAME}")
    set(TESTCASE_SOURCE_FILE    "coveragetest/coveragetest_${UNITNAME}.c")

    # Get the base sourcefile name as a module name without path or the
    # extension, this will be used as the base name of the unit test file.
    get_filename_component(UNIT_NAME "${SRCFILE}" NAME_WE)

    # Create the coverage test executable
    # Note: This renders a target named "coverage-cf-${UNIT_NAME}-testrunner"
    add_cfe_coverage_test(sbn "${UNIT_NAME}" "${TESTCASE_SOURCE_FILE}" "${SBN_APP_SOURCE_DIR}/${SRCFILE}")

    # Link with the SBN internal stub library
    target_link_libraries(coverage-sbn-${UNIT_NAME}-testrunner ut_sbn_common ut_sbn_stubs)

endforeach()
