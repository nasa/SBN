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
#include "sbn_dispatch.h"

union
{
    CFE_SB_Buffer_t         Buf;
    CFE_MSG_CommandHeader_t CommandHeader;

    SBN_NoopCmd_t           NoopCmd;
    SBN_SendHkCmd_t         SendHkCmd;
    SBN_SendHkNetCmd_t      SendHkNetCmd;
    SBN_SendHkPeerCmd_t     SendHkPeerCmd;
    SBN_SendHkPeerSubsCmd_t SendHkPeerSubsCmd;
    SBN_SendHkMySubsCmd_t   SendHkMySubsCmd;
    SBN_ResetCmd_t          ResetCmd;
    SBN_ResetPeerCmd_t      ResetPeerCmd;
    SBN_WakeupCmd_t         WakeupCmd;
    SBN_ReloadTblCmd_t      ReloadTblCmd;

} Buffer;

CFE_MSG_Message_t *CmdPktPtr = &Buffer.Buf.Msg;
CFE_MSG_Size_t     MsgSz     = sizeof(CFE_MSG_CommandHeader_t);
CFE_MSG_FcnCode_t  FcnCode   = SBN_NOOP_CC;

void SBN_UT_Dispatch_MsgInitHelper(void)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_ValueToMsgId(SBN_CMD_MID);

    /* Note - this sets the buffers multiple times because these accessors
     * are called multiple times in the error paths */
    CFE_MSG_Init(CmdPktPtr, MsgId, MsgSz);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &MsgId, sizeof(MsgId), true);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &MsgId, sizeof(MsgId), true);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetFcnCode), &FcnCode, sizeof(FcnCode), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetFcnCode), &FcnCode, sizeof(FcnCode), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetFcnCode), &FcnCode, sizeof(FcnCode), false);
}

void Test_SBN_Dispatch_NOOP_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");
    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_NOOP_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_NoopCmd, 0);
    EVENT_CNT(1);
} /* end NOOP_MsgLenErr() */

void Test_SBN_Dispatch_NOOP_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(SBN_NoopCmd_t);
    FcnCode = SBN_NOOP_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);
    UtAssert_STUB_COUNT(SBN_NoopCmd, 1);
} /* end NOOP_Nominal() */

void Test_SBN_Dispatch_HKNet_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");
    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_HK_NET_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkNetCmd, 0);
    EVENT_CNT(1);
} /* end HKNet_MsgLenErr() */

void Test_SBN_Dispatch_HKNet_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(SBN_SendHkNetCmd_t);
    FcnCode = SBN_HK_NET_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkNetCmd, 1);
} /* end HKNet_Nominal() */

void Test_SBN_Dispatch_HKPeer_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_HK_PEER_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkPeerCmd, 0);
    EVENT_CNT(1);
} /* end HKPeer_MsgLenErr() */

void Test_SBN_Dispatch_HKPeer_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(SBN_SendHkPeerCmd_t);
    FcnCode = SBN_HK_PEER_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkPeerCmd, 1);
} /* end HKPeer_Nominal() */

void Test_SBN_Dispatch_HKPeerSubs_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_HK_PEERSUBS_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkPeerSubsCmd, 0);
    EVENT_CNT(1);
} /* end HKPeerSubs_MsgLenErr() */

void Test_SBN_Dispatch_HKPeerSubs_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    PeerPtr->SubSet.SubCnt = 1;

    MsgSz   = sizeof(SBN_SendHkPeerSubsCmd_t);
    FcnCode = SBN_HK_PEERSUBS_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkPeerSubsCmd, 1);
} /* end HKPeerSubs_Nominal() */

void Test_SBN_Dispatch_HKMySubs_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_HK_MYSUBS_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkMySubsCmd, 0);
    EVENT_CNT(1);
} /* end HKMySubs_MsgLenErr() */

void Test_SBN_Dispatch_HKMySubs_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    SBN_AppData.SubSet.SubCnt = 1;

    MsgSz   = sizeof(SBN_SendHkMySubsCmd_t);
    FcnCode = SBN_HK_MYSUBS_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkMySubsCmd, 1);
} /* end HKMySubs_Nominal() */

void Test_SBN_Dispatch_HKReset_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_HK_RESET_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_ResetCmd, 0);
    EVENT_CNT(1);
} /* end HKReset_MsgLenErr() */

void Test_SBN_Dispatch_HKReset_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_HK_RESET_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_ResetCmd, 1);
} /* end HKReset_Nominal() */

void Test_SBN_Dispatch_HKResetPeer_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length (Name=");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_HK_RESET_PEER_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_ResetPeerCmd, 0);
    EVENT_CNT(1);
} /* end HKResetPeer_MsgLenErr() */

void Test_SBN_Dispatch_HKResetPeer_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(SBN_ResetPeerCmd_t);
    FcnCode = SBN_HK_RESET_PEER_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_ResetPeerCmd, 1);
} /* end HKResetPeer_Nominal() */

void Test_SBN_Dispatch_SCH_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(SBN_WakeupCmd_t);
    FcnCode = SBN_SCH_WAKEUP_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_WakeupCmd, 1);
} /* end SCH_Nominal() */

