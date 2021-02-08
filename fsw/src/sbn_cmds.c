#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "sbn_app.h"
#include "sbn_pack.h"

/**
 * @brief Initializes the housekeeping counters for a peer.
 *
 * @param[in] Peer The peer interface for which to reset housekeeping.
 */
static void InitializePeerCounters(SBN_PeerInterface_t *Peer)
{
    Peer->LastSend   = (OS_time_t) {0};
    Peer->LastRecv   = (OS_time_t) {0};
    Peer->SendCnt    = 0;
    Peer->RecvCnt    = 0;
    Peer->SendErrCnt = 0;
    Peer->RecvErrCnt = 0;
} /* end InitializePeerCounters() */

/**
 * @brief Initializes the housekeeping counters both at the app level
 *        and for each peer.
 */
void SBN_InitializeCounters(void)
{
    SBN.CmdCnt    = 0;
    SBN.CmdErrCnt = 0;

    int NetIdx = 0;
    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net     = &SBN.Nets[NetIdx];
        int                 PeerIdx = 0;
        for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];
            InitializePeerCounters(Peer);
        } /* end for */
    }     /* end for */
} /* end SBN_InitializeCounters */

/************************************************************************/
/** \brief Verify message length.
**
**  \par Description
**       Checks if the actual length of a software bus message matches
**       the expected length and sends an error event if a mismatch
**       occurs.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   msg           A #CFE_MSG_Message_t pointer that
**                              references the software bus message
**
**  \param [in]   ExpectedLen   The expected length of the message
**                              based upon the command code.
**
**  @param [in]   name          Text name of the message expected.
**
**  \returns
**  \retstmt Returns true if the length is as expected      \endcode
**  \retstmt Returns false if the length is not as expected \endcode
**  \endreturns
**
**  \sa #SBN_LEN_EID
**
*************************************************************************/
static bool VerifyMsgLen(CFE_MSG_Message_t *MsgPtr, uint16 ExpectedLen, const char *MsgName)
{
    CFE_MSG_Size_t    ActualLen = 0;
    CFE_MSG_FcnCode_t FcnCode   = 0;

    if(CFE_MSG_GetSize(MsgPtr, &ActualLen) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_CMD_EID, "invalid hk message (Name=%s)", MsgName);
        return false;
    }

    if (ExpectedLen != ActualLen)
    {
        if(CFE_MSG_GetFcnCode(MsgPtr, &FcnCode) != CFE_SUCCESS)
        {
            EVSSendErr(SBN_CMD_EID, "unable to get FcnCode (Name=%s)", MsgName);
            return false;
        }

        if (FcnCode == SBN_HK_CC)
        {
            /*
            ** For a bad HK request, just send the event.  We only increment
            ** the error counter for ground commands and not internal messages.
            */
            EVSSendErr(SBN_CMD_EID,
                       "invalid hk message length (Name=%s ID=0x%04X "
                       "CC=%d Len=%d Expected=%d)",
                       MsgName, FcnCode, (int)FcnCode, (int)ActualLen, (int)ExpectedLen);
        }
        else
        {
            /*
            ** All other cases, increment error counter
            */
            EVSSendErr(SBN_CMD_EID, "invalid message length (Name=%s ID=0x%04X CC=%d Len=%d Expected=%d)", MsgName,
                       FcnCode, (int)FcnCode, (int)ActualLen, (int)ExpectedLen);

            SBN.CmdErrCnt++;
        } /* end if */

        return false;
    } /* end if */

    return true;
} /* end VerifyMsgLen */

/************************************************************************/
/** \brief Noop command
**
**  \par Description
**       Processes a noop ground command.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_RESET_CC
**
*************************************************************************/
static void NoopCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, sizeof(CFE_MSG_CommandHeader_t), "noop"))
    {
        EVSSendErr(SBN_CMD_EID, "invalid no-op command");
        return;
    } /* end if */

    EVSSendDbg(SBN_CMD_EID, "no-op command");

    SBN.CmdCnt++;
} /* end NoopCmd */

