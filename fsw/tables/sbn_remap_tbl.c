#include "sbn_tables.h"
#include "cfe_tbl_filedef.h"

SBN_RemapTable_t SBN_RemapTable =
{ 
    SBN_REMAP_DEFAULT_SEND, /* Remap Default */
    0, /* number of entries, initialized at validation time */
    {  /* remap table */
        /* {CPU_ID, from, to} and if to is 0x0000, filter rather than remap */
/*        {2, 0x0809, 0x0888},
        {2, 0x0811, 0x0889},
        {2, 0x18FA, 0x0000},*/ /* filter out SBN commands */
        {0, 0x0000, 0x0000} /* end of data marker */
    }
};/* end SBN_RemapTable */

CFE_TBL_FILEDEF(SBN_RemapTable, SBN.REMAP_TBL, SBN Remap Table,sbn_remap_tbl.tbl)
