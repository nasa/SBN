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

#include "sbn_coveragetest_common.h"
#include "sbn_app.h"
#include "sbn_connection.h"
#include "sbn_error.h"
#include "sbn_outbound.h"

static SBN_Status_t SBN_UT_PollPeer_Err(SBN_PeerInterface_t *Peer)
{
    return SBN_ERROR;
}

void Test_SBN_Connection_SendProtocolVersionMsg_SendErr(void)
{
    START();

    /* Test case for:
     * SBN_Status_t SBN_Connection_SendProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
     */
    uint8            Buf[16];
    SBN_BufferDesc_t BufD;
    memset(&BufD, 0, sizeof(BufD));

    BufD.DataPtr   = Buf;
    BufD.MaxDataSz = sizeof(Buf);

    UT_SetDefaultReturnValue(UT_KEY(SBN_Outbound_SendToPeer), SBN_ERROR);
    UtAssert_INT32_EQ(SBN_Connection_SendProtocolVersionMsg(&BufD, PeerRec), SBN_ERROR);

    /* Also cover the case where the buffer is too small */
    BufD.MaxDataSz = 0;

    UT_ResetState(UT_KEY(SBN_Outbound_SendToPeer));
    UtAssert_INT32_EQ(SBN_Connection_SendProtocolVersionMsg(&BufD, PeerRec), SBN_ERROR);
}

void Test_SBN_Connection_SendProtocolVersionMsg_Nominal(void)
{
    START();

    /* Test case for:
     * SBN_Status_t SBN_Connection_SendProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
     */
    uint8            Buf[16];
    SBN_BufferDesc_t BufD;
    memset(&BufD, 0, sizeof(BufD));

    BufD.DataPtr   = Buf;
    BufD.MaxDataSz = sizeof(Buf);

    UtAssert_INT32_EQ(SBN_Connection_SendProtocolVersionMsg(&BufD, PeerRec), SBN_SUCCESS);
}

void Test_SBN_Connection_RecvProtocolVersionMsg_VerErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version mismatch with peer ");

    uint8 ver = SBN_PROTO_VER + 1;

    SBN_BufferDesc_t BufD;
    memset(&BufD, 0, sizeof(BufD));

    BufD.MsgType      = SBN_PROTO_MSG;
    BufD.ActualDataSz = sizeof(ver);
    BufD.MaxDataSz    = BufD.ActualDataSz;
    BufD.DataPtr      = &ver;

    UtAssert_INT32_EQ(SBN_Connection_RecvProtocolVersionMsg(&BufD, PeerRec), SBN_ERROR);
    EVENT_CNT(1);

    /* same but size check fails */
    BufD.ActualDataSz = 0;
    UtAssert_INT32_EQ(SBN_Connection_RecvProtocolVersionMsg(&BufD, PeerRec), SBN_ERROR);

} /* end ProcessPeerMsg_ProtoMsg_VerErr() */

void Test_SBN_Connection_RecvProtocolVersionMsg_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version match with peer ");
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    uint8 ver = SBN_PROTO_VER;

    BufD.MsgType      = SBN_PROTO_MSG;
    BufD.ActualDataSz = sizeof(ver);
    BufD.MaxDataSz    = BufD.ActualDataSz;
    BufD.DataPtr      = &ver;

    UtAssert_INT32_EQ(SBN_Connection_RecvProtocolVersionMsg(&BufD, PeerRec), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end ProcessPeerMsg_ProtoMsg_Nominal() */

void Test_SBN_Connection_HandleConnect_AlreadyErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "ERROR: could not connect peer: ");

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: 1 - Pre-set peer as already connected to trigger the error condition */
    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_Connection_HandleConnect(PeerPtr), SBN_ERROR);
    UtAssert_INT32_EQ(PeerPtr->Connected, 1);
    EVENT_CNT(1);
} /* end Connected_AlreadyErr() */

void Test_SBN_Connection_HandleConnect_Nominal(void)
{
    START();

    UT_ResetState(0);
    UtAssert_INT32_EQ(SBN_Connection_HandleConnect(PeerPtr), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->Connected, 1);
} /* end Connected_Nominal() */

void Test_SBN_Connection_HandleDisconnect_ConnErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "ERROR: could not disconnect peer: already not connected to peer ");

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: 0 (default) - Peer starts disconnected to trigger the error condition
     * - PeerPtr->ProcessorID: ProcessorID - Set for error message identification */
    SBN_PeerInterface_t *PeerPtr = SBN_Peer_GetIntfData(&SBN_AppData.PeerRecords[0]);

    PeerPtr->ProcessorID = ProcessorID;

    UtAssert_INT32_EQ(SBN_Connection_HandleDisconnect(PeerPtr), SBN_ERROR);
    EVENT_CNT(1);
} /* end Disconnected_Nominal() */

