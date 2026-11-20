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
    sbn_app_stubs.c
    sbn_app_handlers.c
    sbn_cmds_stubs.c
    sbn_connection_handlers.c
    sbn_connection_stubs.c
    sbn_dispatch_stubs.c
    sbn_encode_stubs.c
    sbn_filter_stubs.c
    sbn_global_appdata_stub.c
    sbn_global_workqueue_stub.c
    sbn_inbound_stubs.c
    sbn_module_stubs.c
    sbn_outbound_stubs.c
    sbn_pack_handlers.c
    sbn_pack_stubs.c
    sbn_resource_handlers.c
    sbn_resource_stubs.c
    sbn_subs_stubs.c
    sbn_worker_stubs.c
)
target_link_libraries(ut_sbn_stubs ut_core_api_stubs ut_assert)
target_include_directories(ut_sbn_stubs PUBLIC $<TARGET_PROPERTY:sbn,INCLUDE_DIRECTORIES>)
target_include_directories(ut_sbn_stubs PRIVATE ${SBN_APP_SOURCE_DIR}/fsw/src)
