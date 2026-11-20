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
#include "cfe_msgids.h"
#include "sbn_pack.h"
#include "sbn_error.h"
#include "sbn_outbound.h"
#include "sbn_filter.h"

CFE_SB_MsgId_t MsgID = { .Value = 0xDEAD };

void Test_SBN_SendSubsRequests_SendMsg1Err(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "Unable to turn on sub reporting (status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_TransmitMsg), 1, -1);

    UtAssert_INT32_EQ(SBN_SendSubsRequests(), SBN_ERROR);
    EVENT_CNT(1);
} /* end SendSubsRequests_SendMsg1Err() */

void Test_SBN_SendSubsRequests_SendMsg2Err(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "Unable to send prev subs request (status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_TransmitMsg), 2, -1);

    UtAssert_INT32_EQ(SBN_SendSubsRequests(), SBN_ERROR);
    EVENT_CNT(1);
} /* end SendSubsRequests_SendMsg2Err() */

void Test_SBN_SendSubsRequests_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_SendSubsRequests(), SBN_SUCCESS);
}

void Test_SBN_SLS2P_SendNetMsgErr(void)
{
    START();

    uint8            Buf[SBN_PACKED_SUB_SZ];
    SBN_BufferDesc_t BufD;

    SBN_AppData.SubSet.SubCnt        = 1;
    SBN_AppData.SubSet.Subs[0].MsgID = MsgID;

    PeerPtr->Net = NetPtr;

    IfOpsPtr->Send = Send_Err;

    memset(&BufD, 0, sizeof(BufD));
    memset(Buf, 0, sizeof(Buf));

    BufD.MaxDataSz = sizeof(Buf);
    BufD.DataPtr   = Buf;

    UT_SetDefaultReturnValue(UT_KEY(SBN_Outbound_SendToPeer), SBN_ERROR);
    UtAssert_INT32_EQ(SBN_Subs_SendLocalToPeer(PeerRec, &BufD), SBN_ERROR);

    IfOpsPtr->Send = Send_Nominal;
} /* end SLS2P_SendNetMsgErr() */

void Test_SBN_CSP_PLS_MaxSubsErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "local subscription ignored for MsgID 0x");

    SBN_AppData.SubSet.SubCnt = SBN_MISSION_MAX_SUBS_PER_PEER;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId   = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_ERROR);

    EVENT_CNT(1);
} /* end CSP_PLS_MaxSubsErr() */

void Test_SBN_CSP_PLS_EvtMsg(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);

    IfOpsPtr->Send = Send_Err;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId   = CFE_SB_ValueToMsgId(CFE_EVS_LONG_EVENT_MSG_MID);
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_SUCCESS);

    IfOpsPtr->Send = Send_Nominal;
} /* end CSP_PLS_EvtMsg() */

void Test_SBN_CSP_PLS_SbnMsg(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);

    IfOpsPtr->Send = Send_Err;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId   = CFE_SB_ValueToMsgId(SBN_CMD_MID);
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_SUCCESS);

    IfOpsPtr->Send = Send_Nominal;
} /* end CSP_PLS_EvtMsg() */

void Test_SBN_CSP_PLU_NotSub(void)
{
    START();

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION;
    Msg.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_SUCCESS);
} /* end CSP_PLU_NotSub() */

void Test_SBN_CSP_PLU_Nominal(void)
{
    START();

    SBN_AppData.SubSet.SubCnt        = 1;
    SBN_AppData.SubSet.Subs[0].MsgID = MsgID;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION;
    Msg.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_SUCCESS);

    UtAssert_INT32_EQ(SBN_AppData.SubSet.SubCnt, 0);
} /* end CSP_PLU_Nominal() */

void Test_SBN_CSP_SubTypeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "unexpected subscription type (");

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION + 10; /* invalid subtype */
    Msg.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_ERROR);

    UtAssert_INT32_EQ(SBN_AppData.SubSet.SubCnt, 0);

    EVENT_CNT(1);
} /* end CSP_SubTypeErr() */

