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

/*
 ** Include Files
 */
#include <string.h>

#include "sbn_app.h"
#include "sbn_error.h"
#include "sbn_resource.h"
#include "cfe_resourceid_basevalue.h"

enum
{
    /* SBN managed resources */
    SBN_PEERID_BASE = CFE_RESOURCEID_MAKE_BASE(OS_OBJECT_TYPE_USER + 37),
    SBN_NETID_BASE  = CFE_RESOURCEID_MAKE_BASE(OS_OBJECT_TYPE_USER + 38),
};

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t SBN_NetId_ToIndex(SBN_NetId_t NetId, uint32 *Idx)
{
    return CFE_ResourceId_ToIndex(CFE_RESOURCEID_UNWRAP(NetId), SBN_NETID_BASE, SBN_PLATFORM_MAX_NETS, Idx);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t SBN_PeerId_ToIndex(SBN_PeerId_t PeerId, uint32 *Idx)
{
    return CFE_ResourceId_ToIndex(CFE_RESOURCEID_UNWRAP(PeerId), SBN_PEERID_BASE, SBN_MISSION_MAX_PEER_CNT, Idx);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_NetRecord_t *SBN_LocateNetRecordByID(SBN_NetId_t NetID)
{
    SBN_NetRecord_t *NetRecPtr;
    uint32           Idx;

    if (SBN_NetId_ToIndex(NetID, &Idx) == CFE_SUCCESS)
    {
        NetRecPtr = &SBN_AppData.NetRecords[Idx];
    }
    else
    {
        NetRecPtr = NULL;
    }

    return NetRecPtr;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_PeerRecord_t *SBN_LocatePeerRecordByID(SBN_PeerId_t PeerId)
{
    SBN_PeerRecord_t *PeerRecPtr;
    uint32            Idx;

    if (SBN_PeerId_ToIndex(PeerId, &Idx) == CFE_SUCCESS)
    {
        PeerRecPtr = &SBN_AppData.PeerRecords[Idx];
    }
    else
    {
        PeerRecPtr = NULL;
    }

    return PeerRecPtr;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool SBN_CheckNetIdSlotUsed(CFE_ResourceId_t CheckId)
{
    SBN_NetRecord_t *NetRecPtr;
    /*
     * Note - The pointer here should never be NULL because the ID should always be
     * within the expected range, but if it ever is NULL, this should return true
     * such that the caller will _not_ attempt to use the record.
     */
    NetRecPtr = SBN_LocateNetRecordByID(SBN_NETID_C(CheckId));
    return (NetRecPtr == NULL || SBN_NetRecordIsUsed(NetRecPtr));
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool SBN_CheckPeerIdSlotUsed(CFE_ResourceId_t CheckId)
{
    SBN_PeerRecord_t *PeerRecPtr;
    /*
     * Note - The pointer here should never be NULL because the ID should always be
     * within the expected range, but if it ever is NULL, this should return true
     * such that the caller will _not_ attempt to use the record.
     */
    PeerRecPtr = SBN_LocatePeerRecordByID(SBN_PEERID_C(CheckId));
    return (PeerRecPtr == NULL || SBN_PeerRecordIsUsed(PeerRecPtr));
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_RegisterPeer(SBN_PeerId_t *PeerIdOut)
{
    SBN_PeerRecord_t *PeerRecPtr;
    SBN_Status_t      Status;
    CFE_ResourceId_t  PendingResourceId;

    /* NOTE: Only this SBN thread allocates anything - no lock needed. */

    if (!CFE_ResourceId_IsDefined(SBN_AppData.LastPeerId))
    {
        SBN_AppData.LastPeerId = CFE_ResourceId_FromInteger(SBN_PEERID_BASE + CFE_RESOURCEID_MAX);
    }

    /* scan for a free slot */
    PendingResourceId =
        CFE_ResourceId_FindNext(SBN_AppData.LastPeerId, SBN_MISSION_MAX_PEER_CNT, SBN_CheckPeerIdSlotUsed);

    PeerRecPtr = SBN_LocatePeerRecordByID(SBN_PEERID_C(PendingResourceId));

    if (PeerRecPtr == NULL)
    {
        CFE_ES_WriteToSysLog("%s: No free peer slots available\n", __func__);

        *PeerIdOut = SBN_PEERID_UNDEFINED;
        Status     = SBN_ERROR;
    }
    else
    {
        SBN_PeerRecordSetUsed(PeerRecPtr, PendingResourceId);
        SBN_AppData.LastPeerId = PendingResourceId;

        *PeerIdOut = SBN_PEERID_C(PendingResourceId);
        Status     = SBN_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_RegisterNet(SBN_NetId_t *NetIdOut)
{
    SBN_NetRecord_t *NetRecPtr;
    SBN_Status_t     Status;
    CFE_ResourceId_t PendingResourceId;

    /* NOTE: Only this SBN thread allocates anything - no lock needed. */

    if (!CFE_ResourceId_IsDefined(SBN_AppData.LastNetId))
    {
        SBN_AppData.LastNetId = CFE_ResourceId_FromInteger(SBN_NETID_BASE + CFE_RESOURCEID_MAX);
    }

    /* scan for a free slot */
    PendingResourceId = CFE_ResourceId_FindNext(SBN_AppData.LastNetId, SBN_PLATFORM_MAX_NETS, SBN_CheckNetIdSlotUsed);

    NetRecPtr = SBN_LocateNetRecordByID(SBN_NETID_C(PendingResourceId));

    if (NetRecPtr == NULL)
    {
        CFE_ES_WriteToSysLog("%s: No free net slots available\n", __func__);

        *NetIdOut = SBN_NETID_UNDEFINED;
        Status    = SBN_ERROR;
    }
    else
    {
        SBN_NetRecordSetUsed(NetRecPtr, PendingResourceId);
        SBN_AppData.LastNetId = PendingResourceId;

        *NetIdOut = SBN_NETID_C(PendingResourceId);
        Status    = SBN_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_NetRecord_t *SBN_LocateNetRecordByExtIdx(SBN_NetIdx_t Idx)
{
    SBN_NetRecord_t *NetRecPtr;
    SBN_NetId_t      NetId;

    /* Look up the index in the backward compatibility map */
    if (Idx < SBN_AppData.NetCnt)
    {
        NetId = SBN_AppData.NetIds[Idx];
    }
    else
    {
        NetId = SBN_NETID_UNDEFINED;
    }

    NetRecPtr = SBN_LocateNetRecordByID(NetId);
    if (!SBN_NetRecordIsMatch(NetRecPtr, NetId))
    {
        /* this is not a valid net index */
        NetRecPtr = NULL;
    }

    return NetRecPtr;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_PeerRecord_t *SBN_LocatePeerRecordInNet(SBN_NetRecord_t *NetRec, SBN_PeerIdx_t Idx)
{
    SBN_PeerRecord_t   *PeerRecPtr;
    SBN_NetInterface_t *NetIf;
    SBN_PeerId_t        PeerId;

    if (NetRec == NULL)
    {
        /* To simplify use in compound lookups, make this a silent error */
        return NULL;
    }

    NetIf = SBN_Net_GetIntfData(NetRec);

    /* Note that the netif + peer lookup is often combined, so its easier to pass thru a prior error */
    if (Idx < NetIf->PeerCnt)
    {
        PeerId = NetIf->PeerIds[Idx];
    }
    else
    {
        PeerId = SBN_PEERID_UNDEFINED;
    }

    PeerRecPtr = SBN_LocatePeerRecordByID(PeerId);
    if (!SBN_PeerRecordIsMatch(PeerRecPtr, PeerId))
    {
        PeerRecPtr = NULL;
    }

    return PeerRecPtr;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_ForEachConfiguredPeer(SBN_PeerFunc_t Func, void *Arg)
{
    SBN_PeerIdx_t     PeerIdx;
    SBN_PeerRecord_t *PeerRec;
    SBN_Status_t      Status;

    Status = SBN_SUCCESS;
    for (PeerIdx = 0; Status == SBN_SUCCESS && PeerIdx < SBN_MISSION_MAX_PEER_CNT; ++PeerIdx)
    {
        PeerRec = &SBN_AppData.PeerRecords[PeerIdx];

        if (CFE_RESOURCEID_TEST_DEFINED(PeerRec->PeerId))
        {
            Status = Func(PeerRec, Arg);
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_ForEachConfiguredNet(SBN_NetFunc_t Func, void *Arg)
{
    SBN_NetIdx_t     NetIdx;
    SBN_NetRecord_t *NetRec;
    SBN_Status_t     Status;

    Status = SBN_SUCCESS;
    for (NetIdx = 0; Status == SBN_SUCCESS && NetIdx < SBN_MISSION_MAX_PEER_CNT; ++NetIdx)
    {
        NetRec = &SBN_AppData.NetRecords[NetIdx];

        if (CFE_RESOURCEID_TEST_DEFINED(NetRec->NetId))
        {
            Status = Func(NetRec, Arg);
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int SBN_NetId_AsInt(SBN_NetId_t NetId)
{
    uint32 Idx;

    if (SBN_NetId_ToIndex(NetId, &Idx) != CFE_SUCCESS)
    {
        return -1;
    }

    return (int)Idx;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int SBN_PeerId_AsInt(SBN_PeerId_t PeerId)
{
    uint32 Idx;

    if (SBN_PeerId_ToIndex(PeerId, &Idx) != CFE_SUCCESS)
    {
        return -1;
    }

    return (int)Idx;
}
