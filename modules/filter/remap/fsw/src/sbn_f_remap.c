/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include "sbn_interfaces.h"
#include "sbn_remap_tbl.h"
#include "sbn_msgids.h"
#include "cfe.h"
#include "cfe_tbl.h"
#include <string.h> /* memcpy */
#include <stdlib.h> /* qsort */

#include "sbn_f_remap_events.h"
#include "sbn_error.h"

static const char SBN_F_REMAP_TABLE_NAME[] = "RemapTbl";

typedef struct SBN_Remap_Global
{
    bool             Initialized;
    bool             EnablePassThru;
    osal_id_t        Mutex;
    CFE_TBL_Handle_t TblHandle;
    SBN_RemapTbl_t  *TblPtr;
    uint32           TblCnt;
} SBN_Remap_Global_t;

SBN_Remap_Global_t SBN_Remap_Global;

SBN_EventID_t SBN_F_REMAP_FIRST_EID;

static int SBN_Remap_TblValidate(void *TblPtr)
{
    SBN_RemapTbl_t *r = (SBN_RemapTbl_t *)TblPtr;

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

    return 0;
}

static int SBN_Remap_CompareEntry(const void *a, const void *b)
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
    return CFE_SB_MsgIdToValue(aEntry->FromMID) - CFE_SB_MsgIdToValue(bEntry->FromMID);
}

