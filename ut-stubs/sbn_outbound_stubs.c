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
 * Auto-Generated stub implementations for functions defined in sbn_outbound header
 */

#include "sbn_outbound.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Outbound_GetMsg()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Outbound_GetMsg(SBN_BufferDesc_t *BufD, CFE_SB_PipeId_t PeerPipe, int32 Timeout)
{
    UT_GenStub_SetupReturnBuffer(SBN_Outbound_GetMsg, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Outbound_GetMsg, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Outbound_GetMsg, CFE_SB_PipeId_t, PeerPipe);
    UT_GenStub_AddParam(SBN_Outbound_GetMsg, int32, Timeout);

    UT_GenStub_Execute(SBN_Outbound_GetMsg, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Outbound_GetMsg, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Outbound_IsScheduled()
 * ----------------------------------------------------
 */
bool SBN_Outbound_IsScheduled(SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Outbound_IsScheduled, bool);

    UT_GenStub_AddParam(SBN_Outbound_IsScheduled, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Outbound_IsScheduled, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Outbound_IsScheduled, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Outbound_PollCycle()
 * ----------------------------------------------------
 */
void SBN_Outbound_PollCycle(void)
{
    UT_GenStub_Execute(SBN_Outbound_PollCycle, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Outbound_SendToPeer()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Outbound_SendToPeer(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer)
{
    UT_GenStub_SetupReturnBuffer(SBN_Outbound_SendToPeer, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Outbound_SendToPeer, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Outbound_SendToPeer, SBN_PeerInterface_t *, Peer);

    UT_GenStub_Execute(SBN_Outbound_SendToPeer, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Outbound_SendToPeer, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Outbound_ServicePeer()
 * ----------------------------------------------------
 */
void SBN_Outbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD, int32 MsgLim)
{
    UT_GenStub_AddParam(SBN_Outbound_ServicePeer, SBN_PeerRecord_t *, PeerRec);
    UT_GenStub_AddParam(SBN_Outbound_ServicePeer, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Outbound_ServicePeer, int32, MsgLim);

    UT_GenStub_Execute(SBN_Outbound_ServicePeer, Basic, NULL);
}
