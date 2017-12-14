#include "sbn_app.h"
#include "sbn_remap.h"
#include "cfe_tbl.h"

static int32 RemapTblVal(void *TblPtr)
{
    SBN_RemapTbl_t *r = (SBN_RemapTbl_t *)TblPtr;
    int i = 0;

    switch(r->RemapDefaultFlag)
    {
        /* all valid values */
        case SBN_REMAP_DEFAULT_IGNORE:
        case SBN_REMAP_DEFAULT_SEND:
            break;
        /* otherwise, unknown! */
        default:
            return 2;
    }/* end switch */

    /* Find the first "empty" entry (with a 0x0000 "from") to determine table
     * size.
     */
    for(i = 0; i < SBN_REMAP_TABLE_SIZE; i++)
    {
        if (r->Entries[i].FromMID == 0x0000)
        {
            break;
        }/* end if */
    }/* end for */

    r->EntryCnt = i;

    SBN_RemapTblSort(r);

    return CFE_SUCCESS;
}/* end SBN_RemapTblVal */

int32 SBN_LoadTbl(SBN_RemapTbl_t **TblPtr)
{
    CFE_TBL_Handle_t TblHandle;
    int32 Status = CFE_SUCCESS;

    if((Status = CFE_TBL_Register(&TblHandle, "SBN_RemapTbl",
        sizeof(SBN_RemapTbl_t), CFE_TBL_OPT_DEFAULT, &RemapTblVal))
        != CFE_SUCCESS)
    {
        return Status;
    }/* end if */

    if((Status = CFE_TBL_Load(TblHandle, CFE_TBL_SRC_FILE, SBN_TBL_FILENAME))
        != CFE_SUCCESS)
    {
        return Status;
    }/* end if */

    if((Status = CFE_TBL_Manage(TblHandle)) != CFE_SUCCESS)
    {
        return Status;
    }/* end if */

    if((Status = CFE_TBL_GetAddress((void **)TblPtr, TblHandle))
        != CFE_TBL_INFO_UPDATED)
    {
        return Status;
    }/* end if */

    return CFE_SUCCESS;
}/* end SBN_LoadTbl */
