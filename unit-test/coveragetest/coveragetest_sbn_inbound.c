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
#include "sbn_inbound.h"
#include "sbn_error.h"
#include "sbn_filter.h"
#include "sbn_worker.h"
#include "sbn_connection.h"

static SBN_Status_t RecvFromPeer_Nominal(SBN_NetInterface_t  *Net,
                                         SBN_PeerInterface_t *Peer,
                                         SBN_MsgType_t       *MsgTypePtr,
                                         SBN_MsgSz_t         *MsgSzPtr,
                                         SBN_ProcessorID_t   *ProcessorIDPtr,
                                         SBN_SpacecraftID_t  *SpacecraftIDPtr,
                                         void                *PayloadBuffer)
{
    *ProcessorIDPtr = 1235;

    return SBN_SUCCESS;
} /* end RecvFromNet_Nominal() */

static SBN_Status_t RecvFromPeer_Err(SBN_NetInterface_t  *Net,
                                     SBN_PeerInterface_t *Peer,
                                     SBN_MsgType_t       *MsgTypePtr,
                                     SBN_MsgSz_t         *MsgSzPtr,
                                     SBN_ProcessorID_t   *ProcessorIDPtr,
                                     SBN_SpacecraftID_t  *SpacecraftIDPtr,
                                     void                *PayloadBuffer)
{
    return SBN_ERROR;
} /* end RecvFromNet_Nominal() */

static SBN_Status_t RecvFromPeer_Empty(SBN_NetInterface_t  *Net,
                                       SBN_PeerInterface_t *Peer,
                                       SBN_MsgType_t       *MsgTypePtr,
                                       SBN_MsgSz_t         *MsgSzPtr,
                                       SBN_ProcessorID_t   *ProcessorIDPtr,
                                       SBN_SpacecraftID_t  *SpacecraftIDPtr,
                                       void                *PayloadBuffer)
{
    return SBN_IF_EMPTY;
} /* end RecvFromNet_Nominal() */

static SBN_Status_t RecvFromNet_Empty(SBN_NetInterface_t *Net,
                                      SBN_MsgType_t      *MsgTypePtr,
                                      SBN_MsgSz_t        *MsgSzPtr,
                                      SBN_ProcessorID_t  *ProcessorIDPtr,
                                      SBN_SpacecraftID_t *SpacecraftIDPtr,
                                      void               *PayloadBuffer)
{
    return SBN_IF_EMPTY;
} /* end RecvFromNet_Nominal() */

static SBN_Status_t RecvFromNet_Err(SBN_NetInterface_t *Net,
                                    SBN_MsgType_t      *MsgTypePtr,
                                    SBN_MsgSz_t        *MsgSzPtr,
                                    SBN_ProcessorID_t  *ProcessorIDPtr,
                                    SBN_SpacecraftID_t *SpacecraftIDPtr,
                                    void               *PayloadBuffer)
{
    return SBN_ERROR;
} /* end RecvFromNet_Nominal() */

static void Test_SBN_Inbound_ProcessMsg_AppMsg_FiltErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 1 - Single filter to test error condition
     * - Filters[0]: Filter with RecvFilter_Err (returns SBN_ERROR) - Triggers filter error during message processing
     * - ProcessorID/SpacecraftID: Set to match the message being processed to ensure peer lookup succeeds */
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    PeerPtr->FilterCnt = 1;

    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetProcessorId), ProcessorID);
    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetSpacecraftId), SpacecraftID);
    UT_SetDeferredRetcode(UT_KEY(SBN_Filter_Mutation), 1, -1);

    BufD.MsgType = SBN_APP_MSG;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_ERROR);
} /* end ProcessPeerMsg_AppMsg_FiltErr() */

static void Test_SBN_Inbound_ProcessMsg_AppMsg_FiltOut(void)
{
    START();

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 1 - Single filter to test message filtering
     * - Filters[0]: Filter with RecvFilter_Out (returns SBN_IF_EMPTY) - Filters out the message
     * - PeerPtr->ProcessorID/SpacecraftID: Set to match message parameters to ensure peer lookup succeeds
     * - ProcessorID/SpacecraftID: Set to match the message being processed */
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    PeerPtr->ProcessorID  = ProcessorID;
    PeerPtr->SpacecraftID = SpacecraftID;
    PeerPtr->FilterCnt    = 1;

    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetProcessorId), ProcessorID);
    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetSpacecraftId), SpacecraftID);
    UT_SetDeferredRetcode(UT_KEY(SBN_Filter_Mutation), 1, SBN_IF_EMPTY);

    BufD.MsgType = SBN_APP_MSG;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_IF_EMPTY);
} /* end ProcessPeerMsg_AppMsg_FiltOut() */