void Test_SBN_CSP_AllSubs_EntryCntErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "entries value ");

    CFE_SB_AllSubscriptionsTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.Entries = CFE_MISSION_SB_SUB_ENTRIES_PER_PKT + 1;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_ERROR);

    UtAssert_INT32_EQ(SBN_AppData.SubSet.SubCnt, 0);

    EVENT_CNT(1);
} /* end CSP_AllSubs_EntryCntErr() */

void Test_SBN_CSP_AllSubs_PLSErr(void)
{
    START();

    /* err generated by ProcessLocalSub() */
    UT_CheckEvent_Setup(SBN_SUB_EID, "local subscription ignored for MsgID 0x");

    SBN_AppData.SubSet.SubCnt = SBN_MISSION_MAX_SUBS_PER_PEER;

    CFE_SB_AllSubscriptionsTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.Entries        = 1;
    Msg.Payload.Entry[0].MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_ERROR);

    EVENT_CNT(1);
} /* end CSP_AllSubs_PLSErr() */

void Test_SBN_CSP_AllSubs_Nominal(void)
{
    START();

    CFE_SB_AllSubscriptionsTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));

    /* This needs to contain one of each of the ignored MsgIDs, plus one other */
    Msg.Payload.Entries        = 10;
    Msg.Payload.Entry[0].MsgId = CFE_SB_ValueToMsgId(SBN_CMD_MID);
    Msg.Payload.Entry[1].MsgId = CFE_SB_ValueToMsgId(SBN_HK_TLM_MID);
    Msg.Payload.Entry[2].MsgId = CFE_SB_ValueToMsgId(SBN_HKNET_TLM_MID);
    Msg.Payload.Entry[3].MsgId = CFE_SB_ValueToMsgId(SBN_HKPEER_TLM_MID);
    Msg.Payload.Entry[4].MsgId = CFE_SB_ValueToMsgId(SBN_HKMYSUBS_TLM_MID);
    Msg.Payload.Entry[5].MsgId = CFE_SB_ValueToMsgId(SBN_HKPEERSUBS_TLM_MID);
    Msg.Payload.Entry[6].MsgId = CFE_SB_ValueToMsgId(CFE_EVS_LONG_EVENT_MSG_MID);
    Msg.Payload.Entry[7].MsgId = CFE_SB_ValueToMsgId(1);
    Msg.Payload.Entry[8].MsgId = CFE_SB_ValueToMsgId(2);
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    SBN_AppData.SubSet.SubCnt        = 1;
    SBN_AppData.SubSet.Subs[0].MsgID = CFE_SB_ValueToMsgId(2);

    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_SUCCESS);

    UtAssert_INT32_EQ(SBN_AppData.SubSet.SubCnt, 2);
} /* end CSP_AllSubs_Nominal() */

void Test_SBN_CSP_NoMsg(void)
{
    START();

    /* There are multiple flavors depending on whether its a timeout or a real error
     * that caused no messages to be read */
    CFE_SB_Buffer_t  SbBuf;
    CFE_SB_Buffer_t *BufPtr;
    CFE_SB_MsgId_t   MsgId;

    MsgId = CFE_SB_INVALID_MSG_ID;
    memset(&SbBuf, 0, sizeof(SbBuf));
    BufPtr = &SbBuf;

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_NO_MESSAGE);
    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_IF_EMPTY);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_TIME_OUT);
    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_IF_EMPTY);

    UT_CheckEvent_Setup(SBN_MSG_EID, "err from rcvmsg on sub pipe");
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_ERROR);
    EVENT_CNT(1);

    UT_CheckEvent_Setup(SBN_MSG_EID, "unable to get message id");
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetMsgId), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_ERROR);
    EVENT_CNT(1);

    UT_CheckEvent_Setup(SBN_MSG_EID, "unexpected message id");
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &MsgId, sizeof(MsgId), false);
    UtAssert_INT32_EQ(SBN_Subs_PollCycle(), SBN_ERROR);
    EVENT_CNT(1);

} /* end CSP_NoMsg() */

