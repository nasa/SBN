#include "sbn_coveragetest_common.h"
#include "cfe_msgids.h"
#include "sbn_pack.h"

CFE_SB_MsgId_t MsgID = 0xDEAD;

static void SendSubsRequests_SendMsg1Err(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "Unable to turn on sub reporting (status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SendMsg), 1, -1);

    UtAssert_INT32_EQ(SBN_SendSubsRequests(), SBN_ERROR);
    EVENT_CNT(1);
}/* end SendSubsRequests_SendMsg1Err() */

static void SendSubsRequests_SendMsg2Err(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "Unable to send prev subs request (status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SendMsg), 2, -1);

    UtAssert_INT32_EQ(SBN_SendSubsRequests(), SBN_ERROR);
    EVENT_CNT(1);
}/* end SendSubsRequests_SendMsg2Err() */

void Test_SBN_SendSubsRequests(void)
{
    SendSubsRequests_SendMsg1Err();
    SendSubsRequests_SendMsg2Err();
}/* end Test_SBN_SendSubsRequests() */

static void SLS2P_SendNetMsgErr(void)
{
    START();

    SBN.SubCnt = 1;
    SBN.Subs[0].MsgID = MsgID;

    SBN.Nets[0].Peers[1].Net = NetPtr;

    IfOpsPtr->Send = Send_Err;

    UtAssert_INT32_EQ(SBN_SendLocalSubsToPeer(&SBN.Nets[0].Peers[1]), SBN_ERROR);

    IfOpsPtr->Send = Send_Nominal;
}/* end SLS2P_SendNetMsgErr() */

void Test_SBN_SendLocalSubsToPeer(void)
{
    SLS2P_SendNetMsgErr();
}/* end Test_SBN_SendLocalSubsToPeer() */

static void CSP_PLS_MaxSubsErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "local subscription ignored for MsgID 0x");

    SBN.SubCnt = SBN_MAX_SUBS_PER_PEER;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_ERROR);

    EVENT_CNT(1);
}/* end CSP_PLS_MaxSubsErr() */

static void CSP_PLS_AddlSubs(void)
{
    START();

    SBN.SubCnt = 1;
    SBN.Subs[0].InUseCtr = 1;
    SBN.Subs[0].MsgID = MsgID;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_SUCCESS);

    UtAssert_INT32_EQ(SBN.Subs[0].InUseCtr, 2);
}/* end CSP_PLS_AddlSubs() */

static void CSP_PLS_SendErr(void)
{
    START();

    IfOpsPtr->Send = Send_Err;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_ERROR);

    IfOpsPtr->Send = Send_Nominal;
}/* end CSP_PLS_SendErr() */

static void CSP_PLU_NotSub(void)
{
    START();

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION;
    Msg.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_SUCCESS);
}/* end CSP_PLU_NotSub() */

static void CSP_PLU_OtherSub(void)
{
    START();

    SBN.SubCnt = 1;
    SBN.Subs[0].InUseCtr = 2;
    SBN.Subs[0].MsgID = MsgID;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION;
    Msg.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_SUCCESS);

    UtAssert_INT32_EQ(SBN.Subs[0].InUseCtr, 1);
    UtAssert_INT32_EQ(SBN.SubCnt, 1);
}/* end CSP_PLU_OtherSub() */

static void CSP_PLU_SLS2PErr(void)
{
    START();

    SBN.SubCnt = 1;
    SBN.Subs[0].InUseCtr = 1;
    SBN.Subs[0].MsgID = MsgID;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION;
    Msg.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    IfOpsPtr->Send = Send_Err;

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_ERROR);

    IfOpsPtr->Send = Send_Nominal;
}/* end CSP_PLU_SLS2PErr() */

static void CSP_PLU_Nominal(void)
{
    START();

    SBN.SubCnt = 1;
    SBN.Subs[0].InUseCtr = 1;
    SBN.Subs[0].MsgID = MsgID;

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION;
    Msg.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_SUCCESS);

    UtAssert_INT32_EQ(SBN.Subs[0].InUseCtr, 0);
    UtAssert_INT32_EQ(SBN.SubCnt, 0);
}/* end CSP_PLU_Nominal() */

static void CSP_SubTypeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "unexpected subscription type (");

    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_UNSUBSCRIPTION + 10; /* invalid subtype */
    Msg.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_ERROR);

    UtAssert_INT32_EQ(SBN.Subs[0].InUseCtr, 0);
    UtAssert_INT32_EQ(SBN.SubCnt, 0);

    EVENT_CNT(1);
}/* end CSP_SubTypeErr() */