static SBN_Status_t RecvFilter_Nominal(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_SUCCESS;
} /* end RecvFilter_Nominal() */

static void Test_SBN_Inbound_ProcessMsg_AppMsg_PassMsgErr(void)
{
    START();
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_SB_EID, "CFE_SB_TransmitMsg error");

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_TransmitMsg), 1, -1);

    BufD.MsgType = SBN_APP_MSG;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_ERROR);

    EVENT_CNT(1);
} /* end ProcessPeerMsg_AppMsg_PassMsgErr() */

static void Test_SBN_Inbound_ProcessMsg_AppMsg_Nominal(void)
{
    START();

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 2 - Two filters to test filter chain processing
     * - Filters[0]: Filter_Empty (NULL FilterRecv function) - Should be skipped during processing
     * - Filters[1]: Filter_Nominal (returns SBN_SUCCESS) - Allows message to pass through
     * - ProcessorID/SpacecraftID: Set to match the message being processed to ensure peer lookup succeeds */
    SBN_FilterInterface_t Filter_Empty, Filter_Nominal;
    SBN_BufferDesc_t      BufD;

    memset(&BufD, 0, sizeof(BufD));

    memset(&Filter_Empty, 0, sizeof(Filter_Empty));
    memset(&Filter_Nominal, 0, sizeof(Filter_Nominal));
    Filter_Nominal.FilterRecv = RecvFilter_Nominal;

    /* Filters[0].Recv is NULL, should skip */
    PeerPtr->Filters[0] = &Filter_Empty;
    PeerPtr->Filters[1] = &Filter_Nominal;
    PeerPtr->FilterCnt  = 2;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    BufD.MsgType = SBN_APP_MSG;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_SUCCESS);
} /* end ProcessPeerMsg_AppMsg_Nominal() */

static void Test_SBN_Inbound_ProcessMsg_SubMsg_Nominal(void)
{
    START();

    uint8            Buf[SBN_PACKED_SUB_SZ];
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    BufD.MsgType      = SBN_SUB_MSG;
    BufD.ActualDataSz = sizeof(Buf);
    BufD.MaxDataSz    = BufD.ActualDataSz;
    BufD.DataPtr      = Buf;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_Subs_ProcessMsgFromPeer, 1);
} /* end ProcessPeerMsg_SubMsg_Nominal() */

static void Test_SBN_Inbound_ProcessMsg_UnSubMsg_Nominal(void)
{
    START();

    uint8            Buf[SBN_PACKED_SUB_SZ];
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    BufD.MsgType      = SBN_UNSUB_MSG;
    BufD.ActualDataSz = sizeof(Buf);
    BufD.MaxDataSz    = BufD.ActualDataSz;
    BufD.DataPtr      = Buf;
    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_Subs_ProcessMsgFromPeer, 1);
} /* end ProcessPeerMsg_UnSubMsg_Nominal() */

static void Test_SBN_Inbound_ProcessMsg_NoMsg_Nominal(void)
{
    START();

    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    BufD.MsgType = SBN_NO_MSG;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_SUCCESS);
} /* end ProcessPeerMsg_NoMsg_Nominal() */

static void Test_SBN_Inbound_ProcessMsg_MsgErr(void)
{
    START();

    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    /* send a net message of an invalid type */
    BufD.MsgType = SBN_NO_MSG + 100;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_ERROR);
} /* end ProcessPeerMsg_MsgErr() */

void Test_SBN_Inbound_ProcessMsg_Other(void)
{
    START();

    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    BufD.MsgType = SBN_MODULE_SPECIFIC_MESSAGE_ID_MASK;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_SUCCESS);

    BufD.MsgType = SBN_PROTO_MSG;

    UtAssert_INT32_EQ(SBN_Inbound_ProcessMsg(&BufD, PeerRec), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_Connection_RecvProtocolVersionMsg, 1);
}

void Test_SBN_Inbound_IsPeerScheduled(void)
{
    START();

    /* Test Case For:
     * bool SBN_Inbound_IsPeerScheduled(SBN_PeerRecord_t *PeerRec)
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];
    UtAssert_BOOL_FALSE(SBN_Inbound_IsPeerScheduled(PeerRec));
}

void Test_SBN_Inbound_IsNetScheduled(void)
{
    START();

    /* Test Case For:
     * bool SBN_Inbound_IsNetScheduled(SBN_NetRecord_t *NetRec)
     */
    SBN_NetRecord_t *NetRec = &SBN_AppData.NetRecords[0];
    UtAssert_BOOL_FALSE(SBN_Inbound_IsNetScheduled(NetRec));
}

