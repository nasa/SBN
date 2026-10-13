project(CFE_LISTENER_APP C)

set(APP_SRC_FILES
  fsw/src/listener_app.c
  fsw/src/listener_app_cmds.c
  fsw/src/listener_app_utils.c
)

# Talker libraries - CKC
#
if (CFE_EDS_ENABLED_BUILD)
  list(APPEND APP_SRC_FILES
    fsw/src/listener_app_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/listener_app_dispatch.c
  )
endif()

# Create the app module
add_cfe_app(listener_app ${APP_SRC_FILES})

target_include_directories(listener_app PUBLIC fsw/inc)
# Include the public API from talker_app to demonstrate how
# to call listener_app message-ids.
add_cfe_app_dependency(listener_app talker_app)

# Add table
add_cfe_tables(listener_app fsw/tables/listener_app_tbl.c)

# # If UT is enabled, then add the tests from the subdirectory
# # Note that this is an app, and therefore does not provide
# # stub functions, as other entities would not typically make
# # direct function calls into this application.
# if (ENABLE_UNIT_TESTS)
#   add_subdirectory(unit-test)
# endif (ENABLE_UNIT_TESTS)