void Test_SBN_PSFP_PFP_FiltErr(void)
{
    START();

    PeerPtr->FilterCnt = 2;

    uint8            Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;
    uint32           StartVersion;

    StartVersion = PeerRec->Priv.SubQ.CurrVersion;
    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UT_SetDeferredRetcode(UT_KEY(SBN_Filter_Remap), 1, -1);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_SUCCESS);

    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, StartVersion);
} /* end PSFP_PFP_FiltErr() */

void Test_SBN_PSFP_PFP_MaxSub(void)
{
    START();

    uint8            Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;
    uint32           StartVersion;

    StartVersion = PeerRec->Priv.SubQ.CurrVersion;

    memset(&BufD, 0, sizeof(BufD));
    memset(&Buf, 0, sizeof(Buf));

    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1 + SBN_MISSION_MAX_SUBS_PER_PEER);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_SUCCESS);

    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, StartVersion + SBN_MISSION_MAX_SUBS_PER_PEER);
} /* end PSFP_PFP_AlreadySub() */

void Test_SBN_PSFP_PFP_AlreadySub(void)
{
    START();

    PeerPtr->SubSet.SubCnt        = 1;
    PeerPtr->SubSet.Subs[0].MsgID = MsgID;

    uint8            Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;
    uint32           StartVersion;

    StartVersion = PeerRec->Priv.SubQ.CurrVersion;

    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_SUCCESS);

    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, StartVersion + 1);
} /* end PSFP_PFP_AlreadySub() */

void Test_SBN_PSFP_IdentErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PROTO_EID, "version number mismatch with peer CpuID ");

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    char  tmpident[SBN_IDENT_LEN];
    memset(tmpident, 0, sizeof(tmpident));
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)tmpident, sizeof(tmpident));
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_ERROR);

    UtAssert_INT32_EQ(PeerPtr->SubSet.SubCnt, 0);

    EVENT_CNT(1);
} /* end PSFP_IdentErr() */

void Test_SBN_PSFP_Nominal(void)
{
    START();

    uint8            Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;
    uint32           StartVersion;

    StartVersion = PeerRec->Priv.SubQ.CurrVersion;
    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_SUCCESS);

    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, StartVersion + 1);
} /* end PSFP_Nominal() */

void Test_SBN_PUSFP_PUFP_FiltErr(void)
{
    START();

    PeerPtr->SubSet.SubCnt        = 1;
    PeerPtr->SubSet.Subs[0].MsgID = MsgID;
    PeerPtr->FilterCnt            = 2;

    uint8            Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;
    uint32           StartVersion;

    StartVersion = PeerRec->Priv.SubQ.CurrVersion;
    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UT_SetDeferredRetcode(UT_KEY(SBN_Filter_Remap), 1, -1);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_SUCCESS);

    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, StartVersion);
} /* end PUSFP_PUFP_FiltErr() */

void Test_SBN_PUSFP_PUFP_NotSub(void)
{
    START();

    uint8            Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;
    uint32           StartVersion;

    StartVersion = PeerRec->Priv.SubQ.CurrVersion;
    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_SUCCESS);

    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, StartVersion + 1);
} /* end PUSFP_PUFP_NotSub() */

void Test_SBN_PUSFP_IdentWarn(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PROTO_EID, "version number mismatch with peer CpuID ");

    PeerPtr->SubSet.SubCnt        = 1;
    PeerPtr->SubSet.Subs[0].MsgID = MsgID;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    char  tmpident[SBN_IDENT_LEN];
    memset(tmpident, 0, sizeof(tmpident));
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;

    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)tmpident, sizeof(tmpident));
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_ERROR);

    EVENT_CNT(1);
} /* end PUSFP_IdentWarn() */

