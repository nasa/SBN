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
 * Auto-Generated stub implementations for functions defined in sbn_connection header
 */

#include "sbn_connection.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Connection_CheckPeer()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Connection_CheckPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    UT_GenStub_SetupReturnBuffer(SBN_Connection_CheckPeer, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Connection_CheckPeer, SBN_PeerRecord_t *, PeerRec);
    UT_GenStub_AddParam(SBN_Connection_CheckPeer, SBN_BufferDesc_t *, BufD);

    UT_GenStub_Execute(SBN_Connection_CheckPeer, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Connection_CheckPeer, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Connection_HandleConnect()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Connection_HandleConnect(SBN_PeerInterface_t *Peer)
{
    UT_GenStub_SetupReturnBuffer(SBN_Connection_HandleConnect, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Connection_HandleConnect, SBN_PeerInterface_t *, Peer);

    UT_GenStub_Execute(SBN_Connection_HandleConnect, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Connection_HandleConnect, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Connection_HandleDisconnect()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Connection_HandleDisconnect(SBN_PeerInterface_t *Peer)
{
    UT_GenStub_SetupReturnBuffer(SBN_Connection_HandleDisconnect, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Connection_HandleDisconnect, SBN_PeerInterface_t *, Peer);

    UT_GenStub_Execute(SBN_Connection_HandleDisconnect, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Connection_HandleDisconnect, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Connection_PollCycle()
 * ----------------------------------------------------
 */
void SBN_Connection_PollCycle(void)
{
    UT_GenStub_Execute(SBN_Connection_PollCycle, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Connection_RecvProtocolVersionMsg()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Connection_RecvProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Connection_RecvProtocolVersionMsg, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Connection_RecvProtocolVersionMsg, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Connection_RecvProtocolVersionMsg, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Connection_RecvProtocolVersionMsg, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Connection_RecvProtocolVersionMsg, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Connection_SendProtocolVersionMsg()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Connection_SendProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_SetupReturnBuffer(SBN_Connection_SendProtocolVersionMsg, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Connection_SendProtocolVersionMsg, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Connection_SendProtocolVersionMsg, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_Connection_SendProtocolVersionMsg, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Connection_SendProtocolVersionMsg, SBN_Status_t);
}
