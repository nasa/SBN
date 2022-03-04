#include "sbn_remap_tbl.h"
#include "cfe_tbl_filedef.h"

SBN_RemapTbl_t SBN_RemapTbl = {
    .RemapDefaultFlag = SBN_REMAP_DEFAULT_SEND,
    .Entries          = {{.ProcessorID = 3, .SpacecraftID = 0x42, .FromMID = {0x0882}, .ToMID = {0x0883}},

                /** ProcessorID "0" signals the end of the table. */
                {.ProcessorID = 0, .SpacecraftID = 0, .FromMID = {0x0000}, .ToMID = {0x0000}}}}; /* end SBN_RemapTbl */

CFE_TBL_FILEDEF(SBN_RemapTbl, SBN.SBN_RemapTbl, SBN Remap Table, sbn_remap_tbl.tbl)