void Test_SBN_PUSFP_Nominal(void)
{
    START();

    PeerPtr->SubSet.SubCnt        = 1;
    PeerPtr->SubSet.Subs[0].MsgID = MsgID;

    uint8            Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_PackState_t  Pack;
    SBN_BufferDesc_t BufD;
    uint32           StartVersion;

    StartVersion = PeerRec->Priv.SubQ.CurrVersion;
    memset(&BufD, 0, sizeof(BufD));
    SBN_Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1);
    SBN_Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    SBN_Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    BufD.MaxDataSz    = sizeof(Buf);
    BufD.DataPtr      = Buf;
    BufD.ActualDataSz = SBN_Pack_GetPosition(&Pack);

    UtAssert_INT32_EQ(SBN_Subs_ProcessMsgFromPeer(PeerRec, &BufD), SBN_SUCCESS);

    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, StartVersion + 1);
} /* end PUSFP_Nominal() */

void Test_SBN_UDPLS_NoPipe(void)
{
    START();

    PeerPtr->Pipe = CFE_SB_INVALID_PIPE;

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_UnsubscribeLocal), 1, -1);

    /* errors are generate events but still successful return */
    UtAssert_VOIDCALL(SBN_Subs_UpdateDisconnectedPeerLocalSubs(PeerRec));
} /* end UDPLS_NoPipe() */

void Test_SBN_UDPLS_Nominal(void)
{
    START();

    PeerPtr->Pipe = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(1));
    UtAssert_VOIDCALL(SBN_Subs_UpdateDisconnectedPeerLocalSubs(PeerRec));
    UtAssert_BOOL_FALSE(CFE_RESOURCEID_TEST_DEFINED(PeerPtr->Pipe));

    /* Same but already scheduled */
    PeerPtr->Pipe = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(1));
    UT_SetDeferredRetcode(UT_KEY(SBN_Outbound_IsScheduled), 1, true);
    UtAssert_VOIDCALL(SBN_Subs_UpdateDisconnectedPeerLocalSubs(PeerRec));
    UtAssert_BOOL_TRUE(CFE_RESOURCEID_TEST_DEFINED(PeerPtr->Pipe));
} /* end UDPLS_Nominal() */

void Test_SBN_Subs_GetPrevChangeEntry(void)
{
    START();

    /* Test Case For:
     * SBN_SubChange_t *SBN_Subs_GetPrevChangeEntry(SBN_SubJournal_t *Journal, uint32 Version);
     */
    uint32           Idx;
    SBN_SubJournal_t SubJournal;

    memset(&SubJournal, 0, sizeof(SubJournal));
    SubJournal.CurrVersion = 1;

    UtAssert_ADDRESS_EQ(SBN_Subs_GetPrevChangeEntry(&SubJournal, 0), &SubJournal.Entries[0]);
    UtAssert_NULL(SBN_Subs_GetPrevChangeEntry(&SubJournal, 1));

    SubJournal.CurrVersion  = 10000;
    Idx                     = SubJournal.CurrVersion % SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH;
    SubJournal.CurrVersion += SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH - 4;

    UtAssert_ADDRESS_EQ(SBN_Subs_GetPrevChangeEntry(&SubJournal, 10000), &SubJournal.Entries[Idx]);
    SubJournal.CurrVersion += 5;
    UtAssert_NULL(SBN_Subs_GetPrevChangeEntry(&SubJournal, 10000));
}