/************************************************************************/
/** \brief Reset counters command
**
**  \par Description
**       Processes a reset counters command which will reset the
**       following SBN application counters to zero:
**         - Command counter
**         - Command error counter
**         - App messages sent counter for each peer
**         - App message send error counter for each peer
**         - App messages received counter for each peer
**         - App message receive error counter for each peer
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_RESET_CC
**
*************************************************************************/
static void HKResetCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, sizeof(CFE_MSG_CommandHeader_t), "reset counters"))
    {
        return;
    } /* end if */

    EVSSendInfo(SBN_CMD_EID, "reset command");

    /*
    ** Don't increment counter because we're resetting anyway
    */
    SBN_InitializeCounters();
} /* end HKResetCmd */

/************************************************************************/
/** \brief Reset Peer counters command
**
**  \par Description
**       Reset the counters for a specified peer.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_RESET_PEER_CC
**
*************************************************************************/
static void HKResetPeerCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, SBN_CMD_PEER_LEN, "reset peer"))
    {
        return;
    } /* end if */

    uint8 *Ptr     = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8  NetIdx  = *Ptr++;
    uint8  PeerIdx = *Ptr;

    if (NetIdx < 0 || NetIdx >= SBN.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "invalid net idx %d (max=%d)", NetIdx, SBN.NetCnt);
        return;
    } /* end if */

    SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

    if (PeerIdx < 0 || PeerIdx >= Net->PeerCnt)
    {
        EVSSendErr(SBN_CMD_EID, "invalid peer idx %d (max=%d)", PeerIdx, Net->PeerCnt);
        return;
    } /* end if */

    SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

    EVSSendInfo(SBN_CMD_EID, "hk reset peer command (NetIdx=%d, PeerIdx=%d)", NetIdx, PeerIdx);
    SBN.CmdCnt++;

    InitializePeerCounters(Peer);
} /* end HKResetPeerCmd */

/** \brief Housekeeping request command
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 */
static void HKCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, sizeof(CFE_MSG_CommandHeader_t), "hk"))
    {
        return;
    } /* end if */

    EVSSendInfo(SBN_CMD_EID, "hk command");

    uint8  HKBuf[SBN_HK_LEN];
    CFE_MSG_Message_t *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    Pack_t Pack;

    CFE_MSG_Init(HKMsg, SBN_TLM_MID, SBN_HK_LEN);

    Pack_Init(&Pack, HKBuf + sizeof(CFE_MSG_TelemetryHeader_t), SBN_HK_LEN - sizeof(CFE_MSG_TelemetryHeader_t), 1);

    Pack_UInt8(&Pack, SBN_HK_CC);
    Pack_UInt16(&Pack, SBN.CmdCnt);
    Pack_UInt16(&Pack, SBN.CmdErrCnt);
    Pack_UInt16(&Pack, SBN.SubCnt);
    Pack_UInt16(&Pack, SBN.NetCnt);

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);
} /* end HKCmd */

/** \brief Request for housekeeping for one network.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 */
static void HKNetCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, SBN_CMD_NET_LEN, "hk net"))
    {
        return;
    } /* end if */

    uint8 *Ptr    = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8  NetIdx = *Ptr;

    if (NetIdx > SBN.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx (%d, max is %d)", NetIdx, SBN.NetCnt - 1);
        return;
    } /* end if */

    EVSSendInfo(SBN_CMD_EID, "hk command, net=%d", NetIdx);

    uint8  HKBuf[SBN_HKNET_LEN];
    CFE_MSG_Message_t *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    Pack_t Pack;

    CFE_MSG_Init(HKMsg, SBN_TLM_MID, SBN_HKNET_LEN);

    Pack_Init(&Pack, HKBuf + sizeof(CFE_MSG_TelemetryHeader_t), SBN_HKNET_LEN - sizeof(CFE_MSG_TelemetryHeader_t), 1);

    Pack_UInt8(&Pack, SBN_HK_NET_CC);
    Pack_UInt8(&Pack, SBN.Nets[NetIdx].ProtocolIdx);
    Pack_UInt16(&Pack, SBN.Nets[NetIdx].PeerCnt);

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);
} /* end HKNetCmd */

