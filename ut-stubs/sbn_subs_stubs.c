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

/**
 * @file
 *
 * Auto-Generated stub implementations for functions defined in sbn_subs header
 */

#include "sbn_subs.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_SendSubsRequests()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_SendSubsRequests(void)
{
    UT_GenStub_SetupReturnBuffer(SBN_SendSubsRequests, SBN_Status_t);

    UT_GenStub_Execute(SBN_SendSubsRequests, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_SendSubsRequests, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_GetPrevChangeEntry()
 * ----------------------------------------------------
 */
SBN_SubChange_t *SBN_Subs_GetPrevChangeEntry(SBN_SubJournal_t *Journal, uint32 Version)
{
    UT_GenStub_SetupReturnBuffer(SBN_Subs_GetPrevChangeEntry, SBN_SubChange_t *);

    UT_GenStub_AddParam(SBN_Subs_GetPrevChangeEntry, SBN_SubJournal_t *, Journal);
    UT_GenStub_AddParam(SBN_Subs_GetPrevChangeEntry, uint32, Version);

    UT_GenStub_Execute(SBN_Subs_GetPrevChangeEntry, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Subs_GetPrevChangeEntry, SBN_SubChange_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_PollCycle()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Subs_PollCycle(void)
{
    UT_GenStub_SetupReturnBuffer(SBN_Subs_PollCycle, SBN_Status_t);

    UT_GenStub_Execute(SBN_Subs_PollCycle, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Subs_PollCycle, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_ProcessMsgFromPeer()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Subs_ProcessMsgFromPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    UT_GenStub_SetupReturnBuffer(SBN_Subs_ProcessMsgFromPeer, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Subs_ProcessMsgFromPeer, SBN_PeerRecord_t *, PeerRec);
    UT_GenStub_AddParam(SBN_Subs_ProcessMsgFromPeer, SBN_BufferDesc_t *, BufD);

    UT_GenStub_Execute(SBN_Subs_ProcessMsgFromPeer, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Subs_ProcessMsgFromPeer, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_RegisterChange()
 * ----------------------------------------------------
 */
void SBN_Subs_RegisterChange(SBN_SubJournal_t *Journal, bool IsSubscribe, CFE_SB_MsgId_t MsgId, CFE_SB_Qos_t QoS)
{
    UT_GenStub_AddParam(SBN_Subs_RegisterChange, SBN_SubJournal_t *, Journal);
    UT_GenStub_AddParam(SBN_Subs_RegisterChange, bool, IsSubscribe);
    UT_GenStub_AddParam(SBN_Subs_RegisterChange, CFE_SB_MsgId_t, MsgId);
    UT_GenStub_AddParam(SBN_Subs_RegisterChange, CFE_SB_Qos_t, QoS);

    UT_GenStub_Execute(SBN_Subs_RegisterChange, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_SendLocalToPeer()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Subs_SendLocalToPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    UT_GenStub_SetupReturnBuffer(SBN_Subs_SendLocalToPeer, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Subs_SendLocalToPeer, SBN_PeerRecord_t *, PeerRec);
    UT_GenStub_AddParam(SBN_Subs_SendLocalToPeer, SBN_BufferDesc_t *, BufD);

    UT_GenStub_Execute(SBN_Subs_SendLocalToPeer, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Subs_SendLocalToPeer, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_SetupLocalPipe()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Subs_SetupLocalPipe(SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Subs_SetupLocalPipe, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Subs_SetupLocalPipe, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Subs_SetupLocalPipe, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Subs_SetupLocalPipe, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_SynchronizeAllPeers()
 * ----------------------------------------------------
 */
void SBN_Subs_SynchronizeAllPeers(void)
{
    UT_GenStub_Execute(SBN_Subs_SynchronizeAllPeers, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_UpdateConnectedPeerLocalSubs()
 * ----------------------------------------------------
 */
void SBN_Subs_UpdateConnectedPeerLocalSubs(SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_AddParam(SBN_Subs_UpdateConnectedPeerLocalSubs, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Subs_UpdateConnectedPeerLocalSubs, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_UpdateConnectedPeerRemoteSubs()
 * ----------------------------------------------------
 */
void SBN_Subs_UpdateConnectedPeerRemoteSubs(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    UT_GenStub_AddParam(SBN_Subs_UpdateConnectedPeerRemoteSubs, SBN_PeerRecord_t *, PeerRec);
    UT_GenStub_AddParam(SBN_Subs_UpdateConnectedPeerRemoteSubs, SBN_BufferDesc_t *, BufD);

    UT_GenStub_Execute(SBN_Subs_UpdateConnectedPeerRemoteSubs, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Subs_UpdateDisconnectedPeerLocalSubs()
 * ----------------------------------------------------
 */
void SBN_Subs_UpdateDisconnectedPeerLocalSubs(SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_AddParam(SBN_Subs_UpdateDisconnectedPeerLocalSubs, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Subs_UpdateDisconnectedPeerLocalSubs, Basic, NULL);
}
