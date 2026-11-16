project(SBN_APP C)

define_property(DIRECTORY PROPERTY SBN_MODULE_LIST
  BRIEF_DOCS "The list of modules to be linked with SBN"
  FULL_DOCS "The list of modules to be linked with SBN"
)

function(add_sbn_module SBN_MOD_NAME SBN_BINDING_SYMBOL_NAME SBN_MOD_SRC_FILES)
  # The input source list is allowed to be empty.  In that case we do not create
  # an object lib, just add the binding symbol ref the SBN_MODULE_LIST property.
  # This way we will bind to a symbol that is already loaded (old way of doing things)
  if (SBN_MOD_SRC_FILES)
    add_library(${SBN_MOD_NAME} OBJECT ${SBN_MOD_SRC_FILES} ${ARGN})
    if (APP_DYNAMIC_TARGET_LIST)
      # This means the SBN app is being compiled as a CFS module (the typical case)
      # All SBN modules need to be position independent code
      set_target_properties(${SBN_MOD_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    endif()

    target_include_directories(${SBN_MOD_NAME} PRIVATE $<TARGET_PROPERTY:sbn,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(${SBN_MOD_NAME} PRIVATE $<TARGET_PROPERTY:sbn,INTERFACE_COMPILE_DEFINITIONS>)
    target_compile_options(${SBN_MOD_NAME} PRIVATE $<TARGET_PROPERTY:sbn,INTERFACE_COMPILE_OPTIONS>)
  endif()

  set_property(DIRECTORY ${SBN_APP_SOURCE_DIR} APPEND PROPERTY SBN_MODULE_LIST "${SBN_MOD_NAME},${SBN_BINDING_SYMBOL_NAME}")

endfunction(add_sbn_module)

# The set of source files common to all builds
set(APP_SRC_FILES
  fsw/src/sbn_app.c
  fsw/src/sbn_cmds.c
  fsw/src/sbn_subs.c
  fsw/src/sbn_pack.c
  fsw/src/sbn_module.c
  fsw/src/sbn_resource.c
)

if (CFE_EDS_ENABLED)
# These files are for EDS-based I/O
  list(APPEND APP_SRC_FILES
    fsw/src/sbn_eds_dispatch.c
    fsw/src/sbn_eds_encoder.c
  )
else()
# These files are for non-eds I/O
  list(APPEND APP_SRC_FILES
    fsw/src/sbn_dispatch.c
    fsw/src/sbn_fixedpack_encoder.c
  )
endif()

set(SBNMOD_sbn_udp_PATH_MAP ${CMAKE_CURRENT_SOURCE_DIR}/modules/protocol/udp)
set(SBNMOD_sbn_f_remap_PATH_MAP ${CMAKE_CURRENT_SOURCE_DIR}/modules/filter/remap)

# Now search for requested modules
set(SBNMOD_MISSING_COUNT 0)
set(SBNMOD_FULL_LIST)
foreach(SBNMOD ${MISSION_SBN_MODULELIST} ${EXTRA_SBN_MODULELIST_${TARGETSYSTEM}})
  set (SBNMODFOUND ${SBNMOD_${SBNMOD}_PATH_MAP})
  if (NOT SBNMODFOUND)
    foreach(SBNMODSRC ${${SBNMOD}_SEARCH_PATH} ${MISSION_MODULE_SEARCH_PATH})
      if (NOT IS_ABSOLUTE "${SBNMODSRC}")
        set(SBNMODSRC "${MISSION_SOURCE_DIR}/${SBNMODSRC}")
      endif()
      if(IS_DIRECTORY "${SBNMODSRC}/${SBNMOD}")
        set(SBNMODFOUND "${SBNMODSRC}/${SBNMOD}")
        break()
      endif()
    endforeach()
  endif()
  if (SBNMODFOUND)
    get_filename_component(${SBNMOD}_MISSION_DIR "${SBNMODFOUND}" ABSOLUTE)
    list(APPEND SBNMOD_FULL_LIST ${SBNMOD})
    message(STATUS "SBN Module '${SBNMOD}' found at ${${SBNMOD}_MISSION_DIR}")
  else()
    message("** SBN Module ${SBNMOD} NOT found **")
    math(EXPR SBNMOD_MISSING_COUNT "${SBNMOD_MISSING_COUNT} + 1")
  endif()
endforeach()

if (SBNMOD_MISSING_COUNT GREATER 0)
  message(FATAL_ERROR "Missing SBN modules")
endif()

# Create the main SBN app module FIRST (before any SBN submodules)
# it is OK that the object libs are not defined yet
add_cfe_app(sbn
  ${APP_SRC_FILES}
  fsw/src/sbn_module_bindings.c
)
target_include_directories(sbn PUBLIC fsw/inc)

add_cfe_tables(sbn fsw/tables/sbn_conf_tbl.c)


# MIGRATION HACK: redefine add_cfe_app to throw an error,
# if any of the SBN submodules use it, instruct the user to use add_sbn_module instead.
function(add_cfe_app APP_NAME APP_SRC_FILES)
  message(FATAL_ERROR "do not call add_cfe_app() for SBN modules")
endfunction(add_cfe_app)

# Now add all the submodules.  Each of these should call add_sbn_module in some way
foreach(SBNMOD ${SBNMOD_FULL_LIST})
  add_subdirectory(${${SBNMOD}_MISSION_DIR} modules/${SBNMOD})
endforeach()

# Now see what modules were actually defined
set(SBNMOD_OBJ_LIST)
get_property(SBN_MODULE_LIST DIRECTORY ${SBN_APP_SOURCE_DIR} PROPERTY SBN_MODULE_LIST)
set(SBN_BINDING_FILE_CONTENT "/* This is a generated file with the list of SBN module symbols to link in */\n\n")
foreach(SBN_MODNAME_SYM ${SBN_MODULE_LIST})
  string(APPEND SBN_BINDING_FILE_CONTENT "SBN_MODULE(${SBN_MODNAME_SYM})\n")
  string(REGEX REPLACE ",.*$" "" SBN_MODULE_NAME "${SBN_MODNAME_SYM}") # extract only the name (first part before comma)
  if (TARGET ${SBN_MODULE_NAME})
    list(APPEND SBNMOD_OBJ_LIST $<TARGET_OBJECTS:${SBN_MODULE_NAME}>)
  endif()
endforeach()

# Write the file only if it changed.  This reduces unnecessary rebuilds.
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/modules/sbn_modules.inc")
  file(SHA1 "${CMAKE_CURRENT_BINARY_DIR}/modules/sbn_modules.inc" FILE_HASH)
else ()
  set(FILE_HASH "XX")
endif()

string(SHA1 "${SBN_BINDING_FILE_CONTENT}" STRING_HASH)
if (NOT FILE_HASH STREQUAL STRING_HASH)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/modules/sbn_modules.inc "${SBN_BINDING_FILE_CONTENT}")
endif()

# Add a special include path so that the module bindings can read this file.  This is the _ONLY_ source file that should ever see it.
set_property(SOURCE fsw/src/sbn_module_bindings.c APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/modules)
target_sources(sbn PRIVATE ${SBNMOD_OBJ_LIST})

if (ENABLE_UNIT_TESTS)
  add_subdirectory(ut-stubs)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
