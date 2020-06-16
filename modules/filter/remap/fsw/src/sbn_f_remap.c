#include "sbn_types.h"
#include "sbn_remap_tbl.h"
#include "cfe.h"
#include <string.h> /* memcpy */
#include <stdlib.h> /* qsort */

SBN_Tbl_Handle_t RemapTblHandle;
SBN_RemapTbl_t *RemapTbl;

static int RemapTblVal(void *TblPtr)
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
            return -1;
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

    return 0;
}/* end RemapTblVal */

static SBN_Status_t LoadRemap(void)
{
    CFE_Status_t Status = CFE_SUCCESS;
    SBN_RemapTbl_t *TblPtr;

    if((Status = CFE_TBL_GetAddress((void **)&TblPtr, RemapTblHandle))
        != CFE_TBL_INFO_UPDATED)
    {
        CFE_EVS_SendEvent(SBN_TBL_EID, CFE_EVS_ERROR,
            "unable to get conf table address");
        CFE_TBL_Unregister(RemapTblHandle);
        return SBN_ERROR;
    }/* end if */

    SBN_RemapTblSort(TblPtr);

    CFE_TBL_Modified(RemapTblHandle);

    RemapTbl = TblPtr;

    return SBN_SUCCESS;
}/* end LoadRemap() */

static SBN_Status_t LoadRemapTbl(void)
{
    if(CFE_TBL_Register(&RemapTblHandle, "SBN_RemapTbl", sizeof(SBN_RemapTbl_t),
        CFE_TBL_OPT_DEFAULT, &RemapTblVal) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TBL_EID, CFE_EVS_ERROR,
            "unable to register remap tbl handle");
        return SBN_ERROR;
    }/* end if */

    if(CFE_TBL_Load(RemapTblHandle, CFE_TBL_SRC_FILE, SBN_REMAP_TBL_FILENAME) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TBL_EID, CFE_EVS_ERROR,
            "unable to load remap tbl %s", SBN_REMAP_TBL_FILENAME);
        CFE_TBL_Unregister(RemapTblHandle);
        return SBN_ERROR;
    }/* end if */

    if(CFE_TBL_Manage(RemapTblHandle) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TBL_EID, CFE_EVS_ERROR,
            "unable to manage remap tbl");
        CFE_TBL_Unregister(RemapTblHandle);
        return SBN_ERROR;
    }/* end if */

    if(CFE_TBL_NotifyByMessage(RemapTblHandle, SBN_CMD_MID, SBN_TBL_CC, 1) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TBL_EID, CFE_EVS_ERROR,
            "unable to set notifybymessage for remap tbl");
        CFE_TBL_Unregister(RemapTblHandle);
        return SBN_ERROR;
    }/* end if */

    return LoadRemap();
}/* end LoadRemapTbl */

static SBN_Status_t UnloadRemap(void)
{
    RemapTbl = NULL;

    CFE_Status_t Status;

    if((Status = CFE_TBL_ReleaseAddress(RemapTblHandle)) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TBL_EID, CFE_EVS_ERROR,
            "unable to release address for remap tbl");
        return SBN_ERROR;
    }/* end if */

    return SBN_SUCCESS;
}/* end UnloadRemap() */

static SBN_Status_t ReloadRemapTbl(void)
{
    SBN_Status_t Status;

    /* releases the table address */
    if((Status = UnloadRemap()) != SBN_SUCCESS)
    {
        return Status;
    }/* end if */

    if(CFE_TBL_Update(RemapTblHandle) != CFE_SUCCESS)
    {
        return SBN_ERROR;
    }/* end if */

    /* gets the new address and loads config */
    return LoadRemap();
}/* end ReloadRemapTbl() */

static int RemapTblCompar(const void *a, const void *b)
{
    SBN_RemapTblEntry_t *aEntry = (SBN_RemapTblEntry_t *)a;
    SBN_RemapTblEntry_t *bEntry = (SBN_RemapTblEntry_t *)b;

    if(aEntry->ProcessorID != bEntry->ProcessorID)
    {   
        return aEntry->ProcessorID - bEntry->ProcessorID;
    }
    return aEntry->FromMID - bEntry->FromMID;
}