void Test_SBN_Dispatch_SCH_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(SBN_WakeupCmd_t) - 1;
    FcnCode = SBN_SCH_WAKEUP_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_WakeupCmd, 0);
    EVENT_CNT(1);
} /* end SCH_Nominal() */

void Test_SBN_Dispatch_TBL_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length ");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_TBL_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_ReloadTblCmd, 0);
    EVENT_CNT(1);
} /* end TBL_MsgLenErr() */

void Test_SBN_Dispatch_TBL_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(SBN_ReloadTblCmd_t);
    FcnCode = SBN_TBL_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_ReloadTblCmd, 1);
} /* end TBL_Nominal() */

void Test_SBN_Dispatch_CC_Err(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid command code (ID=0x");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_TBL_CC + 10;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    EVENT_CNT(1);
} /* end CC_Err() */

void Test_SBN_Dispatch_CC_Err2(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "unable to get FcnCode");

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = 1;
    FcnCode = SBN_HK_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetFcnCode), 2, CFE_SB_BAD_ARGUMENT);

    SBN_HandleCommand(&Buffer.Buf);

    EVENT_CNT(1);
} /* end CC_Err() */

void Test_SBN_Dispatch_HK_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "error getting msg size");
    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = -1;
    FcnCode = SBN_HK_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetSize), 1, CFE_SB_BAD_ARGUMENT);

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkCmd, 0);
    EVENT_CNT(1);
} /* end HK_MsgLenErr() */

void Test_SBN_Dispatch_HK_MsgLenErr2(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid hk message length");
    memset(&Buffer, 0, sizeof(Buffer));

    FcnCode = SBN_HK_CC;
    MsgSz   = 0; /* force an invalid size, should be skipped */
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkCmd, 0);
    EVENT_CNT(1);
} /* end HK_MsgLenErr2() */

void Test_SBN_Dispatch_HK_Nominal(void)
{
    START();

    memset(&Buffer, 0, sizeof(Buffer));

    MsgSz   = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_HK_CC;
    SBN_UT_Dispatch_MsgInitHelper();

    SBN_HandleCommand(&Buffer.Buf);

    UtAssert_STUB_COUNT(SBN_SendHkCmd, 1);
} /* end HK_Nominal() */

void Test_SBN_HandleCommand(void)
{
    START();

    /* Test Case For:
     * void SBN_HandleCommand(const CFE_SB_Buffer_t *SBBufPtr)
     */
    CFE_SB_MsgId_t MsgId;

    memset(&Buffer, 0, sizeof(Buffer));

    /* this checks the error paths */
    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid FcnCode");
    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetMsgId), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_VOIDCALL(SBN_HandleCommand(&Buffer.Buf));
    EVENT_CNT(1);

    /* not the CMD mid */
    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid command pipe MsgId");
    MsgId = CFE_SB_ValueToMsgId(SBN_CMD_MID + 1);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &MsgId, sizeof(MsgId), false);
    UtAssert_VOIDCALL(SBN_HandleCommand(&Buffer.Buf));
    EVENT_CNT(1);

    /* function code 2 */
    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid FcnCode (FcnCode");
    MsgId = CFE_SB_ValueToMsgId(SBN_CMD_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &MsgId, sizeof(MsgId), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetFcnCode), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_VOIDCALL(SBN_HandleCommand(&Buffer.Buf));
    EVENT_CNT(1);
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_HandleCommand);
    ADD_TEST(SBN_Dispatch_NOOP_MsgLenErr);
    ADD_TEST(SBN_Dispatch_NOOP_Nominal);
    ADD_TEST(SBN_Dispatch_HKNet_MsgLenErr);
    ADD_TEST(SBN_Dispatch_HKNet_Nominal);
    ADD_TEST(SBN_Dispatch_HKPeer_MsgLenErr);
    ADD_TEST(SBN_Dispatch_HKPeer_Nominal);
    ADD_TEST(SBN_Dispatch_HKPeerSubs_MsgLenErr);
    ADD_TEST(SBN_Dispatch_HKPeerSubs_Nominal);
    ADD_TEST(SBN_Dispatch_HKMySubs_MsgLenErr);
    ADD_TEST(SBN_Dispatch_HKMySubs_Nominal);
    ADD_TEST(SBN_Dispatch_HKReset_MsgLenErr);
    ADD_TEST(SBN_Dispatch_HKReset_Nominal);
    ADD_TEST(SBN_Dispatch_HKResetPeer_MsgLenErr);
    ADD_TEST(SBN_Dispatch_HKResetPeer_Nominal);
    ADD_TEST(SBN_Dispatch_SCH_MsgLenErr);
    ADD_TEST(SBN_Dispatch_SCH_Nominal);
    ADD_TEST(SBN_Dispatch_TBL_MsgLenErr);
    ADD_TEST(SBN_Dispatch_TBL_Nominal);
    ADD_TEST(SBN_Dispatch_CC_Err);
    ADD_TEST(SBN_Dispatch_CC_Err2);
    ADD_TEST(SBN_Dispatch_HK_MsgLenErr);
    ADD_TEST(SBN_Dispatch_HK_MsgLenErr2);
    ADD_TEST(SBN_Dispatch_HK_Nominal);
}