void Test_SBN_Inbound_ServicePeer(void)
{
    START();

    /* Test Case For:
     * void SBN_Inbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];
    SBN_BufferDesc_t  BufD;

    memset(&BufD, 0, sizeof(BufD));
    IfOpsPtr->RecvFromPeer = RecvFromPeer_Nominal;
    IfOpsPtr->RecvFromNet  = NULL;

    UtAssert_VOIDCALL(SBN_Inbound_ServicePeer(PeerRec, &BufD, 2));

    PeerRec->Intf.Connected              = true;
    PeerRec->Sched.InboundAutoReschedule = true;
    UtAssert_VOIDCALL(SBN_Inbound_ServicePeer(PeerRec, &BufD, 2));

    IfOpsPtr->RecvFromPeer = RecvFromPeer_Err;
    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "SBN_Inbound_ServicePeer failed for peer");
    UtAssert_VOIDCALL(SBN_Inbound_ServicePeer(PeerRec, &BufD, 2));
    EVENT_CNT(1);

    IfOpsPtr->RecvFromPeer = RecvFromPeer_Empty;
    UtAssert_VOIDCALL(SBN_Inbound_ServicePeer(PeerRec, &BufD, 2));

    IfOpsPtr->RecvFromPeer = NULL;
    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
}

void Test_SBN_Inbound_ServiceNet(void)
{
    START();

    /* Test Case For:
     * void SBN_Inbound_ServiceNet(SBN_NetRecord_t *NetRec, SBN_BufferDesc_t *BufD)
     */
    SBN_NetRecord_t *NetRec = &SBN_AppData.NetRecords[0];
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
    UtAssert_VOIDCALL(SBN_Inbound_ServiceNet(NetRec, &BufD, 2));

    NetRec->Sched.InboundAutoReschedule = true;
    UtAssert_VOIDCALL(SBN_Inbound_ServiceNet(NetRec, &BufD, 2));

    IfOpsPtr->RecvFromNet = RecvFromNet_Err;
    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "SBN_Inbound_ServiceNet failed for net");
    UtAssert_VOIDCALL(SBN_Inbound_ServiceNet(NetRec, &BufD, 2));
    EVENT_CNT(1);

    IfOpsPtr->RecvFromNet = RecvFromNet_Empty;
    UtAssert_VOIDCALL(SBN_Inbound_ServiceNet(NetRec, &BufD, 2));

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
    UtAssert_VOIDCALL(SBN_Inbound_ServiceNet(NetRec, &BufD, 1));
}

void Test_SBN_Inbound_PollCycle(void)
{
    START();

    /* Test Case For:
     * void SBN_Inbound_PollCycle(void)
     */
    SBN_NetRecord_t  *NetRec  = &SBN_AppData.NetRecords[0];
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];

    /* This first call will basically do nothing */
    UtAssert_VOIDCALL(SBN_Inbound_PollCycle());

    /* Enable scheduling - this will now schedule it */
    NetRec->Sched.InboundSchedulingEnabled  = true;
    PeerRec->Sched.InboundSchedulingEnabled = true;
    UtAssert_VOIDCALL(SBN_Inbound_PollCycle());

    UT_SetDefaultReturnValue(UT_KEY(SBN_Worker_IsJobPending), true);
    UtAssert_VOIDCALL(SBN_Inbound_PollCycle());
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Inbound_ProcessMsg_AppMsg_FiltErr);
    ADD_TEST(SBN_Inbound_ProcessMsg_AppMsg_FiltOut);
    ADD_TEST(SBN_Inbound_ProcessMsg_AppMsg_PassMsgErr);
    ADD_TEST(SBN_Inbound_ProcessMsg_MsgErr);
    ADD_TEST(SBN_Inbound_ProcessMsg_AppMsg_Nominal);
    ADD_TEST(SBN_Inbound_ProcessMsg_SubMsg_Nominal);
    ADD_TEST(SBN_Inbound_ProcessMsg_UnSubMsg_Nominal);
    ADD_TEST(SBN_Inbound_ProcessMsg_NoMsg_Nominal);
    ADD_TEST(SBN_Inbound_ProcessMsg_Other);
    ADD_TEST(SBN_Inbound_IsPeerScheduled);
    ADD_TEST(SBN_Inbound_IsNetScheduled);
    ADD_TEST(SBN_Inbound_ServicePeer);
    ADD_TEST(SBN_Inbound_ServiceNet);
    ADD_TEST(SBN_Inbound_PollCycle);
}