/** \brief Request for housekeeping for one peer.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 */
static void HKPeerCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, SBN_CMD_PEER_LEN, "hk peer"))
    {
        return;
    } /* end if */

    uint8 *Ptr     = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8  NetIdx  = *Ptr++;
    uint8  PeerIdx = *Ptr;

    if (NetIdx > SBN.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx (%d, max is %d)", NetIdx, SBN.NetCnt - 1);
        return;
    } /* end if */

    if (PeerIdx > SBN.Nets[NetIdx].PeerCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)", NetIdx, PeerIdx,
                   SBN.Nets[NetIdx].PeerCnt - 1);
        return;
    } /* end if */

    SBN_PeerInterface_t *Peer = &SBN.Nets[NetIdx].Peers[PeerIdx];

    EVSSendInfo(SBN_CMD_EID, "hk command, net=%d, peer=%d", NetIdx, PeerIdx);

    uint8  HKBuf[SBN_HKPEER_LEN];
    CFE_MSG_Message_t *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    Pack_t Pack;

    CFE_MSG_Init(HKMsg, SBN_TLM_MID, SBN_HKPEER_LEN);

    Pack_Init(&Pack, HKBuf + sizeof(CFE_MSG_TelemetryHeader_t), SBN_HKPEER_LEN - sizeof(CFE_MSG_TelemetryHeader_t), 1);

    Pack_UInt8(&Pack, SBN_HK_PEER_CC);
    Pack_UInt32(&Pack, Peer->ProcessorID);
    Pack_Time(&Pack, Peer->LastSend);
    Pack_Time(&Pack, Peer->LastRecv);
    Pack_UInt16(&Pack, Peer->SendCnt);
    Pack_UInt16(&Pack, Peer->RecvCnt);
    Pack_UInt16(&Pack, Peer->SendErrCnt);
    Pack_UInt16(&Pack, Peer->RecvErrCnt);
    Pack_UInt16(&Pack, Peer->SubCnt);

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);
} /* end HKPeerCmd */

/** \brief Send My Subscriptions
 *
 *  \par Assumptions, External Events, and Notes:
 *       None
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 *  \sa #SBN_MYSUBS_CC
 */
static void MySubsCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, sizeof(CFE_MSG_CommandHeader_t), "my subs"))
    {
        return;
    } /* end if */

    EVSSendInfo(SBN_CMD_EID, "hk subs command");

    uint8  HKBuf[SBN_HKMYSUBS_LEN];
    CFE_MSG_Message_t *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    Pack_t Pack;

    CFE_MSG_Init(HKMsg, SBN_TLM_MID, SBN_HKMYSUBS_LEN);

    Pack_Init(&Pack, HKBuf + sizeof(CFE_MSG_TelemetryHeader_t), SBN_HKMYSUBS_LEN - sizeof(CFE_MSG_TelemetryHeader_t), 1);

    Pack_UInt8(&Pack, SBN_HK_MYSUBS_CC);
    Pack_UInt16(&Pack, SBN.SubCnt);
    int i;
    for (i = 0; i < SBN.SubCnt; i++)
    {
        Pack_MsgID(&Pack, SBN.Subs[i].MsgID);
    }

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);
} /* end MySubsCmd */

/** \brief Reloads the config table.
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 *  \sa #SBN_TBL_CC
 */
static void ReloadTblCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, sizeof(CFE_MSG_CommandHeader_t), "reloadtbl"))
    {
        EVSSendInfo(SBN_CMD_EID, "reload tbl command");
        SBN_ReloadConfTbl();
    } else {
        EVSSendErr(SBN_CMD_EID, "Recevied tbl reload command, but message was wrong size. This command should only be triggered from the TBL service, itself, and not called directly.");
    }
    return;
}

