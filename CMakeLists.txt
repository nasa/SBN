project(SBN_APP C)

set(APP_SRC_FILES
  fsw/src/sbn_app.c
  fsw/src/sbn_cmds.c
  fsw/src/sbn_subs.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES fsw/src/sbn_eds_dispatch.c)
  list(APPEND APP_SRC_FILES fsw/src/sbn_eds_encoder.c)
else()
  list(APPEND APP_SRC_FILES fsw/src/sbn_dispatch.c)
  list(APPEND APP_SRC_FILES fsw/src/sbn_fixedpack_encoder.c)
  list(APPEND APP_SRC_FILES fsw/src/sbn_pack.c)
endif()

# Create the app module
add_cfe_app(sbn ${APP_SRC_FILES})
target_include_directories(sbn PUBLIC fsw/inc)

add_cfe_tables(sbn fsw/tables/sbn_conf_tbl.c)

if (ENABLE_UNIT_TESTS)
  add_subdirectory(ut-stubs)
  #add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
