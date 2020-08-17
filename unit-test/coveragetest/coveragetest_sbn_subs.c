#include "sbn_coveragetest_common.h"
#include "cfe_msgids.h"

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

void Test_SBN_CheckSubscriptionPipe(void)
{
    CSP_PLS_MaxSubsErr();
    CSP_PLS_AddlSubs();
    CSP_PLU_Nominal();
}/* end Test_SBN_CheckSubscriptionPipe() */

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
}
