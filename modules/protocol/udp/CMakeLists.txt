project(SBN_UDP C)

if(NOT(IS_DIRECTORY ${SBN_APP_SOURCE_DIR}))
    message(FATAL_ERROR "SBN_APP_SOURCE_DIR not defined, is sbn in the target list before this module?")
endif()

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_sbn_module(sbn_udp "SBN_UDP_Ops" ${LIB_SRC_FILES})

if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
