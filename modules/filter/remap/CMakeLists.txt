project(SBN_F_REMAP C)

if(NOT(IS_DIRECTORY ${SBN_APP_SOURCE_DIR}))
    message(FATAL_ERROR "SBN_APP_SOURCE_DIR not defined, is sbn in the target list before this module?")
endif()

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_cfe_app(sbn_f_remap ${LIB_SRC_FILES})
add_cfe_app_dependency(sbn_f_remap sbn)
target_include_directories(sbn_f_remap PUBLIC fsw/inc)

add_cfe_tables(sbn_f_remap fsw/tables/sbn_remap_tbl.c)
target_include_directories(sbn_f_remap.table INTERFACE $<TARGET_PROPERTY:sbn,INCLUDE_DIRECTORIES>)