void Test_SBN_Subs_SetupLocalPipe(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_Subs_SetupLocalPipe(SBN_PeerRecord_t *PeerRec);
     */
    SBN_PeerRecord_t *PeerRec   = &SBN_AppData.PeerRecords[0];
    CFE_SB_PipeId_t   UtPipeId2 = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(2));
    CFE_SB_PipeId_t   UtPipeId3 = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(2));

    UT_SetDataBuffer(UT_KEY(CFE_SB_CreatePipe), &UtPipeId2, sizeof(UtPipeId2), false);
    UtAssert_INT32_EQ(SBN_Subs_SetupLocalPipe(PeerRec), SBN_SUCCESS);
    UtAssert_STUB_COUNT(CFE_SB_CreatePipe, 1);
    UtAssert_STUB_COUNT(CFE_SB_DeletePipe, 0);
    UtAssert_STUB_COUNT(CFE_SB_SetPipeOpts, 1);
    UtAssert_STUB_COUNT(CFE_SB_SubscribeLocal, 0);

    UtAssert_BOOL_TRUE(CFE_RESOURCEID_TEST_EQUAL(UtPipeId2, PeerRec->Intf.Pipe));

    /* Pipe already exists, with subscriptions */
    UT_SetDataBuffer(UT_KEY(CFE_SB_CreatePipe), &UtPipeId3, sizeof(UtPipeId3), false);
    PeerRec->Intf.SubSet.SubCnt = 2;
    UtAssert_INT32_EQ(SBN_Subs_SetupLocalPipe(PeerRec), SBN_SUCCESS);
    UtAssert_STUB_COUNT(CFE_SB_DeletePipe, 1);
    UtAssert_STUB_COUNT(CFE_SB_CreatePipe, 2);
    UtAssert_STUB_COUNT(CFE_SB_SubscribeLocal, 2);

    UtAssert_BOOL_TRUE(CFE_RESOURCEID_TEST_EQUAL(UtPipeId3, PeerRec->Intf.Pipe));

    /* Pipe fails to create */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1);
    UT_CheckEvent_Setup(SBN_PEER_EID, "CFE_SB_CreatePipe() failed");
    UtAssert_INT32_EQ(SBN_Subs_SetupLocalPipe(PeerRec), SBN_ERROR);
    UtAssert_BOOL_FALSE(CFE_RESOURCEID_TEST_DEFINED(PeerRec->Intf.Pipe));
    UtAssert_STUB_COUNT(CFE_SB_DeletePipe, 2);
    EVENT_CNT(1);

    /* Pipe fails to set options */
    UT_SetDataBuffer(UT_KEY(CFE_SB_CreatePipe), &UtPipeId2, sizeof(UtPipeId2), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SetPipeOpts), 1, -1);
    UT_CheckEvent_Setup(SBN_PEER_EID, "CFE_SB_SetPipeOpts() failed");
    UtAssert_INT32_EQ(SBN_Subs_SetupLocalPipe(PeerRec), SBN_ERROR);
    UtAssert_BOOL_FALSE(CFE_RESOURCEID_TEST_DEFINED(PeerRec->Intf.Pipe));
    UtAssert_STUB_COUNT(CFE_SB_DeletePipe, 3);
    EVENT_CNT(1);
}

