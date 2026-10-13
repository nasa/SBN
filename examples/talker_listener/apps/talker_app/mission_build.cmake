###########################################################
#
# TALKER_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the TALKER_APP configuration
set(TALKER_APP_MISSION_CONFIG_FILE_LIST
  talker_app_fcncodes.h
  talker_app_interface_cfg.h
  talker_app_mission_cfg.h
  talker_app_perfids.h
  talker_app_msg.h
  talker_app_msgdefs.h
  talker_app_msgstruct.h
  talker_app_tbl.h
  talker_app_tbldefs.h
  talker_app_tblstruct.h
  talker_app_topicids.h
)

if (CFE_EDS_ENABLED_BUILD)

  # In an EDS-based build, these files come generated from the EDS tool
  set(TALKER_APP_CFGFILE_SRC_talker_app_interface_cfg "talker_app_eds_designparameters.h")
  set(TALKER_APP_CFGFILE_SRC_talker_app_tbldefs       "talker_app_eds_typedefs.h")
  set(TALKER_APP_CFGFILE_SRC_talker_app_tblstruct     "talker_app_eds_typedefs.h")
  set(TALKER_APP_CFGFILE_SRC_talker_app_msgdefs       "talker_app_eds_typedefs.h")
  set(TALKER_APP_CFGFILE_SRC_talker_app_msgstruct     "talker_app_eds_typedefs.h")
  set(TALKER_APP_CFGFILE_SRC_talker_app_fcncodes      "talker_app_eds_cc.h")

endif(CFE_EDS_ENABLED_BUILD)

# Create wrappers around the all the config header files
# This makes them individually overridable by the missions, without modifying
# the distribution default copies
foreach(TALKER_APP_CFGFILE ${TALKER_APP_MISSION_CONFIG_FILE_LIST})
  get_filename_component(CFGKEY "${TALKER_APP_CFGFILE}" NAME_WE)
  if (DEFINED TALKER_APP_CFGFILE_SRC_${CFGKEY})
    set(DEFAULT_SOURCE GENERATED_FILE "${TALKER_APP_CFGFILE_SRC_${CFGKEY}}")
  else()
    set(DEFAULT_SOURCE FALLBACK_FILE "${CMAKE_CURRENT_LIST_DIR}/config/default_${TALKER_APP_CFGFILE}")
  endif()
  generate_config_includefile(
    FILE_NAME           "${TALKER_APP_CFGFILE}"
    ${DEFAULT_SOURCE}
  )
endforeach()
