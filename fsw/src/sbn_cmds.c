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

    SBN.Hk.CmdCount = 0;
    SBN.Hk.CmdErrCount = 0;

    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        SBN.Hk.PeerStatus[i].SentCount = 0;
        SBN.Hk.PeerStatus[i].RecvCount = 0;
        SBN.Hk.PeerStatus[i].SentErrCount = 0;
        SBN.Hk.PeerStatus[i].RecvErrCount = 0;
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

            SBN.Hk.CmdErrCount++;
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
    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid hk command");
        return;
    }/* end if */

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION, "hk command");

    SBN.Hk.CC = SBN_SEND_HK_CC;

    /* 
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &SBN.Hk);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &SBN.Hk);
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
    int PeerIdx = 0;
    SBN_PeerIdxArgsCmd_t *Command = (SBN_PeerIdxArgsCmd_t *)MessagePtr;

    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_PeerIdxArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid reset peer command");
    }/* end if */

    PeerIdx = Command->PeerIdx;

    CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_INFORMATION,
        "reset peer command (PeerIdx=%d)", PeerIdx);

    if(PeerIdx < 0 || PeerIdx >= SBN.Hk.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR, "invalid peer");
        return;
    }/* end if */

    SBN.Hk.CmdCount++;
    
    if(SBN.IfOps[SBN.Hk.PeerStatus[PeerIdx].ProtocolId]->ResetPeer(
            SBN.Peer[PeerIdx].IfData, SBN.Host, SBN.Hk.HostCount)
        == SBN_NOT_IMPLEMENTED)
    {
        CFE_EVS_SendEvent(SBN_CMD_EID,
            CFE_EVS_INFORMATION,
            "reset peer not implemented for peer %d of type %d",
            PeerIdx, SBN.Hk.PeerStatus[PeerIdx].ProtocolId);
    }
    else
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_DEBUG,
            "reset peer %d", PeerIdx);
    }/* end if */
}/* end ResetPeerCmd */
/************************************************************************/
/** \brief Send My Subscriptions
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
static void MySubsCmd(CFE_SB_MsgPtr_t MessagePtr)
{
    SBN_HkSubsPkt_t Pkt;
    int SubNum = 0;

    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_NoArgsCmd_t)))
    {   
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid my subscriptions command");
    }/* end if */

    CFE_SB_InitMsg(&Pkt, SBN_HK_TLM_MID, sizeof(Pkt), TRUE);
    Pkt.CC = SBN_MYSUBS_CC;
    Pkt.PeerIdx = 0;

    Pkt.SubCount = SBN.Hk.SubCount;
    for(SubNum = 0; SubNum < SBN.Hk.SubCount; SubNum++)
    {
        Pkt.Subs[SubNum] = SBN.LocalSubs[SubNum].MsgId;
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
    SBN_HkSubsPkt_t Pkt;
    int PeerIdx = 0;
    int SubNum = 0;
    SBN_PeerIdxArgsCmd_t *Command = (SBN_PeerIdxArgsCmd_t *)MessagePtr;

    DEBUG_START();

    if(!VerifyMsgLength(MessagePtr, sizeof(SBN_PeerIdxArgsCmd_t)))
    {
        CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
            "invalid reset peer command");
    }/* end if */

    PeerIdx = Command->PeerIdx;

    CFE_SB_InitMsg(&Pkt, SBN_HK_TLM_MID, sizeof(Pkt), TRUE);
    Pkt.CC = SBN_PEERSUBS_CC;
    Pkt.PeerIdx = PeerIdx;

    Pkt.SubCount = SBN.Hk.PeerStatus[PeerIdx].SubCount;
    for(SubNum = 0; SubNum < Pkt.SubCount; SubNum++)
    {
        Pkt.Subs[SubNum] = SBN.Peer[PeerIdx].Subs[SubNum].MsgId;
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
    CFE_SB_MsgId_t MsgId = 0;
    uint16 CommandCode = 0;

    DEBUG_START();

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
        case SBN_SEND_HK_CC:
            HKCmd(MessagePtr);
            break;
        case SBN_MYSUBS_CC:
            MySubsCmd(MessagePtr);
            break;
        case SBN_PEERSUBS_CC:
            PeerSubsCmd(MessagePtr);
            break;
        default:
            SBN.Hk.CmdErrCount++;
            CFE_EVS_SendEvent(SBN_CMD_EID, CFE_EVS_ERROR,
                "invalid command code (ID=0x%04X, CC=%d)",
                MsgId, CommandCode);
            break;
    }/* end switch */
}/* end SBN_HandleCommand */
