#include "sbn_coveragetest_common.h"
#include "sbn_app.h"
#include "cfe_msgids.h"
#include "cfe_sb_events.h"
#include "sbn_pack.h"

uint8 Buffer[1024];
CCSDS_CommandPacket_t *CmdPktPtr = (CCSDS_CommandPacket_t *)Buffer;

static void NOOP_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid no-op command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_NOOP_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, -1);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end NOOP_MsgLenErr() */

static void NOOP_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "no-op command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_NOOP_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, CFE_SB_CMD_HDR_SIZE);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end NOOP_Nominal() */

static void HKNet_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_NET_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_NET_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, -1);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end HKNet_MsgLenErr() */

static void HKNet_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid NetIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + CFE_SB_CMD_HDR_SIZE;
    *Ptr++ = 255;
    *Ptr = 0;
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_NET_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_NET_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_NET_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKNet_NetIdErr() */

static void HKNet_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_NET_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_NET_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_NET_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKNet_Nominal() */

static void HKPeer_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, -1);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end HKPeer_MsgLenErr() */

static void HKPeer_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid NetIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + CFE_SB_CMD_HDR_SIZE;
    *Ptr++ = 255;
    *Ptr = 0;
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKPeer_NetIdErr() */

static void HKPeer_PeerIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid PeerIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + CFE_SB_CMD_HDR_SIZE;
    *Ptr++ = 0;
    *Ptr++ = 255;
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKPeer_PeerIdErr() */

static void HKPeer_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command, net=");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKPeer_Nominal() */

static void HKPeerSubs_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command, net=");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEERSUBS_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, 0);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end HKPeerSubs_MsgLenErr() */

static void HKPeerSubs_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid NetIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + CFE_SB_CMD_HDR_SIZE;
    *Ptr++ = 255;
    *Ptr++ = 0;
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEERSUBS_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKPeerSubs_NetIdErr() */

static void HKPeerSubs_PeerIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "Invalid PeerIdx (");

    memset(Buffer, 0, sizeof(Buffer));
    uint8 *Ptr = Buffer + CFE_SB_CMD_HDR_SIZE;
    *Ptr++ = 0;
    *Ptr++ = 255;
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEERSUBS_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKPeerSubs_PeerIdErr() */

static void HKPeerSubs_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command, net=");

    memset(Buffer, 0, sizeof(Buffer));

    PeerPtr->SubCnt = 1;
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_PEERSUBS_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKPeerSubs_Nominal() */

static void HKMySubs_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_MYSUBS_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, 0);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end HKMySubs_MsgLenErr() */

static void HKMySubs_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk subs command");

    memset(Buffer, 0, sizeof(Buffer));

    SBN.SubCnt = 1;
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_MYSUBS_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, CFE_SB_CMD_HDR_SIZE);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKMySubs_Nominal() */

static void HKReset_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reset command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_RESET_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, -1);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end HKReset_MsgLenErr() */

static void HKReset_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reset command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_RESET_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, CFE_SB_CMD_HDR_SIZE);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKReset_Nominal() */

static void HKResetPeer_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid message length (Name=");

    memset(Buffer, 0, sizeof(Buffer));

    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_RESET_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, 0);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKResetPeer_MsgLenErr() */

static void HKResetPeer_NetIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid net idx ");

    memset(Buffer, 0, sizeof(Buffer));

    uint8 *Ptr = Buffer + CFE_SB_CMD_HDR_SIZE;
    *Ptr++ = 255;

    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_RESET_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKResetPeer_NetIdErr() */

static void HKResetPeer_PeerIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid peer idx ");

    memset(Buffer, 0, sizeof(Buffer));

    uint8 *Ptr = Buffer + CFE_SB_CMD_HDR_SIZE;
    *Ptr++ = 0;
    *Ptr++ = 255;

    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_RESET_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKResetPeer_PeerIdErr() */

static void HKResetPeer_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk reset peer command (NetIdx=");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, SBN_CMD_PEER_LEN);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_RESET_PEER_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, SBN_CMD_PEER_LEN);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HKResetPeer_Nominal() */

static void SCH_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "wakeup");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_SCH_WAKEUP_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, CFE_SB_CMD_HDR_SIZE);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end SCH_Nominal() */

static void TBL_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reload tbl command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_TBL_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, 0);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end TBL_MsgLenErr() */

static void TBL_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "reload tbl command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_TBL_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, CFE_SB_CMD_HDR_SIZE);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end TBL_Nominal() */

static void CC_Err(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "invalid command code (ID=0x");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_TBL_CC + 10);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, CFE_SB_CMD_HDR_SIZE);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end CC_Err() */

static void HK_MsgLenErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, 0);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end HK_MsgLenErr() */

static void HK_MsgLenErr2(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command");

    memset(Buffer, 0, sizeof(Buffer));
    
    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, 0);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(0);
}/* end HK_MsgLenErr2() */

static void HK_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_CMD_EID, "hk command");

    memset(Buffer, 0, sizeof(Buffer));

    CCSDS_WR_LEN(CmdPktPtr->SpacePacket.Hdr, CFE_SB_CMD_HDR_SIZE);
    uint32 mid = SBN_CMD_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetCmdCode), 1, SBN_HK_CC);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_GetTotalMsgLength), 1, CFE_SB_CMD_HDR_SIZE);

    SBN_HandleCommand((CFE_SB_MsgPtr_t)CmdPktPtr);

    EVENT_CNT(1);
}/* end HK_Nominal() */

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
}/* end Test_SBN_SendNetMsg() */

void UT_Setup(void)
{
}/* end UT_Setup() */

void UT_TearDown(void)
{
}/* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Cmds);
}
