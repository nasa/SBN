#include "sbn_interfaces.h"
#include "sbn_remap_tbl.h"
#include "sbn_msgids.h"
#include "cfe.h"
#include "cfe_tbl.h"
#include <string.h> /* memcpy */
#include <stdlib.h> /* qsort */

#include "sbn_f_remap_events.h"

const char SBN_F_REMAP_TABLE_NAME[] = "SBN_RemapTbl";

bool             RemapInitialized = false;
OS_MutexID_t     RemapMutex  = 0;
CFE_TBL_Handle_t RemapTblHandle = 0;
SBN_RemapTbl_t   *RemapTbl    = NULL;
int              RemapTblCnt = 0;

CFE_EVS_EventID_t SBN_F_REMAP_FIRST_EID;

static int RemapTblVal(void *TblPtr)
{
    SBN_RemapTbl_t *r = (SBN_RemapTbl_t *)TblPtr;
    int             i = 0;

    switch (r->RemapDefaultFlag)
    {
        /* all valid values */
        case SBN_REMAP_DEFAULT_IGNORE:
        case SBN_REMAP_DEFAULT_SEND:
            break;
        /* otherwise, unknown! */
        default:
            return -1;
    } /* end switch */

    /* Find the first "empty" entry (with a 0x0000 "from") to determine table
     * size.
     */
    for (i = 0; i < SBN_REMAP_TABLE_SIZE; i++)
    {
        if (r->Entries[i].FromMID == 0x0000)
        {
            break;
        } /* end if */
    }     /* end for */

    RemapTblCnt = i;

    return 0;
} /* end RemapTblVal() */

static int RemapTblCompar(const void *a, const void *b)
{
    SBN_RemapTblEntry_t *aEntry = (SBN_RemapTblEntry_t *)a;
    SBN_RemapTblEntry_t *bEntry = (SBN_RemapTblEntry_t *)b;

    if (aEntry->SpacecraftID != bEntry->SpacecraftID)
    {
        return aEntry->SpacecraftID - bEntry->SpacecraftID;
    }
    if (aEntry->ProcessorID != bEntry->ProcessorID)
    {
        return aEntry->ProcessorID - bEntry->ProcessorID;
    }
    return aEntry->FromMID - bEntry->FromMID;
} /* end RemapTblCompar() */

static SBN_Status_t LoadRemapTbl(void)
{
    SBN_RemapTbl_t * TblPtr         = NULL;
    CFE_Status_t     CFE_Status     = CFE_SUCCESS;

    if (CFE_TBL_Register(&RemapTblHandle, SBN_F_REMAP_TABLE_NAME, sizeof(SBN_RemapTbl_t), CFE_TBL_OPT_DEFAULT, &RemapTblVal) !=
        CFE_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to register remap tbl handle");
        return SBN_ERROR;
    } /* end if */

    if (CFE_TBL_Load(RemapTblHandle, CFE_TBL_SRC_FILE, SBN_REMAP_TBL_FILENAME) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to load remap tbl %s", SBN_REMAP_TBL_FILENAME);
        CFE_TBL_Unregister(RemapTblHandle);
        return SBN_ERROR;
    } /* end if */

    if ((CFE_Status = CFE_TBL_GetAddress((void **)&TblPtr, RemapTblHandle)) != CFE_TBL_INFO_UPDATED)
    {
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to get conf table address");
        CFE_TBL_Unregister(RemapTblHandle);
        return SBN_ERROR;
    } /* end if */

    /* sort the entries on <ProcessorID> and <from MID> */
    /* note: qsort is recursive, so it will use some stack space
     * (O[N log N] * <some small amount of stack>). If this is a concern,
     * consider using a non-recursive (insertion, bubble, etc) sort algorithm.
     */

    qsort(TblPtr->Entries, RemapTblCnt, sizeof(SBN_RemapTblEntry_t), RemapTblCompar);

    CFE_TBL_Modified(RemapTblHandle);

    RemapTbl = TblPtr;

    return SBN_SUCCESS;
} /* end LoadRemapTbl() */

/* finds the entry or the one that would immediately follow it */
static int BinarySearch(void *Entries, void *SearchEntry, size_t EntryCnt, size_t EntrySz,
                        int (*EntryCompare)(const void *, const void *))
{
    int start, end, midpoint, found;

    for (start = 0, end = EntryCnt - 1, found = 0; found == 0 && start <= end;)
    {
        midpoint = (end + start) / 2;
        int c    = EntryCompare(SearchEntry, (uint8 *)Entries + EntrySz * midpoint);
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
        } /* end if */
    }     /* end while */

    if (found == 0)
    {
        return EntryCnt;
    } /* end if */

    return midpoint;
} /* end BinarySearch() */

static int RemapTblSearch(uint32 ProcessorID, uint32 SpacecraftID, CFE_SB_MsgId_t MID)
{
    SBN_RemapTblEntry_t Entry = {ProcessorID, SpacecraftID, MID, 0x0000};
    return BinarySearch(RemapTbl->Entries, &Entry, RemapTblCnt, sizeof(SBN_RemapTblEntry_t), RemapTblCompar);
} /* end RemapTblSearch() */

