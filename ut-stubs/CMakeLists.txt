##################################################################
#
# SBN stub function build recipe
#
# This CMake file contains the recipe for building the stub function
# libraries that correlate with the library public API.  This supports
# unit testing of OTHER modules, where the test cases for those modules
# are linked with the stubs supplied here.
#
##################################################################

# Create a static library containing all stubs.
#
# There should be a 1:1 relationship between application source files
# and the stub files.  Each stub file should provide the same set of
# functions that the application source file provides.
add_library(ut_sbn_stubs STATIC
    sbn_stubs.c
)
target_link_libraries(ut_sbn_stubs ut_core_api_stubs ut_assert)
target_include_directories(ut_sbn_stubs PUBLIC $<TARGET_PROPERTY:sbn,INCLUDE_DIRECTORIES>)