void Test_SBN_Subs_UpdateConnectedPeerRemoteSubs(void)
{
    START();

    /* Test Case For:
     * void SBN_Subs_UpdateConnectedPeerRemoteSubs(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];
    SBN_BufferDesc_t  BufD;

    memset(&BufD, 0, sizeof(BufD));

    /* Nominal, does nothing */
    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerRemoteSubs(PeerRec, &BufD));
    UtAssert_UINT32_EQ(PeerRec->Priv.PeerSentSubVersion, SBN_AppData.SubQ.CurrVersion);

    /* Add sub to sync */
    SBN_AppData.SubQ.Entries[SBN_AppData.SubQ.CurrVersion] =
        (SBN_SubChange_t) { .IsSubscribe = true, .MsgId = CFE_SB_MSGID_WRAP_VALUE(0x5678) };
    ++SBN_AppData.SubQ.CurrVersion;
    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerRemoteSubs(PeerRec, &BufD));
    UtAssert_UINT32_EQ(PeerRec->Priv.PeerSentSubVersion, SBN_AppData.SubQ.CurrVersion);

    /* Remove sub to sync */
    SBN_AppData.SubQ.Entries[SBN_AppData.SubQ.CurrVersion] =
        (SBN_SubChange_t) { .IsSubscribe = false, .MsgId = CFE_SB_MSGID_WRAP_VALUE(0x5678) };
    ++SBN_AppData.SubQ.CurrVersion;
    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerRemoteSubs(PeerRec, &BufD));
    UtAssert_UINT32_EQ(PeerRec->Priv.PeerSentSubVersion, SBN_AppData.SubQ.CurrVersion);

    /* Make things out of sync */
    SBN_AppData.SubQ.CurrVersion += SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH * 2;
    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerRemoteSubs(PeerRec, &BufD));

    /* Make it fail to send to the remote */
    /* in this case the local version should NOT increment, it should try again */
    UT_ResetState(UT_KEY(SBN_Outbound_SendToPeer));
    UT_SetDeferredRetcode(UT_KEY(SBN_Outbound_SendToPeer), 1, -1);
    SBN_AppData.SubQ.Entries[SBN_AppData.SubQ.CurrVersion] =
        (SBN_SubChange_t) { .IsSubscribe = true, .MsgId = CFE_SB_MSGID_WRAP_VALUE(0x5678) };
    ++SBN_AppData.SubQ.CurrVersion;
    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerRemoteSubs(PeerRec, &BufD));
    UtAssert_UINT32_EQ(PeerRec->Priv.PeerSentSubVersion, SBN_AppData.SubQ.CurrVersion - 1);
    UtAssert_STUB_COUNT(SBN_Outbound_SendToPeer, 1);

    /* Next cycle, this type SBN_Outbound_SendToPeer does not fail */
    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerRemoteSubs(PeerRec, &BufD));
    UtAssert_UINT32_EQ(PeerRec->Priv.PeerSentSubVersion, SBN_AppData.SubQ.CurrVersion);
    UtAssert_STUB_COUNT(SBN_Outbound_SendToPeer, 2);
}

void Test_SBN_Subs_UpdateConnectedPeerLocalSubs(void)
{
    START();

    /* Test Case For:
     * void SBN_Subs_UpdateConnectedPeerLocalSubs(SBN_PeerRecord_t *PeerRec);
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];
    CFE_SB_MsgId_t    MsgId;

    MsgId = CFE_SB_ValueToMsgId(1234);

    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerLocalSubs(PeerRec));
    UtAssert_UINT32_EQ(PeerRec->Priv.LocalPipeCurrSubVersion, PeerRec->Priv.SubQ.CurrVersion);

    /* Duplicate sub situation */
    PeerRec->Priv.SubQ.Entries[PeerRec->Priv.SubQ.CurrVersion].IsSubscribe = true;
    PeerRec->Priv.SubQ.Entries[PeerRec->Priv.SubQ.CurrVersion].MsgId       = MsgId;
    ++PeerRec->Priv.SubQ.CurrVersion;

    PeerRec->Intf.SubSet.Subs[0].MsgID = MsgId;
    PeerRec->Intf.SubSet.SubCnt        = 1;

    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerLocalSubs(PeerRec));
    UtAssert_UINT32_EQ(PeerRec->Priv.LocalPipeCurrSubVersion, PeerRec->Priv.SubQ.CurrVersion);

    /* Maximum sub situation */
    UT_CheckEvent_Setup(SBN_SUB_EID, "cannot process subscription from ProcessorID");
    MsgId                                                                  = CFE_SB_ValueToMsgId(4321);
    PeerRec->Priv.SubQ.Entries[PeerRec->Priv.SubQ.CurrVersion].IsSubscribe = true;
    PeerRec->Priv.SubQ.Entries[PeerRec->Priv.SubQ.CurrVersion].MsgId       = MsgId;
    ++PeerRec->Priv.SubQ.CurrVersion;

    PeerRec->Intf.SubSet.SubCnt = SBN_MISSION_MAX_SUBS_PER_PEER;

    UtAssert_VOIDCALL(SBN_Subs_UpdateConnectedPeerLocalSubs(PeerRec));
    EVENT_CNT(1);
    UtAssert_UINT32_EQ(PeerRec->Priv.LocalPipeCurrSubVersion, PeerRec->Priv.SubQ.CurrVersion);
}

