project(SBN_F_REMAP C)

if(NOT(IS_DIRECTORY ${SBN_APP_SOURCE_DIR}))
    message(FATAL_ERROR "SBN_APP_SOURCE_DIR not defined, is sbn in the target list before this module?")
endif()

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_sbn_module(sbn_f_remap "SBN_F_Remap" ${LIB_SRC_FILES})
add_cfe_app_dependency(sbn_f_remap sbn)
target_include_directories(sbn_f_remap PUBLIC fsw/inc)

add_cfe_tables(sbn fsw/tables/sbn_remap_tbl.c)
target_include_directories(sbn.table INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fsw/inc)