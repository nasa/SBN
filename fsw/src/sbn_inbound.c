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
#include "sbn_error.h"
#include "sbn_app.h"
#include "sbn_worker.h"
#include "sbn_resource.h"
#include "sbn_connection.h"
#include "sbn_filter.h"

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Marks the peer as ready for inbound data servicing
 *
 *-----------------------------------------------------------------*/
static void SBN_Inbound_AddPeerDataSchedule(SBN_PeerRecord_t *PeerRec)
{
    SBN_Worker_WorkItem_t WorkItem;

    memset(&WorkItem, 0, sizeof(WorkItem));

    WorkItem.Info.Comms.NetId  = PeerRec->NetId;
    WorkItem.Info.Comms.PeerId = SBN_PeerRecordGetID(PeerRec);
    WorkItem.JobType           = SBN_Worker_JobType_PEER_INBOUND;

    SBN_Worker_ScheduleJob(&PeerRec->Sched.LastInboundJobId, &WorkItem);
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Marks the net as ready for outbound data servicing
 *
 *-----------------------------------------------------------------*/
static void SBN_Inbound_AddNetDataSchedule(SBN_NetRecord_t *NetRec)
{
    SBN_Worker_WorkItem_t WorkItem;

    memset(&WorkItem, 0, sizeof(WorkItem));

    WorkItem.Info.Comms.NetId = SBN_NetRecordGetID(NetRec);
    WorkItem.JobType          = SBN_Worker_JobType_NET_INBOUND;

    SBN_Worker_ScheduleJob(&NetRec->Sched.LastInboundJobId, &WorkItem);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool SBN_Inbound_IsPeerScheduled(SBN_PeerRecord_t *PeerRec)
{
    return (SBN_Worker_IsJobPending(PeerRec->Sched.LastInboundJobId));
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool SBN_Inbound_IsNetScheduled(SBN_NetRecord_t *NetRec)
{
    return (SBN_Worker_IsJobPending(NetRec->Sched.LastInboundJobId));
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Inbound_ForwardMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    SBN_Status_t         SBN_Status;
    CFE_Status_t         CFE_Status;
    SBN_PeerInterface_t *Peer;

    Peer = SBN_Peer_GetIntfData(PeerRec);

    /* Note- this goes directly to mutation because this is the backward compatible one.
     * In a future update this should change to being a gatekeeper */
    SBN_Status = SBN_Filter_Mutation(BufD, Peer, SBN_Filter_Direction_INBOUND);

    /* includes SBN_IF_EMPTY, for when filter recommends removing */
    if (SBN_Status == SBN_SUCCESS)
    {
        CFE_Status = CFE_SB_TransmitMsg((const CFE_MSG_Message_t *)BufD->DataPtr, false);

        if (CFE_Status != CFE_SUCCESS)
        {
            EVSSendErr(SBN_SB_EID,
                       "CFE_SB_TransmitMsg error (Status=%d MsgType=0x%x)",
                       (int)CFE_Status,
                       (unsigned int)BufD->MsgType);
            SBN_Status = SBN_ERROR;
        } /* end if */
    }

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Inbound_ProcessMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    static const char FAIL_PREFIX[] = "ERROR: could not process peer message:";
    SBN_Status_t      SBN_Status;

    if (!PeerRec)
    {
        EVSSendErr(SBN_PEERTASK_EID, "%s unknown peer (ProcessorID=%d)", FAIL_PREFIX, (int)BufD->ProcessorID);
        return SBN_ERROR;
    } /* end if */

    /* Everything that comes through here (including protocol messages) bumps the recv time */
    OS_GetLocalTime(&PeerRec->Intf.LastRecv);
    PeerRec->Intf.Counts.RecvCnt++;

    /* Check if message is protocol-specific (unkown to SBN core) */
    if (BufD->MsgType & SBN_MODULE_SPECIFIC_MESSAGE_ID_MASK)
    {
        EVSSendDbg(SBN_PEERTASK_EID, "SBN received module-specific message type: 0x%08x", (unsigned int)BufD->MsgType);
        return SBN_SUCCESS;
    }

    switch (BufD->MsgType)
    {
        case SBN_NO_MSG:
            SBN_Status = SBN_SUCCESS;
            break;
        case SBN_PROTO_MSG:
            SBN_Status = SBN_Connection_RecvProtocolVersionMsg(BufD, PeerRec);
            break;
        case SBN_APP_MSG:
            SBN_Status = SBN_Inbound_ForwardMsg(BufD, PeerRec);
            break;
        case SBN_SUB_MSG:
        case SBN_UNSUB_MSG:
            SBN_Status = SBN_Subs_ProcessMsgFromPeer(PeerRec, BufD);
            break;
        default:
            /* Should I generate an event? Probably... */
            EVSSendErr(SBN_PEERTASK_EID, "%s unknown message type 0x%08x", FAIL_PREFIX, (unsigned int)BufD->MsgType);
            SBN_Status = SBN_ERROR;
            break;
    } /* end switch */

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Inbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    SBN_Status_t         Status;
    SBN_PeerInterface_t *Peer;
    SBN_NetInterface_t  *Net;
    bool                 IsOK;
    int32                MsgLim;

    Peer   = SBN_Peer_GetIntfData(PeerRec);
    Net    = Peer->Net;
    IsOK   = Peer->Connected;
    MsgLim = SBN_PLATFORM_MAX_MSGS_PER_WAKEUP;

    EVSSendDbg(SBN_PEERTASK_EID, "Try to receive from peer...");

    while (MsgLim > 0 && IsOK)
    {
        --MsgLim;

        Status = Net->IfOps->RecvFromPeer(Net,
                                          Peer,
                                          &BufD->MsgType,
                                          &BufD->ActualDataSz,
                                          &BufD->ProcessorID,
                                          &BufD->SpacecraftID,
                                          BufD->DataPtr);

        if (Status == SBN_IF_EMPTY)
        {
            /* fine */
            break;
        }
        if (Status != SBN_SUCCESS)
        {
            /* This is a problem */
            Peer->Counts.RecvErrCnt++;
            IsOK = false;
            break;
        }

        SBN_Inbound_ProcessMsg(BufD, PeerRec);
    }

    if (PeerRec->Sched.InboundAutoReschedule && IsOK)
    {
        /* In task mode, immediately reschedule this same job */
        SBN_Inbound_AddPeerDataSchedule(PeerRec);
    }
    else if (!IsOK)
    {
        /* send an event as this is not expected */
        EVSSendErr(SBN_PEERTASK_EID,
                   "%s failed for peer %d: status=%d",
                   __func__,
                   SBN_PeerId_AsInt(SBN_PeerRecordGetID(PeerRec)),
                   (int)Status);
    }
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Inbound_ServiceNet(SBN_NetRecord_t *NetRec, SBN_BufferDesc_t *BufD)
{
    SBN_Status_t        Status;
    SBN_PeerRecord_t   *PeerRec;
    SBN_NetInterface_t *Net;
    bool                IsOK;
    int32               MsgLim;

    Net    = SBN_Net_GetIntfData(NetRec);
    IsOK   = true;
    MsgLim = SBN_PLATFORM_MAX_MSGS_PER_WAKEUP;

    EVSSendDbg(SBN_PEERTASK_EID, "Try to receive from net...");

    while (MsgLim > 0 && IsOK)
    {
        --MsgLim;

        Status = Net->IfOps->RecvFromNet(Net,
                                         &BufD->MsgType,
                                         &BufD->ActualDataSz,
                                         &BufD->ProcessorID,
                                         &BufD->SpacecraftID,
                                         BufD->DataPtr);

        /* Note - there is not a counter at the net level */
        if (Status != SBN_SUCCESS)
        {
            /* This is the acceptable status, anything else is a problem */
            IsOK = (Status == SBN_IF_EMPTY);
            break;
        }

        /* Note - ProcessPeerMsg handles an unknown (NULL) peer */
        PeerRec = SBN_GetPeerRecord(NetRec, BufD->ProcessorID, BufD->SpacecraftID);
        SBN_Inbound_ProcessMsg(BufD, PeerRec);
    }

    if (NetRec->Sched.InboundAutoReschedule && IsOK)
    {
        /* In task mode, immediately reschedule this same job */
        SBN_Inbound_AddNetDataSchedule(NetRec);
    }
    else if (!IsOK)
    {
        /* this should generally not happen on properly configured nets */
        EVSSendErr(SBN_PEERTASK_EID,
                   "%s failed for net %d: status=%d",
                   __func__,
                   SBN_NetId_AsInt(SBN_NetRecordGetID(NetRec)),
                   (int)Status);
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Checks if the net is ready for inbound data servicing
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_DoInboundNetPollCycle(SBN_NetRecord_t *NetRec, void *Arg)
{
    if (NetRec->Sched.InboundSchedulingEnabled && !SBN_Inbound_IsNetScheduled(NetRec))
    {
        SBN_Inbound_AddNetDataSchedule(NetRec);
    }

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Checks if the peer is ready for inbound data servicing
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_DoInboundPeerPollCycle(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    if (PeerRec->Sched.InboundSchedulingEnabled && !SBN_Inbound_IsPeerScheduled(PeerRec))
    {
        SBN_Inbound_AddPeerDataSchedule(PeerRec);
    }

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Inbound_PollCycle(void)
{
    SBN_ForEachConfiguredNet(SBN_DoInboundNetPollCycle, NULL);
    SBN_ForEachConfiguredPeer(SBN_DoInboundPeerPollCycle, NULL);
}