void Test_SBN_Subs_SynchronizeAllPeers(void)
{
    START();

    /* Test Case For:
     * void SBN_Subs_SynchronizeAllPeers(void);
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];

    /* Nominal, disconnected */
    PeerRec->Intf.Connected = false;
    UtAssert_VOIDCALL(SBN_Subs_SynchronizeAllPeers());

    /* Nominal, connected, but nothing to sync */
    PeerRec->Intf.Connected = true;
    UtAssert_VOIDCALL(SBN_Subs_SynchronizeAllPeers());
    UtAssert_STUB_COUNT(CFE_SB_CreatePipe, 1);

    /* Add something to subscribe with preexisting pipe */
    PeerRec->Intf.Pipe = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(1));
    PeerRec->Priv.SubQ.Entries[PeerRec->Priv.SubQ.CurrVersion] =
        (SBN_SubChange_t) { .IsSubscribe = true, .MsgId = CFE_SB_MSGID_WRAP_VALUE(0x1234) };
    ++PeerRec->Priv.SubQ.CurrVersion;

    UtAssert_VOIDCALL(SBN_Subs_SynchronizeAllPeers());

    UtAssert_STUB_COUNT(CFE_SB_CreatePipe, 1);
    UtAssert_STUB_COUNT(CFE_SB_SubscribeLocal, 1);
    UtAssert_UINT32_EQ(PeerRec->Intf.SubSet.SubCnt, 1);
    UtAssert_UINT32_EQ(CFE_SB_MsgIdToValue(PeerRec->Intf.SubSet.Subs[0].MsgID), 0x1234);
    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, PeerRec->Priv.LocalPipeCurrSubVersion);

    /* Add something to unsubscribe */
    PeerRec->Priv.SubQ.Entries[PeerRec->Priv.SubQ.CurrVersion] =
        (SBN_SubChange_t) { .IsSubscribe = false, .MsgId = CFE_SB_MSGID_WRAP_VALUE(0x1234) };
    ++PeerRec->Priv.SubQ.CurrVersion;

    UtAssert_VOIDCALL(SBN_Subs_SynchronizeAllPeers());

    UtAssert_STUB_COUNT(CFE_SB_UnsubscribeLocal, 1);
    UtAssert_UINT32_EQ(PeerRec->Intf.SubSet.SubCnt, 0);
    UtAssert_UINT32_EQ(PeerRec->Priv.SubQ.CurrVersion, PeerRec->Priv.LocalPipeCurrSubVersion);

    /* Make things out of sync */
    PeerRec->Priv.SubQ.CurrVersion += SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH * 2;
    UtAssert_VOIDCALL(SBN_Subs_SynchronizeAllPeers());
}

void Test_SBN_Subs_AddToSet(void)
{
    /* Test Case For:
     * bool SBN_Subs_AddToSet(SBN_SubscriptionSet_t *SubSet, CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t QoS);
     */
    SBN_SubscriptionSet_t SubSet;
    SBN_SubCnt_t          Idx;

    memset(&SubSet, 0, sizeof(SubSet));

    for (Idx = 0; Idx < SBN_MISSION_MAX_SUBS_PER_PEER; ++Idx)
    {
        if (!SBN_Subs_AddToSet(&SubSet, CFE_SB_ValueToMsgId(Idx + 20), CFE_SB_DEFAULT_QOS))
        {
            break;
        }
    }

    UtAssert_UINT32_EQ(Idx, SBN_MISSION_MAX_SUBS_PER_PEER);

    /* One more than the max should fail */
    UtAssert_BOOL_FALSE(SBN_Subs_AddToSet(&SubSet, CFE_SB_ValueToMsgId(Idx), CFE_SB_DEFAULT_QOS));
    UtAssert_UINT32_EQ(SubSet.SubCnt, SBN_MISSION_MAX_SUBS_PER_PEER);
}

