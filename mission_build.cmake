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
  sbn_interface_cfg_values.h
  sbn_mission_cfg.h
)

generate_configfile_set(${SBN_MISSION_CONFIG_FILE_LIST})
