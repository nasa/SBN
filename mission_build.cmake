###########################################################
#
# SBN mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SBN configuration
set(SBN_MISSION_CONFIG_FILE_LIST
  sbn_extern_typedefs.h
  sbn_fcncode_values.h
  sbn_interface_cfg_values.h
  sbn_mission_cfg.h
  sbn_perfids.h
  sbn_msg.h
  sbn_msgdefs.h
  sbn_msgstruct.h
  sbn_tbl.h
  sbn_tbldefs.h
  sbn_tblstruct.h
  sbn_topicid_values.h
)

generate_configfile_set(${SBN_MISSION_CONFIG_FILE_LIST})
