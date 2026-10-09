###########################################################
#
# SBN platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SBN configuration
set(SBN_PLATFORM_CONFIG_FILE_LIST
  sbn_internal_cfg_values.h
  sbn_platform_cfg.h
  sbn_perfids.h
  sbn_msgids.h
  sbn_msgid_values.h
)

generate_configfile_set(${SBN_PLATFORM_CONFIG_FILE_LIST})

