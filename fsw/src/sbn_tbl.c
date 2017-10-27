#include "sbn_app.h"
#include "sbn_remap.h"

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

int32 SBN_LoadTbl(CFE_TBL_Handle_t *HandlePtr)
{
    int32 Status = CFE_SUCCESS;

    Status = CFE_TBL_Register(HandlePtr, "SBN_RemapTbl",
        sizeof(SBN_RemapTbl_t), CFE_TBL_OPT_DEFAULT, &RemapTblVal);

    if(Status != CFE_SUCCESS)
    {
        return Status;
    }/* end if */

    return CFE_TBL_Load(*HandlePtr, CFE_TBL_SRC_ADDRESS, &SBN_RemapTbl);
}/* end SBN_LoadTbl */