/* finds the entry or the one that would immediately follow it */
static int BinarySearch(void *Entries, void *SearchEntry,
    size_t EntryCnt, size_t EntrySz,
    int (*EntryCompare)(const void *, const void *))
{
    int start, end, midpoint, found;

    for(start = 0, end = EntryCnt - 1, found = 0;
        found == 0 && start <= end;
        )
    {
        midpoint = (end + start) / 2;
        int c = EntryCompare(SearchEntry, (uint8 *)Entries + EntrySz * midpoint);
        if (c == 0)
        {
            return midpoint;
        }
        else if (c > 0)
        {
            start = midpoint + 1;
        }
        else
        {   
           end = midpoint - 1;
        }/* end if */
    }/* end while */

    if(found == 0)
    {
        return EntryCnt;
    }

    return midpoint;
}/* end BinarySearch */

static int RemapTblSearch(uint32 ProcessorID, CFE_SB_MsgId_t MID)
{
    SBN_RemapTblEntry_t Entry = {ProcessorID, MID, 0x0000};
    return BinarySearch(SBN.RemapTbl->Entries, &Entry,
        SBN.RemapTbl->EntryCnt,
        sizeof(SBN_RemapTblEntry_t),
        RemapTblCompar);
}/* end RemapTblSearch() */

static void RemapTblSort(SBN_RemapTbl_t *Tbl)
{
    /* sort the entries on ProcessorID and from MID */
    /* note: qsort is recursive, so it will use some stack space
     * (O[N log N] * <some small amount of stack>). If this is a concern,
     * consider using a non-recursive (insertion, bubble, etc) sort algorithm.
     */

    qsort(Tbl->Entries, Tbl->EntryCnt, sizeof(SBN_RemapTblEntry_t),
        RemapTblCompar);
}/* end RemapTblSort() */

}/* end RemapMsgID() */

SBN_Status_t SBN_F_Remap(void *msg, SBN_Filter_Ctx_t *Context)
{
    CFE_SB_MsgId_t FromMID = 0x0000, ToMID = 0x0000;
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    SBN_Status_t Status;

    FromMID = CCSDS_RD_SID(*PriHdrPtr);

    if(OS_MutSemTake(SBN.RemapMutex) != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_REMAP_EID, CFE_EVS_ERROR, "unable to take mutex");
        return SBN_ERROR;
    }/* end if */

    int i = RemapTblSearch(ProcessorID, FromMID);

    if(i < SBN.RemapTbl->EntryCnt
        && SBN.RemapTbl->Entries[i].ProcessorID == ProcessorID
        && SBN.RemapTbl->Entries[i].FromMID == FromMID)
    {
        ToMID = SBN.RemapTbl->Entries[i].ToMID;
    }
    else
    {
        if(SBN.RemapTbl->RemapDefaultFlag == SBN_REMAP_DEFAULT_SEND)
        {
            ToMID = FromMID;
        }/* end if */
    }/* end if */

    if(OS_MutSemGive(SBN.RemapMutex) != OS_SUCCESS)
    {   
        CFE_EVS_SendEvent(SBN_REMAP_EID, CFE_EVS_ERROR, "unable to give mutex");
        return SBN_ERROR;
    }/* end if */

    if(ToMID == 0x0000)
    {
        return SBN_IF_EMPTY; /* signal to the core app that this filter recommends not sending this message */
    }/* end if */

    CCSDS_WR_SID(*PriHdrPtr, ToMID);

    return SBN_SUCCESS;
}/* end SBN_F_Remap() */

CFE_Status_t SBN_F_Remap_LibInit(void)
{
    OS_printf("SBN_F_Remap Lib Initialized.");
    return CFE_SUCCESS;
}/* end SBN_F_Remap_LibInit() */

