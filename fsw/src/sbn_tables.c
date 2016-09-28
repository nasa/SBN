#include "sbn_tables.h"

int32 SBN_RemapTableValidation(void *TblPtr)
{
    SBN_RemapTable_t *r = (SBN_RemapTable_t *)TblPtr;
    int i = 0;

    if(r->Entries > SBN_REMAP_TABLE_SIZE)
    {
        return 1;
    }/* end if */

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

    /* Ensure the entries are sorted and unique,
     * we use a binary search per MID to find
     * the match. Note also this will catch any "from" values that are 0.
     */
    for(i = 0; i < SBN_REMAP_TABLE_SIZE; i++)
    {
        if(r->Entry[i].from == 0x0000)
        {
            /* found the marker, we're done */
            break;
        }/* end if */

        if(i > 0
            && (r->Entry[i - 1].ProcessorId > r->Entry[i].ProcessorId
            || (r->Entry[i - 1].ProcessorId == r->Entry[i].ProcessorId
                && r->Entry[i - 1].from >= r->Entry[i].from)))
        {
            /* entries are not sorted and/or not unique */
            return 4;
        }/* end if */
    }/* end for */

    r->Entries = i;

    return CFE_SUCCESS;
}/* end SBN_RemapTableValidation */

int32 SBN_LoadTables(CFE_TBL_Handle_t *HandlePtr)
{
    int32 Status = CFE_SUCCESS;

    Status = CFE_TBL_Register(HandlePtr, "SBN_RemapTable",
        sizeof(SBN_RemapTable_t), CFE_TBL_OPT_DEFAULT,
        &SBN_RemapTableValidation);
    if(Status != CFE_SUCCESS)
    {
        return Status;
    }/* end if */

    return CFE_TBL_Load(*HandlePtr, CFE_TBL_SRC_ADDRESS, &SBN_RemapTable);
}/* end SBN_RegisterTables */