static SBN_Status_t Remap(void *msg, SBN_Filter_Ctx_t *Context)
{
    CFE_SB_MsgId_t     FromMID = 0x0000, ToMID = 0x0000;
    CCSDS_PriHdr_t *PriHdrPtr = msg;

    FromMID = CCSDS_RD_SID(*PriHdrPtr);

    if (OS_MutSemTake(RemapMutex) != OS_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_EID, "unable to take mutex");
        return SBN_ERROR;
    } /* end if */

    int i = RemapTblSearch(Context->PeerProcessorID, Context->PeerSpacecraftID, FromMID);

    if (i < RemapTblCnt &&
        RemapTbl->Entries[i].ProcessorID == Context->PeerProcessorID &&
        RemapTbl->Entries[i].SpacecraftID == Context->PeerSpacecraftID &&
        RemapTbl->Entries[i].FromMID == FromMID)
    {
        ToMID = RemapTbl->Entries[i].ToMID;
    }
    else
    {
        if (RemapTbl->RemapDefaultFlag == SBN_REMAP_DEFAULT_SEND)
        {
            ToMID = FromMID;
        } /* end if */
    }     /* end if */

    if (OS_MutSemGive(RemapMutex) != OS_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_EID, "unable to give mutex");
        return SBN_ERROR;
    } /* end if */

    if (ToMID == 0x0000)
    {
        return SBN_IF_EMPTY; /* signal to the core app that this filter recommends not sending this message */
    }                        /* end if */

    CCSDS_WR_SID(*PriHdrPtr, ToMID);

    return SBN_SUCCESS;
} /* end Remap() */

static SBN_Status_t Remap_MID(CFE_SB_MsgId_t *InOutMsgIdPtr, SBN_Filter_Ctx_t *Context)
{
    int i = 0;

    for (i = 0; i < RemapTblCnt; i++)
    {
        if (RemapTbl->Entries[i].ProcessorID == Context->PeerProcessorID &&
            RemapTbl->Entries[i].SpacecraftID == Context->PeerSpacecraftID &&
            RemapTbl->Entries[i].ToMID == *InOutMsgIdPtr)
        {
            *InOutMsgIdPtr = RemapTbl->Entries[i].FromMID;
            return SBN_SUCCESS;
        } /* end if */
    }     /* end for */

    return SBN_SUCCESS;
} /* end Remap_MID() */

static SBN_Status_t UnloadRemapTbl(void)
{
    //  Check the handle to a previously-registered table
    if (CFE_TBL_GetStatus(RemapTblHandle) == CFE_SUCCESS)
    {
        // Unregister and free resources used by table
        if(CFE_TBL_Unregister(RemapTblHandle) != CFE_SUCCESS) {
            EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to unregister table");
        }
        RemapTbl = NULL;
    } else {
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to get registered table");
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

static SBN_Status_t Deinit(CFE_EVS_EventID_t BaseEID)
{
    if (OS_MutSemTake(RemapMutex) != OS_SUCCESS)
    {
        EVSSendErr(BaseEID, "unable to take mutex");
        return SBN_ERROR;
    } /* end if */

    if(UnloadRemapTbl() != SBN_SUCCESS) {
        EVSSendErr(BaseEID, "unable to unload table");
        return SBN_ERROR;
    }

    if (OS_MutSemGive(RemapMutex) != OS_SUCCESS)
    {
        EVSSendErr(BaseEID, "unable to give mutex");
        return SBN_ERROR;
    } /* end if */

    if (RemapMutex != 0)
    {
        if(OS_MutSemDelete(RemapMutex) != OS_SUCCESS) {
            EVSSendErr(BaseEID, "unable to delete mutex");
            return SBN_ERROR;
        }
        RemapMutex = 0;
    }

    RemapInitialized = false;

    return SBN_SUCCESS;
}

static SBN_Status_t Init(int Version, CFE_EVS_EventID_t BaseEID)
{
    CFE_ES_TaskInfo_t TaskInfo;
    OS_Status_t OS_Status;
    CFE_Status_t CFE_Status;

    SBN_F_REMAP_FIRST_EID = BaseEID;

    if (Version != 2) /* TODO: define */
    {
        OS_printf("SBN_F_Remap version mismatch: expected %d, got %d\n", 1, Version);
        return SBN_ERROR;
    } /* end if */

    /* get task name to retrieve table after loading */
    uint32 TskId = OS_TaskGetId();
    if ((CFE_Status = CFE_ES_GetTaskInfo(&TaskInfo, TskId)) != CFE_SUCCESS)
    {
        EVSSendErr(BaseEID, "SBN failed to get task info (%d)", (int)CFE_Status);
        return SBN_ERROR;
    } /* end if */

    if(RemapInitialized)
    {
        OS_printf("SBN_F_Remap Lib already initialized.\n");
        if(Deinit(BaseEID) !=  SBN_SUCCESS) {
            EVSSendErr(BaseEID, "unable to deinitialize SBN_F_Remap");
        }
    }

    OS_Status = OS_MutSemCreate(&RemapMutex, "SBN_F_Remap", 0);
    if (OS_Status != OS_SUCCESS)
    {
        return SBN_ERROR;
    } /* end if */

    if(LoadRemapTbl() != SBN_SUCCESS) {
        EVSSendErr(BaseEID, "unable to load remap table");
    }

    RemapInitialized = true;

    OS_printf("SBN_F_Remap Lib Initialized.\n");

    return SBN_SUCCESS;
} /* end Init() */

SBN_FilterInterface_t SBN_F_Remap = {Init, Remap, Remap, Remap_MID};
