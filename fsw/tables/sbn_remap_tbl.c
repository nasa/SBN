#include "sbn_tbl.h"
#include "cfe_tbl_filedef.h"

SBN_RemapTbl_t SBN_RemapTbl =
{ 
    SBN_REMAP_DEFAULT_SEND, /* Remap Default */
    0, /* number of entries, initialized at validation time */
    {  /* remap table */
        /* {CPU_ID, from, to} and if to is 0x0000, filter rather than remap */
        {2, 0x0811, 0x0889},
        {2, 0x0809, 0x0888},
        {3, 0x0804, 0x0888},
        {4, 0x1234, 0x4321},
        {2, 0x18FA, 0x0000}  /* filter out SBN commands */
    }
};/* end SBN_RemapTbl */

CFE_TBL_FILEDEF(SBN_RemapTbl, SBN.REMAP_TBL, SBN Remap Table,sbn_remap_tbl.tbl)
