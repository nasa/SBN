#include "sbn_coveragetest_common.h"
#include "sbn_app.h"
#include "cfe_msgids.h"
#include "cfe_sb_events.h"
#include "sbn_pack.h"

uint8 Buffer[1024];

CFE_MSG_Message_t *CmdPktPtr = (CFE_MSG_Message_t *)Buffer;
CFE_MSG_Size_t MsgSz = sizeof(CFE_MSG_CommandHeader_t);
CFE_SB_MsgId_t MsgId = SBN_CMD_MID;
CFE_MSG_FcnCode_t FcnCode = SBN_NOOP_CC;

#define MSGINIT() CFE_MSG_Init(CmdPktPtr, MsgId, MsgSz); UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &MsgId, sizeof(MsgId), false); UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz, sizeof(MsgSz), false); UT_SetDataBuffer(UT_KEY(CFE_MSG_GetFcnCode), &FcnCode, sizeof(FcnCode), false); UT_SetDataBuffer(UT_KEY(CFE_MSG_GetFcnCode), &FcnCode, sizeof(FcnCode), false);

static void NOOP_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid no-op command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_NOOP_CC;
    MSGINIT();

    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetSize), 1, -1);

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end NOOP_MsgLenErr() */

static void NOOP_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "no-op command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_NOOP_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end NOOP_Nominal() */

static void HKNet_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_HK_NET_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end HKNet_MsgLenErr() */

static void HKNet_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid NetIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + sizeof(CFE_MSG_CommandHeader_t);
    *Ptr++     = 255;
    *Ptr       = 0;

    MsgSz = SBN_CMD_NET_LEN;
    FcnCode = SBN_HK_NET_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKNet_NetIdErr() */

static void HKNet_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = SBN_CMD_NET_LEN;
    FcnCode = SBN_HK_NET_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKNet_Nominal() */

static void HKPeer_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_HK_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end HKPeer_MsgLenErr() */

static void HKPeer_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid NetIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + sizeof(CFE_MSG_CommandHeader_t);
    *Ptr++     = 255;
    *Ptr       = 0;

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKPeer_NetIdErr() */

static void HKPeer_PeerIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid PeerIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + sizeof(CFE_MSG_CommandHeader_t);
    *Ptr++     = 0;
    *Ptr++     = 255;

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKPeer_PeerIdErr() */

static void HKPeer_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKPeer_Nominal() */

static void HKPeerSubs_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command, net=");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_HK_PEERSUBS_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end HKPeerSubs_MsgLenErr() */

static void HKPeerSubs_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid NetIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + sizeof(CFE_MSG_CommandHeader_t);
    *Ptr++     = 255;
    *Ptr++     = 0;

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_PEERSUBS_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKPeerSubs_NetIdErr() */

static void HKPeerSubs_PeerIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid PeerIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + sizeof(CFE_MSG_CommandHeader_t);
    *Ptr++     = 0;
    *Ptr++     = 255;

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_PEERSUBS_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKPeerSubs_PeerIdErr() */

static void HKPeerSubs_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command, net=");

    memset(Buffer, 0, sizeof(Buffer));

    PeerPtr->SubCnt = 1;

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_PEERSUBS_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKPeerSubs_Nominal() */

static void HKMySubs_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_HK_MYSUBS_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end HKMySubs_MsgLenErr() */

static void HKMySubs_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command");

    memset(Buffer, 0, sizeof(Buffer));

    SBN.SubCnt = 1;

    MsgSz = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_HK_MYSUBS_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKMySubs_Nominal() */

static void HKReset_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reset command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_HK_RESET_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end HKReset_MsgLenErr() */

static void HKReset_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reset command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_HK_RESET_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKReset_Nominal() */

static void HKResetPeer_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length (Name=");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_HK_RESET_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKResetPeer_MsgLenErr() */

static void HKResetPeer_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid net idx ");

    memset(Buffer, 0, sizeof(Buffer));

    uint8 *Ptr = Buffer + sizeof(CFE_MSG_CommandHeader_t);
    *Ptr++     = 255;

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_RESET_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKResetPeer_NetIdErr() */

static void HKResetPeer_PeerIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid peer idx ");

    memset(Buffer, 0, sizeof(Buffer));

    uint8 *Ptr = Buffer + sizeof(CFE_MSG_CommandHeader_t);
    *Ptr++     = 0;
    *Ptr++     = 255;

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_RESET_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKResetPeer_PeerIdErr() */

static void HKResetPeer_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk reset peer command (NetIdx=");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = SBN_CMD_PEER_LEN;
    FcnCode = SBN_HK_RESET_PEER_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HKResetPeer_Nominal() */

static void SCH_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "wakeup");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_SCH_WAKEUP_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end SCH_Nominal() */

static void TBL_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reload tbl command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_TBL_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end TBL_MsgLenErr() */

static void TBL_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reload tbl command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_TBL_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end TBL_Nominal() */

static void CC_Err(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid command code (ID=0x");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_TBL_CC + 10;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end CC_Err() */

static void HK_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = -1;
    FcnCode = SBN_HK_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end HK_MsgLenErr() */

static void HK_MsgLenErr2(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command");

    memset(Buffer, 0, sizeof(Buffer));

    FcnCode = SBN_HK_CC;
    MsgSz = 0; /* force an invalid size, should be skipped */
    MSGINIT();

    CFE_MSG_Message_t *CmdPktPtr2 = (CFE_MSG_Message_t *)Buffer;
    CFE_MSG_Size_t MsgSz2 = sizeof(CFE_MSG_CommandHeader_t);

    CFE_MSG_Init(CmdPktPtr2, MsgId, MsgSz2);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &MsgId, sizeof(MsgId), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MsgSz2, sizeof(MsgSz2), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetFcnCode), &FcnCode, sizeof(FcnCode), false);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetFcnCode), &FcnCode, sizeof(FcnCode), false);

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(0);
} /* end HK_MsgLenErr2() */

static void HK_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command");

    memset(Buffer, 0, sizeof(Buffer));

    MsgSz = sizeof(CFE_MSG_CommandHeader_t);
    FcnCode = SBN_HK_CC;
    MSGINIT();

    SBN_HandleCommand(CmdPktPtr);

    EVENT_CNT(1);
} /* end HK_Nominal() */

static void Test_SBN_Cmds(void)
{
    NOOP_MsgLenErr();
    NOOP_Nominal();
    HKNet_MsgLenErr();
    HKNet_NetIdErr();
    HKNet_Nominal();
    HKPeer_MsgLenErr();
    HKPeer_NetIdErr();
    HKPeer_PeerIdErr();
    HKPeer_Nominal();
    HKPeerSubs_MsgLenErr();
    HKPeerSubs_NetIdErr();
    HKPeerSubs_PeerIdErr();
    HKPeerSubs_Nominal();
    HKMySubs_MsgLenErr();
    HKMySubs_Nominal();
    HKReset_MsgLenErr();
    HKReset_Nominal();
    HKResetPeer_MsgLenErr();
    HKResetPeer_NetIdErr();
    HKResetPeer_PeerIdErr();
    HKResetPeer_Nominal();
    SCH_Nominal();
    TBL_MsgLenErr();
    TBL_Nominal();
    HK_MsgLenErr();
    HK_MsgLenErr2();
    HK_Nominal();
    CC_Err();
} /* end Test_SBN_SendNetMsg() */

void UT_Setup(void) {} /* end UT_Setup() */

void UT_TearDown(void) {} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Cmds);
}
