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

# Use the UT assert public headers 
include_directories(${osal_MISSION_DIR}/ut_assert/inc)

# Create a static library containing all stubs.
#
# There should be a 1:1 relationship between application source files
# and the stub files.  Each stub file should provide the same set of
# functions that the application source file provides.
add_library(ut_sbn_stubs STATIC 
    sbn_stubs.c
)
