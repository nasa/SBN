# For the native builds, include a wrapper script to start cFS on the container.
if (${SIMULATION} MATCHES "^native")
    install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/container-start DESTINATION cpu1)
endif()

set(APP_DEFINITION_TABLE_LIST)
set(APP_INCLUDE_TABLE_LIST
    $<TARGET_PROPERTY:ci_lab,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:to_lab,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:sbn,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:talker_app,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:listener_app,INCLUDE_DIRECTORIES>
)

# specify extra include dirs for to_lab + sch_lab table builds
target_include_directories(to_lab.table INTERFACE ${APP_INCLUDE_TABLE_LIST})
target_compile_definitions(to_lab.table INTERFACE ${APP_DEFINITION_TABLE_LIST})
target_include_directories(sch_lab.table INTERFACE ${APP_INCLUDE_TABLE_LIST})
target_compile_definitions(sch_lab.table INTERFACE ${APP_DEFINITION_TABLE_LIST})