static void CSP_AllSubs_EntryCntErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "entries value ");
 
    CFE_SB_AllSubscriptionsTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.Entries = CFE_SB_SUB_ENTRIES_PER_PKT + 1;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ALLSUBS_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_ERROR);

    UtAssert_INT32_EQ(SBN.Subs[0].InUseCtr, 0);
    UtAssert_INT32_EQ(SBN.SubCnt, 0);

    EVENT_CNT(1);
}/* end CSP_AllSubs_EntryCntErr() */

static void CSP_AllSubs_PLSErr(void)
{
    START();

    /* err generated by ProcessLocalSub() */
    UT_CheckEvent_Setup(SBN_SUB_EID, "local subscription ignored for MsgID 0x");

    SBN.SubCnt = SBN_MAX_SUBS_PER_PEER;

    CFE_SB_AllSubscriptionsTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.Entries = 1;
    Msg.Payload.Entry[0].MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ALLSUBS_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_ERROR);

    EVENT_CNT(1);
}/* end CSP_AllSubs_PLSErr() */

static void CSP_AllSubs_Nominal(void)
{
    START();

    CFE_SB_AllSubscriptionsTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.Entries = 1;
    Msg.Payload.Entry[0].MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ALLSUBS_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_SUCCESS);

    UtAssert_INT32_EQ(SBN.Subs[0].InUseCtr, 1);
    UtAssert_INT32_EQ(SBN.SubCnt, 1);
}/* end CSP_AllSubs_Nominal() */

static void CSP_NoMsg(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_NO_MESSAGE);

    UtAssert_INT32_EQ(SBN_CheckSubscriptionPipe(), SBN_IF_EMPTY);
}/* end CSP_NoMsg() */

void Test_SBN_CheckSubscriptionPipe(void)
{
    CSP_PLS_MaxSubsErr();
    CSP_PLS_AddlSubs();
    CSP_PLS_SendErr();
    CSP_PLU_NotSub();
    CSP_PLU_OtherSub();
    CSP_PLU_SLS2PErr();
    CSP_PLU_Nominal();
    CSP_SubTypeErr();
    CSP_AllSubs_EntryCntErr();
    CSP_AllSubs_PLSErr();
    CSP_AllSubs_Nominal();
    CSP_NoMsg();
}/* end Test_SBN_CheckSubscriptionPipe() */

static SBN_Status_t RemapMID_Err(CFE_SB_MsgId_t *FromToMidPtr, SBN_Filter_Ctx_t *Context) 
{
    return SBN_ERROR;
}/* end RemapMID_Err() */

static void PSFP_PFP_FiltErr(void)
{
    START();

    PeerPtr->FilterCnt = 2;
    SBN_FilterInterface_t Filter1, Filter2;
    memset(&Filter1, 0, sizeof(Filter1));
    memset(&Filter2, 0, sizeof(Filter2));
    Filter2.RemapMID = RemapMID_Err;
    PeerPtr->Filters[0] = &Filter1;
    PeerPtr->Filters[1] = &Filter2;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessSubsFromPeer(PeerPtr, Buf), SBN_ERROR);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);
}/* end PSFP_PFP_FiltErr() */

static void PSFP_PFP_AlreadySub(void)
{
    START();

    PeerPtr->SubCnt = 1;
    PeerPtr->Subs[0].MsgID = MsgID;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessSubsFromPeer(PeerPtr, Buf), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 1);

    EVENT_CNT(1);
}/* end PSFP_PFP_AlreadySub() */

static void PSFP_PFP_SubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "unable to subscribe to MID 0x");

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, -1);

    UtAssert_INT32_EQ(SBN_ProcessSubsFromPeer(PeerPtr, Buf), SBN_ERROR);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);

    EVENT_CNT(1);
}/* end PSFP_PFP_SubErr() */

static void PSFP_PFP_MaxSubsErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "cannot process subscription from ProcessorID ");

    PeerPtr->SubCnt = SBN_MAX_SUBS_PER_PEER;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessSubsFromPeer(PeerPtr, Buf), SBN_ERROR);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, SBN_MAX_SUBS_PER_PEER);

    EVENT_CNT(1);
}/* end PSFP_PFP_MaxSubsErr() */

static void PSFP_IdentErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PROTO_EID, "version number mismatch with peer CpuID ");

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    char tmpident[SBN_IDENT_LEN];
    memset(tmpident, 0, sizeof(tmpident));
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)tmpident, sizeof(tmpident));
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessSubsFromPeer(PeerPtr, Buf), SBN_ERROR);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);

    EVENT_CNT(1);
}/* end PSFP_IdentErr() */

static void PSFP_Nominal(void)
{
    START();

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessSubsFromPeer(PeerPtr, Buf), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 1);
}/* end PSFP_Nominal() */

