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
#include "sbn_outbound.h"
#include "sbn_filter.h"
#include "sbn_worker.h"
#include "sbn_error.h"

static SBN_Status_t SBN_UT_SendErr(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload)
{
    return SBN_ERROR;
}

void Test_SBN_Outbound_SendToPeer(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_Outbound_SendToPeer(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer);
     */
    SBN_PeerInterface_t *Peer = &SBN_AppData.PeerRecords[0].Intf;
    SBN_BufferDesc_t     BufD;

    memset(&BufD, 0, sizeof(BufD));

    UtAssert_INT32_EQ(SBN_Outbound_SendToPeer(&BufD, Peer), SBN_SUCCESS);
    UtAssert_UINT16_EQ(Peer->Counts.SendCnt, 1);

    IfOpsPtr->Send = SBN_UT_SendErr;
    UtAssert_INT32_EQ(SBN_Outbound_SendToPeer(&BufD, Peer), SBN_ERROR);
    UtAssert_UINT16_EQ(Peer->Counts.SendErrCnt, 1);
    IfOpsPtr->Send = Send_Nominal;
}

void Test_SBN_Outbound_GetMsg(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_Outbound_GetMsg(SBN_BufferDesc_t *BufD, CFE_SB_PipeId_t PeerPipe, int32 Timeout);
     */
    SBN_BufferDesc_t  BufD;
    CFE_SB_PipeId_t   PeerPipe = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(1));
    CFE_SB_Buffer_t   SbBuf;
    CFE_MSG_Message_t Msg;
    CFE_SB_Buffer_t  *BufPtr;
    CFE_MSG_Size_t    MsgSz;

    memset(&BufD, 0, sizeof(BufD));
    memset(&SbBuf, 0, sizeof(SbBuf));
    memset(&Msg, 0xA5, sizeof(Msg));
    BufPtr = &SbBuf;
    MsgSz  = sizeof(SbBuf.Msg);

    BufD.MaxDataSz = sizeof(Msg);
    BufD.DataPtr   = &Msg;

    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false);

    UtAssert_INT32_EQ(SBN_Outbound_GetMsg(&BufD, PeerPipe, 100), SBN_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_TIME_OUT);
    UtAssert_INT32_EQ(SBN_Outbound_GetMsg(&BufD, PeerPipe, 100), SBN_IF_EMPTY);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_NO_MESSAGE);
    UtAssert_INT32_EQ(SBN_Outbound_GetMsg(&BufD, PeerPipe, 100), SBN_IF_EMPTY);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_INT32_EQ(SBN_Outbound_GetMsg(&BufD, PeerPipe, 100), SBN_ERROR);

    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetSize), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_INT32_EQ(SBN_Outbound_GetMsg(&BufD, PeerPipe, 100), SBN_ERROR);

    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false);
    BufD.MaxDataSz = sizeof(Msg) - 1;
    UtAssert_INT32_EQ(SBN_Outbound_GetMsg(&BufD, PeerPipe, 100), SBN_ERROR);
}

void Test_SBN_Outbound_IsScheduled(void)
{
    START();

    /* Test Case For:
     * bool SBN_Outbound_IsScheduled(SBN_PeerRecord_t *PeerRec);
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];

    UtAssert_BOOL_FALSE(SBN_Outbound_IsScheduled(PeerRec));
}

void Test_SBN_Outbound_ServicePeer(void)
{
    START();

    /* Test Case For:
     * void SBN_Outbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];
    SBN_BufferDesc_t  BufD;
    CFE_SB_Buffer_t   SbBuf;
    CFE_MSG_Message_t Msg;
    CFE_SB_Buffer_t  *BufPtr;
    CFE_MSG_Size_t    MsgSz;

    memset(&BufD, 0, sizeof(BufD));
    memset(&SbBuf, 0, sizeof(SbBuf));
    memset(&Msg, 0xA5, sizeof(Msg));
    BufPtr = &SbBuf;
    MsgSz  = sizeof(SbBuf.Msg);

    BufD.MaxDataSz = sizeof(Msg);
    BufD.DataPtr   = &Msg;

    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 2));

    PeerRec->Intf.Connected               = true;
    PeerRec->Sched.OutboundAutoReschedule = true;
    PeerRec->Intf.Pipe                    = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(1));
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_NO_MESSAGE);
    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 2));
    UtAssert_UINT16_EQ(PeerRec->Intf.Counts.SendCnt, 0);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_TIME_OUT);
    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 2));
    UtAssert_UINT16_EQ(PeerRec->Intf.Counts.SendCnt, 0);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 2));
    UtAssert_UINT16_EQ(PeerRec->Intf.Counts.SendCnt, 0);

    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetSize), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 2));
    UtAssert_UINT16_EQ(PeerRec->Intf.Counts.SendCnt, 0);

    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false);
    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 1));
    UtAssert_UINT16_EQ(PeerRec->Intf.Counts.SendCnt, 1);

    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false);
    UT_SetDeferredRetcode(UT_KEY(SBN_Filter_Mutation), 1, SBN_IF_EMPTY);
    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 1));
    UtAssert_UINT16_EQ(PeerRec->Intf.Counts.SendCnt, 1);

    /* Buffer too small error */
    BufD.MaxDataSz = 0;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false);
    UtAssert_VOIDCALL(SBN_Outbound_ServicePeer(PeerRec, &BufD, 1));
    UtAssert_UINT16_EQ(PeerRec->Intf.Counts.SendCnt, 1);
}

void Test_SBN_Outbound_PollCycle(void)
{
    START();

    /* Test Case For:
     * void SBN_Outbound_PollCycle(void);
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];

    UtAssert_VOIDCALL(SBN_Outbound_PollCycle());
    UtAssert_STUB_COUNT(SBN_Worker_ScheduleJob, 1);

    PeerRec->Intf.Connected = true;
    UtAssert_VOIDCALL(SBN_Outbound_PollCycle());
    UtAssert_STUB_COUNT(SBN_Worker_ScheduleJob, 1);

    PeerRec->Intf.Pipe = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(1));
    UtAssert_VOIDCALL(SBN_Outbound_PollCycle());
    UtAssert_STUB_COUNT(SBN_Worker_ScheduleJob, 2);

    UT_SetDeferredRetcode(UT_KEY(SBN_Worker_IsJobPending), 1, true);
    UtAssert_VOIDCALL(SBN_Outbound_PollCycle());
    UtAssert_STUB_COUNT(SBN_Worker_ScheduleJob, 2);
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Outbound_SendToPeer);
    ADD_TEST(SBN_Outbound_GetMsg);
    ADD_TEST(SBN_Outbound_IsScheduled);
    ADD_TEST(SBN_Outbound_ServicePeer);
    ADD_TEST(SBN_Outbound_PollCycle);
}
