#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "sbn_app.h"
#include "sbn_remap.h"

/*******************************************************************/
/*                                                                 */
/* Reset telemetry counters                                        */
/*                                                                 */
/*******************************************************************/
void SBN_InitializeCounters(void)
{
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
**  \param [in]   msg           A #CFE_SB_MsgPtr_t pointer that 
**                              references the software bus message
**
**  \param [in]   ExpectedLen   The expected length of the message
**                              based upon the command code.
**
**  @param [in]   name          Text name of the message expected.
**
**  \returns
**  \retstmt Returns TRUE if the length is as expected      \endcode
**  \retstmt Returns FALSE if the length is not as expected \endcode
**  \endreturns
**
**  \sa #SBN_LEN_EID
**
*************************************************************************/
static boolean VerifyMsgLen(CFE_SB_MsgPtr_t msg, uint16 ExpectedLen,
    const char *MsgName)
{
    uint16 CommandCode = 0;
    uint16 ActualLen = 0;
    CFE_SB_MsgId_t MsgId = 0;

    ActualLen = CFE_SB_GetTotalMsgLength(msg);

    if(ExpectedLen != ActualLen)
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
                "invalid hk message length (Name=%s ID=0x%04X "
                "CC=%d Len=%d Expected=%d)",
                MsgName, MsgId, CommandCode, ActualLen, ExpectedLen);
        }
        else
        {
            /*
            ** All other cases, increment error counter
            */
            CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
                "invalid message length (Name=%s ID=0x%04X "
                "CC=%d Len=%d Expected=%d)",
                MsgName, MsgId, CommandCode, ActualLen, ExpectedLen);

            SBN.Hk.CmdErrCount++;
        }/* end if */

        return FALSE;
    }/* end if */

    return TRUE;
}/* end VerifyMsgLen */

/************************************************************************/
/** \brief Noop command
**  
**  \par Description
**       Processes a noop ground command.
**  
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
**                       references the software bus message
**
**  \sa #SBN_RESET_CC
**
*************************************************************************/
static void NoopCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, CFE_SB_CMD_HDR_SIZE, "noop"))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid no-op command");
        return;
    }/* end if */

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "no-op command");

    SBN.Hk.CmdCount++;
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
**  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
**                       references the software bus message
**
**  \sa #SBN_RESET_CC
**
*************************************************************************/
static void ResetCountersCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, CFE_SB_CMD_HDR_SIZE, "reset counters"))
    {
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
**  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
**                       references the software bus message
**
**  \sa #SBN_RESET_PEER_CC
**
*************************************************************************/
static void ResetPeerCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, SBN_CMD_PEER_LEN, "reset peer"))
    {
        return;
    }/* end if */

    uint8 *Ptr = (uint8 *)MsgPtr + CFE_SB_CMD_HDR_SIZE;
    uint8 NetIdx = *Ptr++;
    uint8 PeerIdx = *Ptr;

    if(NetIdx < 0 || NetIdx >= SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid net idx %d (max=%d)", NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

    if(PeerIdx < 0 || PeerIdx >= Net->Status.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid peer idx %d (max=%d)", PeerIdx, Net->Status.PeerCount);
        return;
    }/* end if */

    SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "reset peer command (NetIdx=%d, PeerIdx=%d)",
        NetIdx, PeerIdx);
    SBN.Hk.CmdCount++;
    
    if(Net->IfOps->ResetPeer(Peer) == SBN_NOT_IMPLEMENTED)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
            "reset peer not implemented for net");
    }/* end if */
}/* end ResetPeerCmd */

/************************************************************************/
/** \brief Configure Remapping behavior.
**
**  \par Description
**       Configure the remapping behavior (enabling/disabling, defining
**       default behavior for when a MID is not in the remap table.)
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
**                       references the software bus message
**
**  \sa #SBN_REMAPCFG_CC
**
*************************************************************************/
static void RemapCfgCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, SBN_CMD_REMAPCFG_LEN, "remap cfg"))
    {
        return;
    }/* end if */

    uint8 *Ptr = (uint8 *)MsgPtr + CFE_SB_CMD_HDR_SIZE;
    SBN.RemapEnabled = *Ptr++;
    SBN.RemapTbl->RemapDefaultFlag = *Ptr;

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "remap cfg (RemapEnabled=%d, RemapDefaultFlag=%d)",
        SBN.RemapEnabled, SBN.RemapTbl->RemapDefaultFlag);
}/* end RemapCfgCmd */

/************************************************************************/
/** \brief Add remapping behavior.
**
**  \par Description
**       Adds a remapping to the remapping table.
**
**  \par Assumptions, External Events, and Notes:
**       Will fail if there is insufficient space in the table.
**
**  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
**                       references the software bus message
**
**  \sa #SBN_REMAPADD_CC
**
*************************************************************************/
static void RemapAddCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, SBN_CMD_REMAPADD_LEN, "remap add"))
    {
        return;
    }/* end if */

    uint8 *Ptr = (uint8 *)MsgPtr + CFE_SB_CMD_HDR_SIZE;
    uint32 ProcessorID = *((uint32 *)Ptr); Ptr += sizeof(ProcessorID);
    CFE_SB_MsgId_t FromMID = *((CFE_SB_MsgId_t *)Ptr); Ptr += sizeof(FromMID);
    CFE_SB_MsgId_t ToMID = *((CFE_SB_MsgId_t *)Ptr);

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "remap add (ProcessorID=%d, FromMID=0x%04x, ToMID=0x%04x)",
        ProcessorID, FromMID, ToMID);

    SBN_RemapAdd(ProcessorID, FromMID, ToMID);
}/* end RemapAddCmd */

