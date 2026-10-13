###########################################################
#
# LISTENER_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the LISTENER_APP configuration
set(LISTENER_APP_MISSION_CONFIG_FILE_LIST
  listener_app_fcncodes.h
  listener_app_interface_cfg.h
  listener_app_mission_cfg.h
  listener_app_perfids.h
  listener_app_msg.h
  listener_app_msgdefs.h
  listener_app_msgstruct.h
  listener_app_tbl.h
  listener_app_tbldefs.h
  listener_app_tblstruct.h
  listener_app_topicids.h
)

if (CFE_EDS_ENABLED_BUILD)

  # In an EDS-based build, these files come generated from the EDS tool
  set(LISTENER_APP_CFGFILE_SRC_listener_app_interface_cfg "listener_app_eds_designparameters.h")
  set(LISTENER_APP_CFGFILE_SRC_listener_app_tbldefs       "listener_app_eds_typedefs.h")
  set(LISTENER_APP_CFGFILE_SRC_listener_app_tblstruct     "listener_app_eds_typedefs.h")
  set(LISTENER_APP_CFGFILE_SRC_listener_app_msgdefs       "listener_app_eds_typedefs.h")
  set(LISTENER_APP_CFGFILE_SRC_listener_app_msgstruct     "listener_app_eds_typedefs.h")
  set(LISTENER_APP_CFGFILE_SRC_listener_app_fcncodes      "listener_app_eds_cc.h")

endif(CFE_EDS_ENABLED_BUILD)

# Create wrappers around the all the config header files
# This makes them individually overridable by the missions, without modifying
# the distribution default copies
foreach(LISTENER_APP_CFGFILE ${LISTENER_APP_MISSION_CONFIG_FILE_LIST})
  get_filename_component(CFGKEY "${LISTENER_APP_CFGFILE}" NAME_WE)
  if (DEFINED LISTENER_APP_CFGFILE_SRC_${CFGKEY})
    set(DEFAULT_SOURCE GENERATED_FILE "${LISTENER_APP_CFGFILE_SRC_${CFGKEY}}")
  else()
    set(DEFAULT_SOURCE FALLBACK_FILE "${CMAKE_CURRENT_LIST_DIR}/config/default_${LISTENER_APP_CFGFILE}")
  endif()
  generate_config_includefile(
    FILE_NAME           "${LISTENER_APP_CFGFILE}"
    ${DEFAULT_SOURCE}
  )
endforeach()
