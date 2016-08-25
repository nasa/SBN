#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "sbn_msgids.h"
#include "sbn_main_events.h"
#include "sbn_cmds.h"
#include "sbn_app.h"
#include "cfe.h"

/*******************************************************************/
/*                                                                 */
/* Reset telemetry counters                                        */
/*                                                                 */
/*******************************************************************/
void SBN_InitializeCounters(void)
{
    int32 i = 0;

    DEBUG_START();

    SBN.HkPkt.CmdCount = 0;
    SBN.HkPkt.CmdErrCount = 0;

    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        CFE_PSP_MemSet(&SBN.HkPkt.PeerHk[i], 0, sizeof(SBN.HkPkt.PeerHk));
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
    uint16 CommandCode = 0;
    uint16 ActualLength = 0;
    CFE_SB_MsgId_t MsgId = 0;

    DEBUG_START();

    ActualLength = CFE_SB_GetTotalMsgLength(msg);

    if(ExpectedLength != ActualLength)
    {
        MsgId = CFE_SB_GetMsgId(msg);
        CommandCode = CFE_SB_GetCmdCode(msg);

        if(CommandCode == SBN_SEND_HK_CC)
        {
            /*
            ** For a bad HK request, just send the event.  We only increment
            ** the error counter for ground commands and not internal messages.
            */
            CFE_EVS_SendEvent(SBN_HK_EID, CFE_EVS_ERROR,
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

            SBN.HkPkt.CmdErrCount++;
        }/* end if */

        return FALSE;
    }/* end if */

    return TRUE;
}/* end VerifyMsgLength */

/************************************************************************/
/** \brief Housekeeping request command
**
**  \par Description
**       Processes an on-board housekeeping request message.
**
**  \par Assumptions, External Events, and Notes:
**       This message does not affect the command execution counter

**  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
**                             references the software bus message
**
*************************************************************************/
static void HKCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    int32 i = 0;

    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid hk command");
        return;
    }/* end if */

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "hk command");

    /* 
    ** Update with the latest subscription counts 
    */
    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        SBN.HkPkt.PeerHk[i].SubsCount = SBN.Peer[i].SubCnt;
    }/* end if */

    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &SBN.HkPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &SBN.HkPkt);
}/* end HKCmd */

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
    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid no-op command");
        return;
    }/* end if */

    SBN.HkPkt.CmdCount++;

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
    DEBUG_START();

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
/** \brief Get Peer List Command
**
**  \par Description
**       Gets a list of all the peers recognized by the SBN.  The list
**       includes all available identifying information about the peer.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
**                             references the software bus message
**
**  \sa #SBN_GET_PEER_LIST_CC
**  \sa #SBN_PeerSummary_t
**
*************************************************************************/
static void GetPeerListCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    SBN_PeerListResponsePacket_t response;
    int32 i = 0;

    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid get peer list command");
        return;
    }/* end if */

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "get peer list command");

    CFE_SB_InitMsg(&response, SBN_GET_PEER_LIST_RSP_MID,
        sizeof(SBN_PeerListResponsePacket_t), TRUE);
    response.NumPeers = SBN.NumPeers;

    for(i = 0; i < response.NumPeers; i++)
    {
        strncpy(response.PeerList[i].Name, SBN.Peer[i].Name,
            SBN_MAX_PEERNAME_LENGTH);
        response.PeerList[i].ProcessorId = SBN.Peer[i].ProcessorId;
        response.PeerList[i].ProtocolId = SBN.Peer[i].ProtocolId;
        response.PeerList[i].SpaceCraftId = SBN.Peer[i].SpaceCraftId;
        response.PeerList[i].State = SBN.Peer[i].State;
        response.PeerList[i].SubCnt = SBN.Peer[i].SubCnt;
    }/* end for */

    SBN.HkPkt.CmdCount++;
    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &response);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &response);

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_DEBUG,
        "peer list retrieved (%d peers)", (int)response.NumPeers);
}/* end GetPeerListCmd */

