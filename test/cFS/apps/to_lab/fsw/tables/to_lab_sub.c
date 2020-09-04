#include "cfe_tbl_filedef.h"
#include "to_lab_sub_table.h"
#include "to_lab_msgids.h"
#include "fib_app_msgids.h"

TO_LAB_Subs_t TO_LAB_Subs =
{
    .Subs =
    {
        {CFE_SB_MSGID_WRAP_VALUE(FIB_TLM_REMAP_MID), {0, 0}, 4}
    }
};

CFE_TBL_FILEDEF(TO_LAB_Subs, TO_LAB_APP.TO_LAB_Subs, TO Lab Sub Tbl, to_lab_sub.tbl)
