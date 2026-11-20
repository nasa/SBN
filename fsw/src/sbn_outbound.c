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

#include "sbn_error.h"
#include "sbn_app.h"
#include "sbn_outbound.h"
#include "sbn_filter.h"
#include "sbn_worker.h"
#include "sbn_connection.h"

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Marks the peer as ready for outbound data servicing
 *
 *-----------------------------------------------------------------*/
static void SBN_Outbound_AddDataSchedule(SBN_PeerRecord_t *PeerRec)
{
    SBN_Worker_WorkItem_t WorkItem;

    memset(&WorkItem, 0, sizeof(WorkItem));

    WorkItem.JobType           = SBN_Worker_JobType_PEER_OUTBOUND;
    WorkItem.Info.Comms.PeerId = SBN_PeerRecordGetID(PeerRec);
    WorkItem.Info.Comms.NetId  = PeerRec->NetId;

    SBN_Worker_ScheduleJob(&PeerRec->Sched.LastOutboundJobId, &WorkItem);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool SBN_Outbound_IsScheduled(SBN_PeerRecord_t *PeerRec)
{
    return (SBN_Worker_IsJobPending(PeerRec->Sched.LastOutboundJobId));
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Outbound_GetMsg(SBN_BufferDesc_t *BufD, CFE_SB_PipeId_t PeerPipe, int32 Timeout)
{
    const CFE_SB_Buffer_t *SbBufPtr;
    CFE_MSG_Size_t         MsgSz;
    SBN_Status_t           SBN_Status;
    CFE_Status_t           CFE_Status;

    /* if peer data is not in use, go to next peer */
    SbBufPtr   = NULL;
    CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&SbBufPtr, PeerPipe, Timeout);

    if (CFE_Status == CFE_SB_TIME_OUT || CFE_Status == CFE_SB_NO_MESSAGE)
    {
        /* This is OK */
        SBN_Status = SBN_IF_EMPTY;
    }
    else if (CFE_Status != CFE_SUCCESS)
    {
        /* This is a problem */
        SBN_Status = SBN_ERROR;
    }
    else if (CFE_MSG_GetSize(&SbBufPtr->Msg, &MsgSz) != CFE_SUCCESS || MsgSz > BufD->MaxDataSz)
    {
        SBN_Status = SBN_ERROR;
    }
    else
    {
        /* Got a valid buffer and size */

        /* For now simply copy it so we can change/modify the message as needed without violating
         * the const-ness of the software bus buffer.  In the future this should be more integrated
         * with encoding/packing so its not an "extra" copy */
        memcpy(BufD->DataPtr, &SbBufPtr->Msg, MsgSz);
        BufD->ActualDataSz = MsgSz;

        SBN_Status = SBN_SUCCESS;
    }

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Outbound_SendToPeer(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer)
{
    SBN_NetInterface_t *Net;
    SBN_Status_t        SBN_Status;

    Net = Peer->Net;

    SBN_Status = Net->IfOps->Send(Peer, BufD->MsgType, BufD->ActualDataSz, BufD->DataPtr);

    if (SBN_Status != SBN_SUCCESS)
    {
        Peer->Counts.SendErrCnt++;
    }
    else
    {
        Peer->Counts.SendCnt++;
    }

    /* for clients that need a poll or heartbeat, update time even when failing */
    OS_GetLocalTime(&Peer->LastSend);

    /* Reset the buffer because this message was sent */
    BufD->MsgType      = SBN_NO_MSG;
    BufD->ActualDataSz = 0;

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Outbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD, int32 MsgLim)
{
    const CFE_SB_Buffer_t *BufPtr;
    int32                  TimeOut;
    CFE_MSG_Size_t         MsgSz;
    CFE_Status_t           CFE_Status;
    SBN_Status_t           SBN_Status;
    bool                   IsOK;
    SBN_PeerInterface_t   *Peer = SBN_Peer_GetIntfData(PeerRec);

    IsOK   = Peer->Connected;
    BufPtr = NULL;

    /* First take care of connection maintenance - subscription sync and protocol messages */
    /* This is done independently of whether the connection is ready for data */
    SBN_Connection_CheckPeer(PeerRec, BufD);
    if (IsOK)
    {
        SBN_Subs_UpdateConnectedPeerRemoteSubs(PeerRec, BufD);

        /* Confirm that the local pipe is OK before trying to read it.
         * This is not really an error if false, just means the management
         * task has work to do.  Notably this will prevent re-scheduling
         * until the main task runs and does its thing. */
        IsOK = CFE_RESOURCEID_TEST_DEFINED(Peer->Pipe);
    }

    /* Now move on to data passing */
    if (PeerRec->Sched.OutboundAutoReschedule)
    {
        /* Emulate a dedicated task by pending on the pipe, but not forever */
        /* Notably this needs to pause looking for data to check subscriptions */
        TimeOut = SBN_PLATFORM_WORKER_WAIT_TIME_MSEC;
    }
    else
    {
        /* A normal Poll */
        TimeOut = CFE_SB_POLL;
    }

    while (MsgLim > 0 && IsOK)
    {
        --MsgLim;

        CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&BufPtr, Peer->Pipe, TimeOut);
        if (CFE_Status != CFE_SUCCESS)
        {
            /* These two status codes are acceptable, anything else is a problem */
            IsOK = (CFE_Status == CFE_SB_TIME_OUT || CFE_Status == CFE_SB_NO_MESSAGE);
            break;
        }

        if (CFE_MSG_GetSize(&BufPtr->Msg, &MsgSz) != CFE_SUCCESS)
        {
            /* this is not supposed to happen */
            IsOK = false;
        }
        else if (MsgSz <= BufD->MaxDataSz)
        {
            /* We must make a copy of the message before modifying it.
             * TODO: this is highly inefficient - replace with a useful copy */
            memcpy(BufD->DataPtr, &BufPtr->Msg, MsgSz);
            BufD->ActualDataSz = MsgSz;
            BufD->MsgType      = SBN_APP_MSG;

            SBN_Status = SBN_Filter_Mutation(BufD, Peer, SBN_Filter_Direction_OUTBOUND);
            if (SBN_Status == SBN_SUCCESS)
            {
                SBN_Status = SBN_Outbound_SendToPeer(BufD, Peer);

                /* The connection might have been broken, stop trying to send if this fails */
                IsOK = (SBN_Status == SBN_SUCCESS);
            }
        }
    }

    if (PeerRec->Sched.OutboundAutoReschedule && IsOK)
    {
        /* In task mode, immediately reschedule this same job */
        SBN_Outbound_AddDataSchedule(PeerRec);
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Checks for peers that have disconnected and updates state accordingly
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Outbound_DoPollCycle(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    /* Upon initial connection or disconnection, we need to do some internal housekeeping
     * on the SB pipe and its configuration/subscriptions.  When initially connected, the
     * pipe does not exist and needs to be created.  When initially disconnected, the pipe
     * still exists and needs to be removed.  We should not pass outbound data until these
     * two are in agreement, meaning our pipe config has caught up with the connection */

    bool IsPipeValid = CFE_RESOURCEID_TEST_DEFINED(PeerRec->Intf.Pipe);

    /* The actual work is done by helper threads, just schedule it. */
    if (PeerRec->Intf.Connected == IsPipeValid && !SBN_Outbound_IsScheduled(PeerRec))
    {
        SBN_Outbound_AddDataSchedule(PeerRec);
    }

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Outbound_PollCycle(void)
{
    SBN_ForEachConfiguredPeer(SBN_Outbound_DoPollCycle, NULL);
}
