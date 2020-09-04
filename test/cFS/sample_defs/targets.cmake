SET(MISSION_NAME "SampleMission")
SET(SPACECRAFT_ID 0x42)
list(APPEND MISSION_GLOBAL_APPLIST sbn sbn_udp sbn_tcp sch_lab sbn_f_remap)

SET(FT_INSTALL_SUBDIR "host/functional-test")

SET(TGT1_NAME cpu1)
SET(TGT1_APPLIST ci_lab)
SET(TGT1_FILELIST cfe_es_startup.scr)

SET(TGT2_NAME cpu2)
SET(TGT2_APPLIST fib)
SET(TGT2_FILELIST cfe_es_startup.scr)

SET(TGT3_NAME cpu3)
SET(TGT3_APPLIST to_lab)
SET(TGT3_FILELIST cfe_es_startup.scr)