/************************************************************************/
/** \brief Send A Peer's Subscriptions
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr   A #CFE_MSG_Message_t pointer that
**                         references the software bus message
**
**  \sa #SBN_MYSUBS_CC
**
*************************************************************************/
static void PeerSubsCmd(CFE_MSG_Message_t *MsgPtr)
{
    if (!VerifyMsgLen(MsgPtr, SBN_CMD_PEER_LEN, "peer subs"))
    {
        return;
    } /* end if */

    uint8 *Ptr     = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8  NetIdx  = *Ptr++;
    uint8  PeerIdx = *Ptr;

    if (NetIdx >= SBN.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx (%d, max is %d)", NetIdx, SBN.NetCnt - 1);
        return;
    } /* end if */

    if (PeerIdx >= SBN.Nets[NetIdx].PeerCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)", NetIdx, PeerIdx,
                   SBN.Nets[NetIdx].PeerCnt - 1);
        return;
    } /* end if */

    EVSSendInfo(SBN_CMD_EID, "hk subs command, net=%d peer=%d", NetIdx, PeerIdx);

    SBN_PeerInterface_t *Peer = &SBN.Nets[NetIdx].Peers[PeerIdx];

    uint8  HKBuf[SBN_HKPEERSUBS_LEN];
    CFE_MSG_Message_t *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    Pack_t Pack;

    CFE_MSG_Init(HKMsg, SBN_TLM_MID, SBN_HKPEERSUBS_LEN);

    Pack_Init(&Pack, HKBuf + sizeof(CFE_MSG_TelemetryHeader_t), SBN_HKPEERSUBS_LEN - sizeof(CFE_MSG_TelemetryHeader_t), 1);

    Pack_UInt8(&Pack, SBN_HK_PEERSUBS_CC);
    Pack_UInt16(&Pack, NetIdx);
    Pack_UInt16(&Pack, PeerIdx);
    Pack_UInt16(&Pack, Peer->SubCnt);

    int i;
    for (i = 0; i < Peer->SubCnt; i++)
    {
        Pack_MsgID(&Pack, Peer->Subs[i].MsgID);
    }

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);
} /* end PeerSubsCmd */

/*******************************************************************/
/*                                                                 */
/* Process a command pipe message                                  */
/*                                                                 */
/*******************************************************************/
void SBN_HandleCommand(CFE_MSG_Message_t *MsgPtr)
{
    CFE_SB_MsgId_t    MsgId   = 0;
    CFE_MSG_FcnCode_t FcnCode = 0;

    if (CFE_MSG_GetMsgId(MsgPtr, &MsgId) != CFE_SUCCESS)
    {
        SBN.CmdErrCnt++;
        EVSSendErr(SBN_CMD_EID, "invalid FcnCode");
        return;
    }

    if (MsgId != SBN_CMD_MID)
    {
        SBN.CmdErrCnt++;
        EVSSendErr(SBN_CMD_EID, "invalid command pipe MsgId (MsgId=0x%04X)", MsgId);
        return;
    } /* end if */

    if(CFE_MSG_GetFcnCode(MsgPtr, &FcnCode) != CFE_SUCCESS)
    {
        SBN.CmdErrCnt++;
        EVSSendErr(SBN_CMD_EID, "invalid FcnCode (FcnCode=0x%04X)", FcnCode);
        return;
    }

    switch (FcnCode)
    {
        case SBN_NOOP_CC:
            NoopCmd(MsgPtr);
            break;

        case SBN_HK_CC:
            HKCmd(MsgPtr);
            break;
        case SBN_HK_NET_CC:
            HKNetCmd(MsgPtr);
            break;
        case SBN_HK_PEER_CC:
            HKPeerCmd(MsgPtr);
            break;
        case SBN_HK_PEERSUBS_CC:
            PeerSubsCmd(MsgPtr);
            break;
        case SBN_HK_MYSUBS_CC:
            MySubsCmd(MsgPtr);
            break;
        case SBN_HK_RESET_CC:
            HKResetCmd(MsgPtr);
            break;
        case SBN_HK_RESET_PEER_CC:
            HKResetPeerCmd(MsgPtr);
            break;

        case SBN_SCH_WAKEUP_CC:
            EVSSendDbg(SBN_CMD_EID, "wakeup");
            break;
        case SBN_TBL_CC:
            ReloadTblCmd(MsgPtr);
            break;
        default:
            SBN.CmdErrCnt++;
            EVSSendErr(SBN_CMD_EID, "invalid command code (ID=0x%04X, CC=%d)", FcnCode, FcnCode);
            break;
    } /* end switch */
} /* end SBN_HandleCommand */
