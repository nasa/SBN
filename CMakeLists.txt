cmake_minimum_required(VERSION 2.6.4)
project(SBN_APP C)

# workaround until these definitions are exposed by cfe_sb
include_directories(${CFECORE_SOURCE_DIR}/src/sb)

include_directories(fsw/platform_inc)

if(NOT(IS_DIRECTORY ${SBN_LIB_SOURCE_DIR}))
    message(FATAL_ERROR "SBN_LIB_SOURCE_DIR not defined, is sbn_lib in the target list before sbn?")
endif()

include_directories(${SBN_LIB_SOURCE_DIR}/fsw/public_inc)

aux_source_directory(fsw/src APP_SRC_FILES)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(sbn ${APP_SRC_FILES})