/************************************************************************/
/** \brief Delete remapping behavior.
**
**  \par Description
**       Deletes a remapping from the remapping table.
**
**  \par Assumptions, External Events, and Notes:
**       Will do nothing if the entry does not exist in the table.
**
**  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
**                       references the software bus message
**
**  \sa #SBN_REMAPDEL_CC
**
*************************************************************************/
static void RemapDelCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, SBN_CMD_REMAPDEL_LEN, "remap del"))
    {
        return;
    }/* end if */

    uint8 *Ptr = (uint8 *)MsgPtr + CFE_SB_CMD_HDR_SIZE;
    uint32 ProcessorID = *((uint32 *)Ptr); Ptr += sizeof(ProcessorID);
    CFE_SB_MsgId_t FromMID = *((CFE_SB_MsgId_t *)Ptr);

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "remap del (ProcessorID=%d, FromMID=0x%04x)",
        ProcessorID, FromMID);

    SBN_RemapDel(ProcessorID, FromMID);
}/* end RemapDelCmd */

/** \brief Housekeeping request command
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
 *                       references the software bus message
 *
 */
static void HKCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, CFE_SB_CMD_HDR_SIZE, "hk"))
    {
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
 *  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
 *                       references the software bus message
 */
static void HKNetCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, SBN_CMD_NET_LEN, "hk net"))
    {
        return;
    }/* end if */

    uint8 *Ptr = (uint8 *)MsgPtr + CFE_SB_CMD_HDR_SIZE;
    uint8 NetIdx = *Ptr;

    if(NetIdx > SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid NetIdx (%d, max is %d)", NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    SBN_NetStatus_t *NetStatus = &(SBN.Nets[NetIdx].Status);
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
 *  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
 *                       references the software bus message
 */
static void HKPeerCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    if(!VerifyMsgLen(MsgPtr, SBN_CMD_PEER_LEN, "hk peer"))
    {
        return;
    }/* end if */

    uint8 *Ptr = (uint8 *)MsgPtr + CFE_SB_CMD_HDR_SIZE;
    uint8 NetIdx = *Ptr++;
    uint8 PeerIdx = *Ptr;

    if(NetIdx > SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid NetIdx (%d, max is %d)", NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    if(PeerIdx > SBN.Nets[NetIdx].Status.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)",
            NetIdx, PeerIdx, SBN.Nets[NetIdx].Status.PeerCount);
        return;
    }/* end if */

    SBN_PeerStatus_t *PeerStatus = &(SBN.Nets[NetIdx].Peers[PeerIdx].Status);
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
 *  \param [in]   MsgPtr A #CFE_SB_MsgPtr_t pointer that
 *                       references the software bus message
 *
 *  \sa #SBN_MYSUBS_CC
 */
static void MySubsCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    SBN_HkSubsPkt_t Pkt;
    int SubNum = 0;

    if(!VerifyMsgLen(MsgPtr, CFE_SB_CMD_HDR_SIZE, "my subs"))
    {   
        return;
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
**  \param [in]   MsgPtr   A #CFE_SB_MsgPtr_t pointer that
**                         references the software bus message
**
**  \sa #SBN_MYSUBS_CC
**
*************************************************************************/
static void PeerSubsCmd(CFE_SB_MsgPtr_t MsgPtr)
{
    int SubNum = 0;
    if(!VerifyMsgLen(MsgPtr, SBN_CMD_PEER_LEN, "peer subs"))
    {
        return;
    }/* end if */

    uint8 *Ptr = (uint8 *)MsgPtr + CFE_SB_CMD_HDR_SIZE;
    uint8 NetIdx = *Ptr++;
    uint8 PeerIdx = *Ptr;

    if(NetIdx >= SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid NetIdx (%d, max is %d)",
            NetIdx, SBN.Hk.NetCount);
        return;
    }/* end if */

    if(PeerIdx >= SBN.Nets[NetIdx].Status.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)",
            NetIdx, PeerIdx,
            SBN.Nets[NetIdx].Status.PeerCount);
        return;
    }/* end if */

    SBN_HkSubsPkt_t Pkt;

    CFE_SB_InitMsg(&Pkt, SBN_TLM_MID, sizeof(Pkt), TRUE);
    Pkt.CC = SBN_HK_PEERSUBS_CC;
    Pkt.NetIdx = NetIdx;
    Pkt.PeerIdx = PeerIdx;

    SBN_PeerInterface_t *Peer =
        &(SBN.Nets[NetIdx].Peers[PeerIdx]);

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
void SBN_HandleCommand(CFE_SB_MsgPtr_t MsgPtr)
{
    CFE_SB_MsgId_t MsgId = 0;
    uint16 CommandCode = 0;

    if((MsgId = CFE_SB_GetMsgId(MsgPtr)) != SBN_CMD_MID)
    {
        SBN.Hk.CmdErrCount++;
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid command pipe message ID (ID=0x%04X)",
            MsgId);
        return;
    }/* end if */

    switch((CommandCode = CFE_SB_GetCmdCode(MsgPtr)))
    {
        case SBN_NOOP_CC:
            NoopCmd(MsgPtr);
            break;
        case SBN_RESET_CC:
            ResetCountersCmd(MsgPtr);
            break;
        case SBN_RESET_PEER_CC:
            ResetPeerCmd(MsgPtr);
            break;

        case SBN_REMAPCFG_CC:
            RemapCfgCmd(MsgPtr);
            break;
        case SBN_REMAPADD_CC:
            RemapAddCmd(MsgPtr);
            break;
        case SBN_REMAPDEL_CC:
            RemapDelCmd(MsgPtr);
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