static SBN_Status_t SBN_Remap_RegisterTable(void)
{
    CFE_Status_t CFE_Status;

    CFE_Status = CFE_TBL_Register(&SBN_Remap_Global.TblHandle,
                                  SBN_F_REMAP_TABLE_NAME,
                                  sizeof(SBN_RemapTbl_t),
                                  CFE_TBL_OPT_DEFAULT,
                                  SBN_Remap_TblValidate);

    if (CFE_Status < CFE_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to register remap tbl handle");
        return SBN_ERROR;
    } /* end if */

    CFE_Status = CFE_TBL_Load(SBN_Remap_Global.TblHandle, CFE_TBL_SRC_FILE, SBN_PLATFORM_REMAP_TBL_FILENAME);
    if (CFE_Status < CFE_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to load remap tbl %s", SBN_PLATFORM_REMAP_TBL_FILENAME);
        /* Do not unregister - it can be loaded later */
        return SBN_ERROR;
    } /* end if */

    EVSSendDbg(SBN_F_REMAP_TBL_EID, "remap table loaded");

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_Remap_ReadTable(void)
{
    CFE_Status_t CFE_Status;
    void        *TblPtr = NULL;

    SBN_Remap_Global.TblCnt = 0;

    CFE_Status = CFE_TBL_GetAddress(&TblPtr, SBN_Remap_Global.TblHandle);
    if (CFE_Status < CFE_SUCCESS)
    {
        /* Enables passthru mode or else this will just block all traffic */
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to get conf table address");
        SBN_Remap_Global.TblPtr = NULL;
        return SBN_ERROR;
    } /* end if */

    SBN_Remap_Global.TblPtr = TblPtr;

    /* Find the first "empty" entry to determine table size. */
    while (SBN_Remap_Global.TblCnt < SBN_REMAP_TABLE_SIZE)
    {
        if (!CFE_SB_IsValidMsgId(SBN_Remap_Global.TblPtr->Entries[SBN_Remap_Global.TblCnt].FromMID))
        {
            break;
        }

        ++SBN_Remap_Global.TblCnt;
    }

    SBN_Remap_Global.EnablePassThru = (SBN_Remap_Global.TblPtr->RemapDefaultFlag == SBN_REMAP_DEFAULT_SEND);

    EVSSendDbg(SBN_F_REMAP_TBL_EID, "remap table address retrieved");
    /* sort the entries on <ProcessorID> and <from MID> */
    /* note: qsort is recursive, so it will use some stack space
     * (O[N log N] * <some small amount of stack>). If this is a concern,
     * consider using a non-recursive (insertion, bubble, etc) sort algorithm.
     */

    qsort(SBN_Remap_Global.TblPtr->Entries,
          SBN_Remap_Global.TblCnt,
          sizeof(SBN_RemapTblEntry_t),
          SBN_Remap_CompareEntry);
    EVSSendDbg(SBN_F_REMAP_TBL_EID, "remap table sorted");

    CFE_TBL_Modified(SBN_Remap_Global.TblHandle);

    return SBN_SUCCESS;
}

/* finds the entry or the one that would immediately follow it */
static void *SBN_Remap_BinarySearch(void  *Entries,
                                    void  *SearchEntry,
                                    size_t EntryCnt,
                                    size_t EntrySz,
                                    int (*EntryCompare)(const void *, const void *))
{
    int32 start;
    int32 end;
    int32 midpoint;
    int32 c;
    void *EntryPtr;

    EntryPtr = NULL;
    start    = 0;
    end      = (int)EntryCnt - 1;

    while (start <= end)
    {
        midpoint = (end + start) / 2;
        EntryPtr = (uint8 *)Entries + (EntrySz * midpoint);
        c        = EntryCompare(SearchEntry, EntryPtr);
        if (c == 0)
        {
            break;
        }

        EntryPtr = NULL;
        if (c > 0)
        {
            start = midpoint + 1;
        }
        else
        {
            end = midpoint - 1;
        } /* end if */
    } /* end while */

    return EntryPtr;
}

static SBN_RemapTblEntry_t *SBN_Remap_TblSearch(uint32 ProcessorID, uint32 SpacecraftID, CFE_SB_MsgId_t MID)
{
    SBN_RemapTblEntry_t *Result;
    SBN_RemapTblEntry_t  Entry = { ProcessorID, SpacecraftID, MID, { 0 } };

    if (SBN_Remap_Global.TblPtr != NULL)
    {
        Result = SBN_Remap_BinarySearch(SBN_Remap_Global.TblPtr->Entries,
                                        &Entry,
                                        SBN_Remap_Global.TblCnt,
                                        sizeof(SBN_RemapTblEntry_t),
                                        SBN_Remap_CompareEntry);
    }
    else
    {
        Result = NULL;
    }

    return Result;
}

static SBN_Status_t SBN_Remap_ProcessMsg(void *msg, SBN_Filter_Ctx_t *Context)
{
    CFE_SB_MsgId_t       FromMID = CFE_SB_INVALID_MSG_ID;
    CFE_SB_MsgId_t       ToMID   = CFE_SB_INVALID_MSG_ID;
    SBN_RemapTblEntry_t *RemapEntry;
    CFE_MSG_Message_t   *CFE_MsgPtr = msg;

    if (CFE_MSG_GetMsgId(CFE_MsgPtr, &FromMID) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_EID, "unable to get apid");
        return SBN_ERROR;
    } /* end if */

    if (OS_MutSemTake(SBN_Remap_Global.Mutex) != OS_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_EID, "unable to take mutex");
        return SBN_ERROR;
    } /* end if */

    RemapEntry = SBN_Remap_TblSearch(Context->PeerProcessorID, Context->PeerSpacecraftID, FromMID);
    if (RemapEntry != NULL)
    {
        ToMID = RemapEntry->ToMID;
    }
    else if (SBN_Remap_Global.EnablePassThru)
    {
        ToMID = FromMID;
    }

    if (OS_MutSemGive(SBN_Remap_Global.Mutex) != OS_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_EID, "unable to give mutex");
        return SBN_ERROR;
    } /* end if */

    if (!CFE_SB_IsValidMsgId(ToMID))
    {
        return SBN_IF_EMPTY; /* signal to the core app that this filter recommends not sending this message */
    } /* end if */

    if (CFE_MSG_SetMsgId(CFE_MsgPtr, ToMID) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_F_REMAP_EID, "unable to set msgid");
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_Remap_ProcessMID(CFE_SB_MsgId_t *InOutMsgIdPtr, SBN_Filter_Ctx_t *Context)
{
    SBN_RemapTblEntry_t *RemapEntry;

    EVSSendDbg(SBN_F_REMAP_TBL_EID, "Remap check 0x%04X", CFE_SB_MsgIdToValue(*InOutMsgIdPtr));

    RemapEntry = SBN_Remap_TblSearch(Context->PeerProcessorID, Context->PeerSpacecraftID, *InOutMsgIdPtr);
    if (RemapEntry != NULL)
    {
        *InOutMsgIdPtr = RemapEntry->FromMID;
        EVSSendDbg(SBN_F_REMAP_TBL_EID, " --> 0x%04X", CFE_SB_MsgIdToValue(*InOutMsgIdPtr));
    }

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_Remap_UnloadTbl(void)
{
    //  Check the handle to a previously-registered table
    if (CFE_TBL_GetStatus(SBN_Remap_Global.TblHandle) == CFE_SUCCESS)
    {
        // Unregister and free resources used by table
        if (CFE_TBL_Unregister(SBN_Remap_Global.TblHandle) != CFE_SUCCESS)
        {
            EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to unregister table");
        }
        SBN_Remap_Global.TblHandle = CFE_TBL_BAD_TABLE_HANDLE;
        SBN_Remap_Global.TblPtr    = NULL;
    }
    else
    {
        EVSSendErr(SBN_F_REMAP_TBL_EID, "unable to get registered table");
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_Remap_Deinit(SBN_EventID_t BaseEID)
{
    if (OS_MutSemTake(SBN_Remap_Global.Mutex) != OS_SUCCESS)
    {
        EVSSendErr(BaseEID, "unable to take mutex");
        return SBN_ERROR;
    } /* end if */

    if (SBN_Remap_UnloadTbl() != SBN_SUCCESS)
    {
        EVSSendErr(BaseEID, "unable to unload table");
        return SBN_ERROR;
    }

    if (OS_MutSemGive(SBN_Remap_Global.Mutex) != OS_SUCCESS)
    {
        EVSSendErr(BaseEID, "unable to give mutex");
        return SBN_ERROR;
    } /* end if */

    if (OS_ObjectIdDefined(SBN_Remap_Global.Mutex))
    {
        if (OS_MutSemDelete(SBN_Remap_Global.Mutex) != OS_SUCCESS)
        {
            EVSSendErr(BaseEID, "unable to delete mutex");
            return SBN_ERROR;
        }
        SBN_Remap_Global.Mutex = OS_OBJECT_ID_UNDEFINED;
    }

    SBN_Remap_Global.Initialized = false;

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_Remap_Init(int Version, SBN_EventID_t BaseEID)
{
    CFE_ES_TaskInfo_t TaskInfo;
    OS_Status_t       OS_Status;
    CFE_Status_t      CFE_Status;

    SBN_Remap_Global.Mutex = OS_OBJECT_ID_UNDEFINED;

    SBN_F_REMAP_FIRST_EID = BaseEID;

    if (Version != 2) /* TODO: define */
    {
        EVSSendErr(BaseEID, "version mismatch: expected %d, got %d", 1, Version);
        return SBN_ERROR;
    } /* end if */

    /* get task name to retrieve table after loading */
    CFE_ES_TaskId_t TskId;
    CFE_ES_GetTaskID(&TskId);
    if ((CFE_Status = CFE_ES_GetTaskInfo(&TaskInfo, TskId)) != CFE_SUCCESS)
    {
        EVSSendErr(BaseEID, "SBN failed to get task info (%d)", (int)CFE_Status);
        return SBN_ERROR;
    } /* end if */

    if (SBN_Remap_Global.Initialized)
    {
        EVSSendErr(BaseEID, "already initialized.");
        if (SBN_Remap_Deinit(BaseEID) != SBN_SUCCESS)
        {
            EVSSendErr(BaseEID, "unable to deinitialize SBN_F_Remap");
        }
    }

    OS_Status = OS_MutSemCreate(&SBN_Remap_Global.Mutex, "SBN_F_Remap", 0);
    if (OS_Status != OS_SUCCESS)
    {
        return SBN_ERROR;
    } /* end if */

    if (SBN_Remap_RegisterTable() != SBN_SUCCESS)
    {
        EVSSendErr(BaseEID, "unable to register remap table");
        SBN_Remap_Global.EnablePassThru = true; /* TBD */
    }
    else if (SBN_Remap_ReadTable() != SBN_SUCCESS)
    {
        EVSSendErr(BaseEID, "unable to read remap table");
        SBN_Remap_Global.EnablePassThru = true; /* TBD */
    }

    SBN_Remap_Global.Initialized = true;

    EVSSendInfo(BaseEID, "SBN_F_Remap Lib Initialized.");

    return SBN_SUCCESS;
} /* end Init() */

SBN_FilterInterface_t SBN_F_Remap = { SBN_Remap_Init,
                                      SBN_Remap_ProcessMsg,
                                      SBN_Remap_ProcessMsg,
                                      SBN_Remap_ProcessMID };