/************************************************************************/
/** \brief Get Peer Status Command
**
**  \par Description
**       Get status information on the specified peer.  The interface 
**       module fills up to a maximum number of bytes with status 
**       information in a module-defined format.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MessagePtr   A #CFE_SB_MsgPtr_t pointer that
**                             references the software bus message
**
**  \sa #SBN_GET_PEER_STATUS_CC
**
*************************************************************************/
static void GetPeerStatusCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    SBN_ModuleStatusPacket_t response;
    int32 PeerIdx = 0, Status = 0;
    SBN_PeerData_t *Peer;

    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_GetPeerStatusCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid get peer status command");
        return;
    }/* end if */

    SBN_GetPeerStatusCmd_t *Command = (SBN_GetPeerStatusCmd_t*)MessagePtr;

    PeerIdx = Command->PeerIdx;

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "get peer status command (PeerIdx=%d)", PeerIdx);

    if(PeerIdx < 0 || PeerIdx >= SBN.NumPeers)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid peer");
        return;
    }/* end if */

    Peer = &SBN.Peer[PeerIdx];
    
    SBN.HkPkt.CmdCount++;
    CFE_SB_InitMsg(&response, SBN_GET_PEER_STATUS_RSP_MID,
        sizeof(SBN_ModuleStatusPacket_t), TRUE);
    response.ProtocolId = Peer->ProtocolId;
    Status = SBN.IfOps[Peer->ProtocolId]->ReportModuleStatus(&response,
        Peer->IfData, SBN.Host, SBN.NumHosts);
    
    if(Status == SBN_NOT_IMPLEMENTED)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
            "peer status command not implemented for peer %d of type %d",
            (int)PeerIdx, (int)response.ProtocolId);
        return;
    }/* end if */

    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &response);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &response);

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_DEBUG,
        "peer status retrieved for peer %d", (int)PeerIdx);
}/* end GetPeerStatusCmd */

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
    int PeerIdx = 0, Status = 0;
    SBN_PeerData_t *Peer;
    SBN_ResetPeerCmd_t *Command = NULL;

    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_ResetPeerCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid reset peer command");
    }/* end if */

    Command = (SBN_ResetPeerCmd_t*)MessagePtr;
    PeerIdx = Command->PeerIdx;

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "reset peer command (PeerIdx=%d)", PeerIdx);

    if(PeerIdx < 0 || PeerIdx >= SBN.NumPeers)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid peer");
        return;
    }/* end if */

    SBN.HkPkt.CmdCount++;
    Peer = &SBN.Peer[PeerIdx];
    
    Status = SBN.IfOps[Peer->ProtocolId]->ResetPeer(
        Peer->IfData, SBN.Host, SBN.NumHosts);
    
    if(Status == SBN_NOT_IMPLEMENTED)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID,
            CFE_EVS_INFORMATION,
            "reset peer not implemented for peer %d of type %d",
            PeerIdx, Peer->ProtocolId);
    }
    else
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_DEBUG,
            "reset peer %d", PeerIdx);
    }/* end if */
}/* end ResetPeerCmd */

/*******************************************************************/
/*                                                                 */
/* Process a command pipe message                                  */
/*                                                                 */
/*******************************************************************/
void SBN_HandleCommand(CFE_SB_MsgPtr_t MessagePtr)
{
    CFE_SB_MsgId_t MsgId = 0;
    uint16 CommandCode = 0;

    DEBUG_START();

    if((MsgId = CFE_SB_GetMsgId(MessagePtr)) != SBN_CMD_MID)
    {
        SBN.HkPkt.CmdErrCount++;
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
        case SBN_GET_PEER_LIST_CC:
            GetPeerListCmd(MessagePtr);
            break;
        case SBN_GET_PEER_STATUS_CC:
            GetPeerStatusCmd(MessagePtr);
            break;
        case SBN_RESET_PEER_CC:
            ResetPeerCmd(MessagePtr);
            break;
        case SBN_SEND_HK_CC:
            HKCmd(MessagePtr);
            break;
        default:
            SBN.HkPkt.CmdErrCount++;
            CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
                "invalid command code (ID=0x%04X, CC=%d)",
                MsgId, CommandCode);
            break;
    }/* end switch */
}/* end SBN_HandleCommand */