void Test_SBN_ProcessSubsFromPeer(void)
{
    PSFP_PFP_FiltErr();
    PSFP_PFP_AlreadySub();
    PSFP_PFP_SubErr();
    PSFP_PFP_MaxSubsErr();
    PSFP_IdentErr();
    PSFP_Nominal();
}/* end Test_SBN_ProcessSubsFromPeer() */

static void PUSFP_PUFP_FiltErr(void)
{
    START();

    PeerPtr->SubCnt = 1;
    PeerPtr->Subs[0].MsgID = MsgID;
    PeerPtr->FilterCnt = 2;
    SBN_FilterInterface_t Filter1, Filter2;
    memset(&Filter1, 0, sizeof(Filter1));
    memset(&Filter2, 0, sizeof(Filter2));
    Filter2.RemapMID = RemapMID_Err;
    PeerPtr->Filters[0] = &Filter1;
    PeerPtr->Filters[1] = &Filter2;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessUnsubsFromPeer(PeerPtr, Buf), SBN_SUCCESS);

    /* ProcessUnsubsFromPeer() ignores any subs that the filter remap function returns an err */
    UtAssert_INT32_EQ(PeerPtr->SubCnt, 1);
}/* end PUSFP_PUFP_FiltErr() */

static void PUSFP_PUFP_NotSub(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "cannot process unsubscription from ProcessorID ");

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessUnsubsFromPeer(PeerPtr, Buf), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);

    EVENT_CNT(1);
}/* end PUSFP_PUFP_NotSub() */

static void PUSFP_PUFP_UnsubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "unable to unsubscribe from MID 0x");

    PeerPtr->SubCnt = 1;
    PeerPtr->Subs[0].MsgID = MsgID;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_UnsubscribeLocal), 1, -1);

    UtAssert_INT32_EQ(SBN_ProcessUnsubsFromPeer(PeerPtr, Buf), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);

    EVENT_CNT(1);
}/* end PUSFP_PUFP_UnsubErr() */

static void PUSFP_IdentWarn(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PROTO_EID, "version number mismatch with peer CpuID ");

    PeerPtr->SubCnt = 1;
    PeerPtr->Subs[0].MsgID = MsgID;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    char tmpident[SBN_IDENT_LEN];
    memset(tmpident, 0, sizeof(tmpident));
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)tmpident, sizeof(tmpident));
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessUnsubsFromPeer(PeerPtr, Buf), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);

    EVENT_CNT(1);
}/* end PUSFP_IdentWarn() */

static void PUSFP_Nominal(void)
{
    START();

    PeerPtr->SubCnt = 1;
    PeerPtr->Subs[0].MsgID = MsgID;

    uint8 Buf[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UtAssert_INT32_EQ(SBN_ProcessUnsubsFromPeer(PeerPtr, Buf), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);
}/* end PUSFP_Nominal() */

void Test_SBN_ProcessUnsubsFromPeer(void)
{
    PUSFP_PUFP_FiltErr();
    PUSFP_PUFP_NotSub();
    PUSFP_PUFP_UnsubErr();
    PUSFP_IdentWarn();
    PUSFP_Nominal();
}/* end Test_SBN_ProcessUnsubsFromPeer() */

static void RASFP_UnsubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "unable to unsubscribe from message id 0x");

    PeerPtr->SubCnt = 1;
    PeerPtr->Subs[0].MsgID = MsgID;

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_UnsubscribeLocal), 1, -1);

    /* errors are generate events but still successful return */
    UtAssert_INT32_EQ(SBN_RemoveAllSubsFromPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end RASFP_UnsubErr() */

static void RASFP_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SUB_EID, "unsubscribed 1 message id's from ProcessorID ");

    PeerPtr->SubCnt = 1;
    PeerPtr->Subs[0].MsgID = MsgID;

    UtAssert_INT32_EQ(SBN_RemoveAllSubsFromPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end RASFP_Nominal() */

void Test_SBN_RemoveAllSubsFromPeer(void)
{
    RASFP_UnsubErr();
    RASFP_Nominal();
}/* end Test_SBN_RemoveAllSubsFromPeer() */

void UT_Setup(void)
{
}/* end UT_Setup() */

void UT_TearDown(void)
{
}/* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_SendSubsRequests);
    ADD_TEST(SBN_SendLocalSubsToPeer);
    ADD_TEST(SBN_CheckSubscriptionPipe);
    ADD_TEST(SBN_ProcessSubsFromPeer);
    ADD_TEST(SBN_ProcessUnsubsFromPeer);
    ADD_TEST(SBN_RemoveAllSubsFromPeer);
}
