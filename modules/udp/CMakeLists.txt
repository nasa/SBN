cmake_minimum_required(VERSION 2.6.4)
project(SBN_UDP_MODULE C)

INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(BIG_ENDIAN)

if(BIG_ENDIAN)
    add_definitions(-DBIG_ENDIAN)
else(BIG_ENDIAN)
    add_definitions(-DLITTLE_ENDIAN)
endif(BIG_ENDIAN)

if(NOT(IS_DIRECTORY ${SBN_APP_SOURCE_DIR}))
    message(FATAL_ERROR "SBN_APP_SOURCE_DIR not defined, is sbn in the target list before this module?")
endif()

include_directories(${SBN_APP_SOURCE_DIR}/fsw/platform_inc)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_cfe_app(sbn_udp_module ${LIB_SRC_FILES})
