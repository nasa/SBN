#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "sbn_app.h"

/*******************************************************************/
/*                                                                 */
/* Reset telemetry counters                                        */
/*                                                                 */
/*******************************************************************/
void SBN_InitializeCounters(void)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    SBN.Hk.CmdCount = 0;
    SBN.Hk.CmdErrCount = 0;

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        int PeerIdx = 0;
        for(PeerIdx = 0; PeerIdx < Net->Status.PeerCount; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];
            Peer->Status.SendCount = 0;
            Peer->Status.RecvCount = 0;
            Peer->Status.SendErrCount = 0;
            Peer->Status.RecvErrCount = 0;
        }/* end for */
    }/* end for */
}/* end SBN_InitializeCounters */

/************************************************************************/
/** \brief Verify message length 
**
**  \par Description
**       Checks if the actual length of a software bus message matches
**       the expected length and sends an error event if a mismatch
**       occurs.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   msg              A #CFE_SB_MsgPtr_t pointer that 
**                                 references the software bus message
**
**  \param [in]   ExpectedLength   The expected length of the message
**                                 based upon the command code.
**
**  \returns
**  \retstmt Returns TRUE if the length is as expected      \endcode
**  \retstmt Returns FALSE if the length is not as expected \endcode
**  \endreturns
**
**  \sa #SBN_LEN_EID
**
*************************************************************************/
static boolean VerifyMsgLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    uint16 CommandCode = 0;
    uint16 ActualLength = 0;
    CFE_SB_MsgId_t MsgId = 0;

    ActualLength = CFE_SB_GetTotalMsgLength(msg);

    if(ExpectedLength != ActualLength)
    {
        MsgId = CFE_SB_GetMsgId(msg);
        CommandCode = CFE_SB_GetCmdCode(msg);

        if(CommandCode == SBN_HK_CC)
        {
            /*
            ** For a bad HK request, just send the event.  We only increment
            ** the error counter for ground commands and not internal messages.
            */
            CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
                "invalid housekeeping request message length (ID=0x%04X "
                "CC=%d Len=%d Expected=%d)",
                MsgId, CommandCode, ActualLength, ExpectedLength);
        }
        else
        {
            /*
            ** All other cases, increment error counter
            */
            CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
                "invalid message length (ID=0x%04X "
                "CC=%d Len=%d Expected=%d)",
                MsgId, CommandCode, ActualLength, ExpectedLength);

            SBN.Hk.CmdErrCount++;
        }/* end if */

        return FALSE;
    }/* end if */

    return TRUE;
}/* end VerifyMsgLength */

/************************************************************************/
/** \brief Noop command
**  
**  \par Description
**       Processes a noop ground command.
**  
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
**                             references the software bus message
**
**  \sa #SBN_RESET_CC
**
*************************************************************************/
static void NoopCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid no-op command");
        return;
    }/* end if */

    SBN.Hk.CmdCount++;

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "no-op command");
}/* end NoopCmd */

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
**  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
**                             references the software bus message
**
**  \sa #SBN_RESET_CC
**
*************************************************************************/
static void ResetCountersCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid reset command");
        return;
    }/* end if */

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "reset command");

    /*
    ** Don't increment counter because we're resetting anyway
    */
    SBN_InitializeCounters();
}/* end ResetCountersCmd */

/************************************************************************/
/** \brief Reset Peer
**
**  \par Description
**       Reset a specified peer.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
**                             references the software bus message
**
**  \sa #SBN_RESET_PEER_CC
**
*************************************************************************/
static void ResetPeerCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    SBN_PeerCmd_t *Command = (SBN_PeerCmd_t *)MessagePtr;
    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_PeerCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid reset peer command");
        return;
    }/* end if */

    if(Command->NetIdx < 0 || Command->NetIdx >= SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid net idx %d (max=%d)", Command->NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    SBN_NetInterface_t *Net = &SBN.Nets[Command->NetIdx];

    if(Command->PeerIdx < 0 || Command->PeerIdx >= Net->Status.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid peer idx %d (max=%d)", Command->PeerIdx,
            Net->Status.PeerCount);
        return;
    }/* end if */

    SBN_PeerInterface_t *Peer = &Net->Peers[Command->PeerIdx];

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "reset peer command (NetIdx=%d, PeerIdx=%d)",
        Command->NetIdx, Command->PeerIdx);
    SBN.Hk.CmdCount++;
    
    if(Net->IfOps->ResetPeer(Peer) == SBN_NOT_IMPLEMENTED)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
            "reset peer not implemented for net");
    }/* end if */
}/* end ResetPeerCmd */

/** \brief Housekeeping request command
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
 *                             references the software bus message
 *
 */
static void HKCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid hk command");
        return;
    }/* end if */

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "hk command");

    SBN.Hk.CC = SBN_HK_CC;

    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &SBN.Hk);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &SBN.Hk);
}/* end HKCmd */

/** \brief Request for housekeeping for one network.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
 *                             references the software bus message
 */
static void HKNetCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NetCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid hk command");
        return;
    }/* end if */

    SBN_NetCmd_t *NetCmd = (SBN_NetCmd_t *)MessagePtr;

    if(NetCmd->NetIdx > SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid NetIdx (%d, max is %d)",
            NetCmd->NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    SBN_NetStatus_t *NetStatus = &(SBN.Nets[NetCmd->NetIdx].Status);
    NetStatus->CC = SBN_HK_NET_CC;

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "hk net command");

    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *)NetStatus);
    CFE_SB_SendMsg((CFE_SB_Msg_t *)NetStatus);
}/* end HKNetCmd */