void Test_SBN_Connection_HandleDisconnect_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "Disconnected from peer ");

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->ProcessorID: ProcessorID - Set for event message identification
     * - PeerPtr->Connected: 1 - Pre-set peer as connected to allow valid disconnection */
    SBN_PeerInterface_t *PeerPtr = SBN_Peer_GetIntfData(&SBN_AppData.PeerRecords[0]);

    PeerPtr->ProcessorID = ProcessorID;
    PeerPtr->Connected   = 1;

    UtAssert_INT32_EQ(SBN_Connection_HandleDisconnect(PeerPtr), SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->Connected, 0);
    EVENT_CNT(1);
} /* end Disconnected_Nominal() */

void Test_SBN_Connection_CheckPeer(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_Connection_CheckPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];
    SBN_BufferDesc_t  BufD;
    uint8             Buf[16];

    memset(&BufD, 0, sizeof(BufD));
    BufD.MaxDataSz = sizeof(Buf);
    BufD.DataPtr   = Buf;

    PeerRec->Priv.ProtoMsgSent = false;
    PeerRec->Intf.Connected    = false;

    UtAssert_INT32_EQ(SBN_Connection_CheckPeer(PeerRec, &BufD), SBN_SUCCESS);
    UtAssert_BOOL_FALSE(PeerRec->Priv.ProtoMsgSent);

    IfOpsPtr->PollPeer = NULL;
    UtAssert_INT32_EQ(SBN_Connection_CheckPeer(PeerRec, &BufD), SBN_SUCCESS);
    UtAssert_BOOL_FALSE(PeerRec->Priv.ProtoMsgSent);

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "failed to poll peer");

    IfOpsPtr->PollPeer = SBN_UT_PollPeer_Err;
    UtAssert_INT32_EQ(SBN_Connection_CheckPeer(PeerRec, &BufD), SBN_ERROR);
    UtAssert_BOOL_FALSE(PeerRec->Priv.ProtoMsgSent);
    EVENT_CNT(1);

    IfOpsPtr->PollPeer      = PollPeer_Nominal;
    PeerRec->Intf.Connected = true;
    UT_SetDeferredRetcode(UT_KEY(SBN_Outbound_SendToPeer), 1, SBN_ERROR);
    UtAssert_INT32_EQ(SBN_Connection_CheckPeer(PeerRec, &BufD), SBN_ERROR);
    UtAssert_BOOL_TRUE(PeerRec->Intf.Connected);
    UtAssert_BOOL_FALSE(PeerRec->Priv.ProtoMsgSent);

    UT_ResetState(UT_KEY(SBN_Outbound_SendToPeer));
    UtAssert_INT32_EQ(SBN_Connection_CheckPeer(PeerRec, &BufD), SBN_SUCCESS);
    UtAssert_BOOL_TRUE(PeerRec->Intf.Connected);
    UtAssert_BOOL_TRUE(PeerRec->Priv.ProtoMsgSent);
    UtAssert_STUB_COUNT(SBN_Outbound_SendToPeer, 1);

    /* This is to cover the case where the msg was already sent */
    UtAssert_INT32_EQ(SBN_Connection_CheckPeer(PeerRec, &BufD), SBN_SUCCESS);
    UtAssert_BOOL_TRUE(PeerRec->Intf.Connected);
    UtAssert_BOOL_TRUE(PeerRec->Priv.ProtoMsgSent);
    UtAssert_STUB_COUNT(SBN_Outbound_SendToPeer, 1);
}

void Test_SBN_Connection_PollCycle(void)
{
    START();

    /* Test Case For:
     * void SBN_Connection_PollCycle(void);
     */

    PeerRec->Intf.Connected    = true;
    PeerRec->Priv.ProtoMsgSent = true;

    UtAssert_VOIDCALL(SBN_Connection_PollCycle());
    UtAssert_BOOL_TRUE(PeerRec->Intf.Connected);
    UtAssert_BOOL_TRUE(PeerRec->Priv.ProtoMsgSent);

    PeerRec->Intf.Connected = false;
    UtAssert_VOIDCALL(SBN_Connection_PollCycle());
    UtAssert_BOOL_FALSE(PeerRec->Priv.ProtoMsgSent);
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Connection_HandleConnect_AlreadyErr);
    ADD_TEST(SBN_Connection_HandleConnect_Nominal);
    ADD_TEST(SBN_Connection_HandleDisconnect_ConnErr);
    ADD_TEST(SBN_Connection_HandleDisconnect_Nominal);
    ADD_TEST(SBN_Connection_SendProtocolVersionMsg_SendErr);
    ADD_TEST(SBN_Connection_SendProtocolVersionMsg_Nominal);
    ADD_TEST(SBN_Connection_RecvProtocolVersionMsg_VerErr);
    ADD_TEST(SBN_Connection_RecvProtocolVersionMsg_Nominal);
    ADD_TEST(SBN_Connection_CheckPeer);
    ADD_TEST(SBN_Connection_PollCycle);
}
