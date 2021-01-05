#include "cfe_tbl_filedef.h"
#include "sch_lab_table.h"
#include "cfe_sb.h"
#include "ci_lab_msgids.h"
#include "sbn_msgids.h"

SCH_LAB_ScheduleTable_t SCH_TBL_Structure = {.Config = {
                                                 {CFE_SB_MSGID_WRAP_VALUE(CFE_ES_SEND_HK_MID), 4},
                                                 {CFE_SB_MSGID_WRAP_VALUE(CFE_EVS_SEND_HK_MID), 4},
                                                 {CFE_SB_MSGID_WRAP_VALUE(CFE_TIME_SEND_HK_MID), 4},
                                                 {CFE_SB_MSGID_WRAP_VALUE(CFE_SB_SEND_HK_MID), 4},
                                                 {CFE_SB_MSGID_WRAP_VALUE(CFE_TBL_SEND_HK_MID), 4},
                                                 {CFE_SB_MSGID_WRAP_VALUE(CI_LAB_SEND_HK_MID), 4},
                                                 {CFE_SB_MSGID_WRAP_VALUE(SBN_CMD_MID), 4},
                                             }};

CFE_TBL_FILEDEF(SCH_TBL_Structure, SCH_LAB_APP.SCH_LAB_SchTbl, Schedule Lab MsgID Table, sch_lab_table.tbl)
