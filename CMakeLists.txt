cmake_minimum_required(VERSION 2.6.4)
project(SBN_APP C)

# workaround until these definitions are exposed by cfe_sb
include_directories(${CFECORE_SOURCE_DIR}/src/sb)

include_directories(fsw/platform_inc)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(sbn ${APP_SRC_FILES})

aux_source_directory(fsw/tables APP_TBL_FILES)
add_cfe_tables(sbn ${APP_TBL_FILES})

if (ENABLE_UNIT_TESTS)
  add_subdirectory(ut-stubs)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)

# The bundled configuration loads these modules dynamically. Build them when
# the mission selects only sbn, but leave explicitly selected modules to cFE.
if(NOT DEFINED SBN_BUILD_DEFAULT_MODULES)
  set(SBN_BUILD_DEFAULT_MODULES ON)
endif()
if(SBN_BUILD_DEFAULT_MODULES)
  if(NOT DEFINED sbn_udp_MISSION_DIR)
    add_subdirectory(modules/protocol/udp)
  endif()
  if(NOT DEFINED sbn_f_remap_MISSION_DIR)
    set(sbn_f_remap_MISSION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/filter/remap")
    set(TGTLIST_sbn_f_remap ${TGTLIST_sbn})
    add_subdirectory(modules/filter/remap)
  endif()
endif()
