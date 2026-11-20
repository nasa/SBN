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
 * Auto-Generated stub implementations for functions defined in sbn_inbound header
 */

#include "sbn_inbound.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Inbound_ForwardMsg()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Inbound_ForwardMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Inbound_ForwardMsg, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Inbound_ForwardMsg, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Inbound_ForwardMsg, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Inbound_ForwardMsg, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Inbound_ForwardMsg, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Inbound_IsNetScheduled()
 * ----------------------------------------------------
 */
bool SBN_Inbound_IsNetScheduled(SBN_NetRecord_t *NetRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Inbound_IsNetScheduled, bool);

    UT_GenStub_AddParam(SBN_Inbound_IsNetScheduled, SBN_NetRecord_t *, NetRec);

    UT_GenStub_Execute(SBN_Inbound_IsNetScheduled, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Inbound_IsNetScheduled, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Inbound_IsPeerScheduled()
 * ----------------------------------------------------
 */
bool SBN_Inbound_IsPeerScheduled(SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Inbound_IsPeerScheduled, bool);

    UT_GenStub_AddParam(SBN_Inbound_IsPeerScheduled, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Inbound_IsPeerScheduled, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Inbound_IsPeerScheduled, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Inbound_PollCycle()
 * ----------------------------------------------------
 */
void SBN_Inbound_PollCycle(void)
{
    UT_GenStub_Execute(SBN_Inbound_PollCycle, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Inbound_ProcessMsg()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Inbound_ProcessMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Inbound_ProcessMsg, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Inbound_ProcessMsg, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Inbound_ProcessMsg, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Inbound_ProcessMsg, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Inbound_ProcessMsg, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Inbound_ServiceNet()
 * ----------------------------------------------------
 */
void SBN_Inbound_ServiceNet(SBN_NetRecord_t *NetRec, SBN_BufferDesc_t *BufD, int32 MsgLim)
{
    UT_GenStub_AddParam(SBN_Inbound_ServiceNet, SBN_NetRecord_t *, NetRec);
    UT_GenStub_AddParam(SBN_Inbound_ServiceNet, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Inbound_ServiceNet, int32, MsgLim);

    UT_GenStub_Execute(SBN_Inbound_ServiceNet, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Inbound_ServicePeer()
 * ----------------------------------------------------
 */
void SBN_Inbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD, int32 MsgLim)
{
    UT_GenStub_AddParam(SBN_Inbound_ServicePeer, SBN_PeerRecord_t *, PeerRec);
    UT_GenStub_AddParam(SBN_Inbound_ServicePeer, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Inbound_ServicePeer, int32, MsgLim);

    UT_GenStub_Execute(SBN_Inbound_ServicePeer, Basic, NULL);
}
