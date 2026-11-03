project(SBN_APP C)

# The set of source files common to all builds
set(APP_SRC_FILES
  fsw/src/sbn_app.c
  fsw/src/sbn_cmds.c
  fsw/src/sbn_subs.c
)

# These files are for non-eds I/O
list(APPEND APP_SRC_FILES
  fsw/src/sbn_dispatch.c
  fsw/src/sbn_pack.c
)

# Create the app module
add_cfe_app(sbn ${APP_SRC_FILES})
target_include_directories(sbn PUBLIC fsw/inc)

add_cfe_tables(sbn fsw/tables/sbn_conf_tbl.c)

if (ENABLE_UNIT_TESTS)
  add_subdirectory(ut-stubs)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
