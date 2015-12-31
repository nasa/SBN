cmake_minimum_required(VERSION 2.6.4)
<<<<<<< HEAD
project(CFS_SBN C)

include_directories(fsw/platform_inc)

=======
project(SBN_APP C)

# workaround until these definitions are exposed by cfe_sb
add_definitions(-DCFE_SB_DO_NOT_INCREMENT=0 -DCFE_SB_SEND_ONECOPY=1)

include_directories(fsw/platform_inc)

if(NOT(IS_DIRECTORY ${SBN_LIB_SOURCE_DIR}))
    message(FATAL_ERROR "SBN_LIB_SOURCE_DIR not defined, is sbn_lib in the target list before sbn?")
endif()

include_directories(${SBN_LIB_SOURCE_DIR}/fsw/public_inc)

>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(sbn ${APP_SRC_FILES})