/** \brief Request for housekeeping for one peer.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
 *                             references the software bus message
 */
static void HKPeerCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_PeerCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid hk command");
        return;
    }/* end if */

    SBN_PeerCmd_t *PeerCmd = (SBN_PeerCmd_t *)MessagePtr;

    if(PeerCmd->NetIdx > SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid NetIdx (%d, max is %d)",
            PeerCmd->NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    if(PeerCmd->PeerIdx > SBN.Nets[PeerCmd->NetIdx].Status.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)",
            PeerCmd->NetIdx, PeerCmd->PeerIdx,
            SBN.Nets[PeerCmd->NetIdx].Status.PeerCount);
        return;
    }/* end if */

    SBN_PeerStatus_t *PeerStatus =
        &(SBN.Nets[PeerCmd->NetIdx].Peers[PeerCmd->PeerIdx].Status);
    PeerStatus->CC = SBN_HK_PEER_CC;

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "hk peer command");

    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *)PeerStatus);
    CFE_SB_SendMsg((CFE_SB_Msg_t *)PeerStatus);
}/* end HKCmd */

/** \brief Send My Subscriptions
 *
 *  \par Assumptions, External Events, and Notes:
 *       None
 *
 *  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
 *                             references the software bus message
 *
 *  \sa #SBN_MYSUBS_CC
 */
static void MySubsCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    SBN_HkSubsPkt_t Pkt;
    int SubNum = 0;

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {   
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid my subscriptions command");
    }/* end if */

    CFE_SB_InitMsg(&Pkt, SBN_TLM_MID, sizeof(Pkt), TRUE);
    Pkt.CC = SBN_HK_MYSUBS_CC;
    Pkt.PeerIdx = 0;

    Pkt.SubCount = SBN.Hk.SubCount;
    for(SubNum = 0; SubNum < SBN.Hk.SubCount; SubNum++)
    {
        Pkt.Subs[SubNum] = SBN.LocalSubs[SubNum].MsgID;
    }/* end for */

    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &Pkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &Pkt);
}/* end MySubsCmd */

/************************************************************************/
/** \brief Send A Peer's Subscriptions
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
**                             references the software bus message
**
**  \sa #SBN_MYSUBS_CC
**
*************************************************************************/
static void PeerSubsCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    int SubNum = 0;
    SBN_PeerCmd_t *Command = (SBN_PeerCmd_t *)MessagePtr;

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_PeerCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid reset peer command");
    }/* end if */

    SBN_PeerCmd_t *PeerCmd = (SBN_PeerCmd_t *)MessagePtr;

    if(PeerCmd->NetIdx >= SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid NetIdx (%d, max is %d)",
            PeerCmd->NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    if(PeerCmd->PeerIdx >= SBN.Nets[PeerCmd->NetIdx].Status.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)",
            PeerCmd->NetIdx, PeerCmd->PeerIdx,
            SBN.Nets[PeerCmd->NetIdx].Status.PeerCount);
        return;
    }/* end if */

    SBN_HkSubsPkt_t Pkt;

    CFE_SB_InitMsg(&Pkt, SBN_TLM_MID, sizeof(Pkt), TRUE);
    Pkt.CC = SBN_HK_PEERSUBS_CC;
    Pkt.NetIdx = Command->NetIdx;
    Pkt.PeerIdx = Command->PeerIdx;

    SBN_PeerInterface_t *Peer =
        &(SBN.Nets[PeerCmd->NetIdx].Peers[PeerCmd->PeerIdx]);

    Pkt.SubCount = Peer->Status.SubCount;
    for(SubNum = 0; SubNum < Pkt.SubCount; SubNum++)
    {
        Pkt.Subs[SubNum] = Peer->Subs[SubNum].MsgID;
    }/* end for */

    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &Pkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &Pkt);
}/* end PeerSubsCmd */

/*******************************************************************/
/*                                                                 */
/* Process a command pipe message                                  */
/*                                                                 */
/*******************************************************************/
void SBN_HandleCommand(CFE_SB_MsgPtr_t MessagePtr)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    CFE_SB_MsgId_t MsgId = 0;
    uint16 CommandCode = 0;

    if((MsgId = CFE_SB_GetMsgId(MessagePtr)) != SBN_CMD_MID)
    {
        SBN.Hk.CmdErrCount++;
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid command pipe message ID (ID=0x%04X)",
            MsgId);
        return;
    }/* end if */

    switch((CommandCode = CFE_SB_GetCmdCode(MessagePtr)))
    {
        case SBN_NOOP_CC:
            NoopCmd(MessagePtr);
            break;
        case SBN_RESET_CC:
            ResetCountersCmd(MessagePtr);
            break;
        case SBN_RESET_PEER_CC:
            ResetPeerCmd(MessagePtr);
            break;

        case SBN_HK_CC:
            HKCmd(MessagePtr);
            break;
        case SBN_HK_NET_CC:
            HKNetCmd(MessagePtr);
            break;
        case SBN_HK_PEER_CC:
            HKPeerCmd(MessagePtr);
            break;
        case SBN_HK_PEERSUBS_CC:
            PeerSubsCmd(MessagePtr);
            break;
        case SBN_HK_MYSUBS_CC:
            MySubsCmd(MessagePtr);
            break;

        case SBN_SCH_WAKEUP_CC:
            /* TODO: debug message? */
            break;
        default:
            SBN.Hk.CmdErrCount++;
            CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
                "invalid command code (ID=0x%04X, CC=%d)",
                MsgId, CommandCode);
            break;
    }/* end switch */
}/* end SBN_HandleCommand */