void Test_SBN_Subs_RemoveFromSet(void)
{
    /* Test Case For:
     * bool SBN_Subs_RemoveFromSet(SBN_SubscriptionSet_t *SubSet, SBN_SubCnt_t Idx)
     */
    SBN_SubscriptionSet_t SubSet;
    SBN_SubCnt_t          Idx;

    memset(&SubSet, 0, sizeof(SubSet));

    /* Something not in the set */
    UtAssert_BOOL_FALSE(SBN_Subs_RemoveFromSet(&SubSet, 1));

    for (Idx = 0; Idx < SBN_MISSION_MAX_SUBS_PER_PEER; ++Idx)
    {
        if (!SBN_Subs_AddToSet(&SubSet, CFE_SB_ValueToMsgId(Idx + 10), CFE_SB_DEFAULT_QOS))
        {
            break;
        }
    }

    /* Remove from the end */
    UtAssert_BOOL_TRUE(SBN_Subs_RemoveFromSet(&SubSet, SBN_MISSION_MAX_SUBS_PER_PEER - 1));

    /* Remove from the beginning */
    UtAssert_BOOL_TRUE(SBN_Subs_RemoveFromSet(&SubSet, 0));

    UtAssert_UINT32_EQ(SubSet.SubCnt, SBN_MISSION_MAX_SUBS_PER_PEER - 2);
    UtAssert_UINT32_EQ(CFE_SB_MsgIdToValue(SubSet.Subs[0].MsgID), 11);
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_SendSubsRequests_SendMsg1Err);
    ADD_TEST(SBN_SendSubsRequests_SendMsg2Err);
    ADD_TEST(SBN_SLS2P_SendNetMsgErr);
    ADD_TEST(SBN_CSP_PLS_MaxSubsErr);
    ADD_TEST(SBN_CSP_PLS_EvtMsg);
    ADD_TEST(SBN_CSP_PLS_SbnMsg);
    ADD_TEST(SBN_CSP_PLU_NotSub);
    ADD_TEST(SBN_CSP_PLU_Nominal);
    ADD_TEST(SBN_CSP_SubTypeErr);
    ADD_TEST(SBN_CSP_AllSubs_EntryCntErr);
    ADD_TEST(SBN_CSP_AllSubs_PLSErr);
    ADD_TEST(SBN_CSP_AllSubs_Nominal);
    ADD_TEST(SBN_CSP_NoMsg);
    ADD_TEST(SBN_PSFP_PFP_FiltErr);
    ADD_TEST(SBN_PSFP_PFP_AlreadySub);
    ADD_TEST(SBN_PSFP_PFP_MaxSub);
    ADD_TEST(SBN_PSFP_IdentErr);
    ADD_TEST(SBN_PSFP_Nominal);
    ADD_TEST(SBN_PUSFP_PUFP_FiltErr);
    ADD_TEST(SBN_PUSFP_PUFP_NotSub);
    ADD_TEST(SBN_PUSFP_IdentWarn);
    ADD_TEST(SBN_PUSFP_Nominal);
    ADD_TEST(SBN_UDPLS_NoPipe);
    ADD_TEST(SBN_UDPLS_Nominal);
    ADD_TEST(SBN_SendSubsRequests_Nominal);
    // ADD_TEST(SBN_Subs_PollCycle);
#ifdef jphfix
    ADD_TEST(SBN_Subs_SendLocalToPeer);
    ADD_TEST(SBN_ProcessSubsFromPeer);
    ADD_TEST(SBN_ProcessUnsubsFromPeer);
    ADD_TEST(SBN_Subs_UpdateDisconnectedPeerLocalSubs);
#endif
    ADD_TEST(SBN_Subs_GetPrevChangeEntry);
    ADD_TEST(SBN_Subs_SetupLocalPipe);
    ADD_TEST(SBN_Subs_UpdateConnectedPeerRemoteSubs);
    ADD_TEST(SBN_Subs_UpdateConnectedPeerLocalSubs);
    ADD_TEST(SBN_Subs_SynchronizeAllPeers);
    ADD_TEST(SBN_Subs_AddToSet);
    ADD_TEST(SBN_Subs_RemoveFromSet);
}
