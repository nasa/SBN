/******************************************************************************
 ** \file sbn_subs.c
 **
 **      Copyright (c) 2004-2006, United States government as represented by the
 **      administrator of the National Aeronautics Space Administration.
 **      All rights reserved. This software(cFE) was created at NASA's Goddard
 **      Space Flight Center pursuant to government contracts.
 **
 **      This software may be used only pursuant to a United States government
 **      sponsored project and the United States government may not be charged
 **      for use thereof.
 **
 ** Purpose:
 **      This file contains source code for the Software Bus Network Application.
 **
 ** Authors:   J. Wilmot/GSFC Code582
 **            R. McGraw/SSI
 **            E. Timmons/GSFC Code587
 **            C. Knight/ARC Code TI
 */

#include <string.h>
#include "sbn_subs.h"
#include "sbn_main_events.h"
#include <arpa/inet.h>

// TODO: instead of using void * for the buffer for SBN messages, use
// a struct that has the SBN header in packed bytes.

/**
 * Informs the software bus to send this application all subscription requests.
 */
void SBN_SendSubsRequests(void)
{
    CFE_SB_CmdHdr_t     SBCmdMsg;

    DEBUG_START();

    /* Turn on SB subscription reporting */
    CFE_SB_InitMsg(&SBCmdMsg.Pri, CFE_SB_CMD_MID, sizeof(CFE_SB_CmdHdr_t),
        TRUE);
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) &SBCmdMsg,
        CFE_SB_ENABLE_SUB_REPORTING_CC);
    CFE_SB_SendMsg((CFE_SB_MsgPtr_t) &SBCmdMsg);

    /* Request a list of previous subscriptions from SB */
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) &SBCmdMsg, CFE_SB_SEND_PREV_SUBS_CC);
    CFE_SB_SendMsg((CFE_SB_MsgPtr_t) &SBCmdMsg);
}/* end SBN_SendSubsRequests */

/**
 * \brief Utility function to pack SBN protocol subscription information.
 *
 * @param[out] SubMsg Destination SBN msg buffer for subscription information.
 * @param[in] MsgId The CCSDS message ID for the subscription.
 * @param[in] Qos The CCSDS quality of service for the subscription.
 */
static void PackSub(void *SubMsg, CFE_SB_MsgId_t MsgId, CFE_SB_Qos_t Qos)
{
    MsgId = CFE_MAKE_BIG16(MsgId);
    memcpy(SubMsg, &MsgId, sizeof(MsgId));
}

/**
 * \brief Utility function to unpack SBN protocol subscription information.
 *
 * @param[in] SubMsg Source SBN msg buffer for subscription information.
 * @param[out] MsgIdPtr A pointer to the CCSDS message ID for the subscription.
 * @param[out] QosPtr A pointer to CCSDS quality of service for the
 *             subscription.
 */
static void UnPackSub(void *SubMsg, CFE_SB_MsgId_t *MsgIdPtr,
    CFE_SB_Qos_t *QosPtr)
{
    memcpy(MsgIdPtr, SubMsg, sizeof(*MsgIdPtr));
    *MsgIdPtr = CFE_MAKE_BIG16(*MsgIdPtr);
    memcpy(QosPtr, SubMsg + sizeof(*MsgIdPtr), sizeof(*QosPtr));
}

/**
 * \brief Sends a local subscription over the wire to a peer.
 *
 * @param[in] SubFlag Whether this is a subscription or unsubscription.
 * @param[in] MsgId The CCSDS message ID being (un)subscribed.
 * @param[in] Qos The CCSDS quality of service being (un)subscribed.
 * @param[in] PeerIdx The index in SBN.Peer of the peer.
 */
static void SendLocalSubToPeer(int SubFlag, CFE_SB_MsgId_t MsgId,
    CFE_SB_Qos_t Qos, int PeerIdx)
{
    SBN_PackedSub_t SubMsg;
    memset(&SubMsg, 0, sizeof(SubMsg));
    PackSub(&SubMsg, MsgId, Qos);
    SBN_SendNetMsg(SubFlag, sizeof(SubMsg), (SBN_Payload_t *)&SubMsg, PeerIdx);
}/* end SendLocalSubToPeer */

/**
 * \brief Sends all local subscriptions over the wire to a peer.
 *
 * @param[in] PeerIdx The index in SBN.Peer of the peer.
 */
void SBN_SendLocalSubsToPeer(int PeerIdx)
{
    int i = 0;

    DEBUG_START();

    for(i = 0; i < SBN.Hk.SubCount; i++)
    {
        SendLocalSubToPeer(SBN_SUBSCRIBE_MSG, SBN.LocalSubs[i].MsgId,
            SBN.LocalSubs[i].Qos, PeerIdx);
    }/* end for */
}/* end SBN_SendLocalSubsToPeer */

/**
 * Utility to find the subscription index (SBN.LocalSubs)
 * that is subscribed to the CCSDS message ID.
 *
 * @param[out] IdxPtr The subscription index found.
 * @param[in] MsgId The CCSDS message ID of the subscription being sought.
 * @return TRUE if found.
 */
static int IsMsgIdSub(int *IdxPtr, CFE_SB_MsgId_t MsgId)
{
    int     i = 0;

    DEBUG_START();

    for(i = 0; i < SBN.Hk.SubCount; i++)
    {
        if(SBN.LocalSubs[i].MsgId == MsgId)
        {
            if (IdxPtr)
            {
                *IdxPtr = i;
            }/* end if */

            return TRUE;
        }/* end if */
    }/* end for */

    return FALSE;
}/* end IsMsgIdSub */

/**
 * \brief Is this peer subscribed to this message ID? If so, what is the index
 *        of the subscription?
 *
 * @param[out] SubIdxPtr The pointer to the subscription index value.
 * @param[in] MsgId The CCSDS message ID of the subscription being sought.
 * @param[in] PeerIdx The peer whose subscription is being sought.
 *
 * @return TRUE if found.
 */
static int IsPeerSubMsgId(int *SubIdxPtr, CFE_SB_MsgId_t MsgId,
    int PeerIdx)
{
    int     i = 0;

    DEBUG_START();

    for(i = 0; i < SBN.Hk.PeerStatus[PeerIdx].SubCount; i++)
    {
        if(SBN.Peers[PeerIdx].Subs[i].MsgId == MsgId)
        {
            *SubIdxPtr = i;
            return TRUE;
        }/* end if */
    }/* end for */

    return FALSE;

}/* end IsPeerSubMsgId */

/**
 * \brief I have seen a local subscription, send it on to peers if this is the
 * first instance of a subscription for this message ID.
 *
 * @param[in] MsgId The CCSDS Message ID of the local subscription.
 * @param[in] Qos The CCSDS quality of service of the local subscription.
 */
static void ProcessLocalSub(CFE_SB_MsgId_t MsgId, CFE_SB_Qos_t Qos)
{
    int SubIdx = 0, PeerIdx = 0;

    DEBUG_START();

    /* don't subscribe to event messages */
    if(MsgId == CFE_EVS_EVENT_MSG_MID) return;

    if(SBN.Hk.SubCount >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                "local subscription ignored for MsgId 0x%04X, max (%d) met",
                ntohs(MsgId), SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if(IsMsgIdSub(&SubIdx, MsgId))
    {
        SBN.LocalSubs[SubIdx].InUseCtr++;
        /* does not send to peers, as they already know */
        return;
    }/* end if */

    /* log new entry into LocalSubs array */
    SBN.LocalSubs[SBN.Hk.SubCount].InUseCtr = 1;
    SBN.LocalSubs[SBN.Hk.SubCount].MsgId = MsgId;
    SBN.LocalSubs[SBN.Hk.SubCount].Qos = Qos;
    SBN.Hk.SubCount++;

    for(PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {
        if(SBN.Hk.PeerStatus[PeerIdx].State != SBN_HEARTBEATING)
        {
            continue;
        }/* end if */
        SendLocalSubToPeer(SBN_SUBSCRIBE_MSG, MsgId, Qos, PeerIdx);
    }/* end for */
}/* end ProcessLocalSub */

/**
 * \brief I have seen a local unsubscription, send it on to peers if this is the
 * last instance of a subscription for this message ID.
 *
 * @param[in] MsgId The CCSDS Message ID of the local unsubscription.
 * @param[in] Qos The CCSDS quality of service of the local unsubscription.
 */
static void ProcessLocalUnsub(CFE_SB_MsgId_t MsgId)
{
    int SubIdx = 0, PeerIdx = 0, i = 0;

    DEBUG_START();

    /* find idx of matching subscription */
    if(!IsMsgIdSub(&SubIdx, MsgId))
    {
        return;
    }/* end if */

    SBN.LocalSubs[SubIdx].InUseCtr--;

    /* do not modify the array and tell peers
    ** until the # of local subscriptions = 0
    */
    if(SBN.LocalSubs[SubIdx].InUseCtr > 0)
    {
        return;
    }/* end if */

    /* remove sub from array for and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the LocalSubs[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for(i = SubIdx; i < SBN.Hk.SubCount; i++)
    {
        memcpy(&SBN.LocalSubs[i], &SBN.LocalSubs[i + 1],
            sizeof(SBN_Subs_t));
    }/* end for */

    SBN.Hk.SubCount--;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    for(PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {
        if(SBN.Hk.PeerStatus[PeerIdx].State != SBN_HEARTBEATING)
        {
            continue;
        }/* end if */
        SendLocalSubToPeer(SBN_UN_SUBSCRIBE_MSG, SBN.LocalSubs[PeerIdx].MsgId,
            SBN.LocalSubs[PeerIdx].Qos, PeerIdx);
    }/* end for */
}/* end ProcessLocalUnsub */

/**
 * \brief Check the local pipe for subscription messages. Send them on to peers
 *        if there are any (new)subscriptions.
 * @return TRUE if subscriptions received.
 */
int32 SBN_CheckSubscriptionPipe(void)
{
    int32 RecvSub = FALSE;
    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_SubRprtMsg_t *SubRprtMsgPtr;

    /* DEBUG_START(); chatty */

    while(CFE_SB_RcvMsg(&SBMsgPtr, SBN.SubPipe, CFE_SB_POLL) == CFE_SUCCESS)
    {
        SubRprtMsgPtr = (CFE_SB_SubRprtMsg_t *)SBMsgPtr;

        switch(CFE_SB_GetMsgId(SBMsgPtr))
        {
            case CFE_SB_ONESUB_TLM_MID:
#ifdef SBN_PAYLOAD
                switch(SubRprtMsgPtr->Payload.SubType)
                {
                    case CFE_SB_SUBSCRIPTION:
                        ProcessLocalSub(SubRprtMsgPtr->Payload.MsgId,
                            SubRprtMsgPtr->Payload.Qos);
                        RecvSub = TRUE;
                        break;
                    case CFE_SB_UNSUBSCRIPTION:
                        ProcessLocalUnsub(SubRprtMsgPtr->Payload.MsgId);
                        RecvSub = TRUE;
                        break;
                    default:
                        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                            "unexpected subscription type (%d) in "
                            "SBN_CheckSubscriptionPipe",
                            SubRprtMsgPtr->Payload.SubType);
                }/* end switch */
                break;
#else /* !SBN_PAYLOAD */
                switch(SubRprtMsgPtr->SubType)
                {
                    case CFE_SB_SUBSCRIPTION:
                        ProcessLocalSub(SubRprtMsgPtr->MsgId,
                            SubRprtMsgPtr->Qos);
                        RecvSub = TRUE;
                        break;
                    case CFE_SB_UNSUBSCRIPTION:
                        ProcessLocalUnsub(SubRprtMsgPtr->MsgId);
                        RecvSub = TRUE;
                        break;
                    default:
                        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                            "unexpected subscription type (%d) in "
                            "SBN_CheckSubscriptionPipe",
                            SubRprtMsgPtr->SubType);
                }/* end switch */
                break;
#endif /* SBN_PAYLOAD */

            case CFE_SB_ALLSUBS_TLM_MID:
                SBN_ProcessAllSubscriptions((CFE_SB_PrevSubMsg_t *) SBMsgPtr);
                RecvSub = TRUE;
                break;

            default:
                CFE_EVS_SendEvent(SBN_MSG_EID, CFE_EVS_ERROR,
                        "unexpected message id (0x%04X) on SBN.SubPipe",
                        ntohs(CFE_SB_GetMsgId(SBMsgPtr)));
        }/* end switch */
    }/* end while */

    return RecvSub;
}/* end SBN_CheckSubscriptionPipe */

static void ProcessSubFromPeer(int PeerIdx, CFE_SB_MsgId_t MsgId,
    CFE_SB_Qos_t Qos)
{   
    int idx = 0, FirstOpenSlot = SBN.Hk.PeerStatus[PeerIdx].SubCount;
    uint32 Status = CFE_SUCCESS;
    
    /* if msg id already in the list, ignore */
    if(IsPeerSubMsgId(&idx, MsgId, PeerIdx))
    {   
        return;
    }/* end if */

    if(SBN.Hk.PeerStatus[PeerIdx].SubCount >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "cannot process subscription from '%s', max (%d) met",
            SBN.Hk.PeerStatus[PeerIdx].Name, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */
    
    /* SubscribeLocal suppresses the subscription report */
    Status = CFE_SB_SubscribeLocal(MsgId, SBN.Peers[PeerIdx].Pipe,
            SBN_DEFAULT_MSG_LIM);
    if(Status != CFE_SUCCESS)
    {   
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "Unable to subscribe to MID 0x%04X", htons(MsgId));
        return;
    }/* end if */
    
    /* log the subscription in the peer table */ 
    SBN.Peers[PeerIdx].Subs[FirstOpenSlot].MsgId = MsgId;
    SBN.Peers[PeerIdx].Subs[FirstOpenSlot].Qos = Qos;
    
    SBN.Hk.PeerStatus[PeerIdx].SubCount++;
}/* end ProcessSubFromPeer */

/**
 * \brief Process a subscription message from a peer.
 *
 * @param[in] PeerIdx The peer index (in SBN.Peer)
 * @param[in] Msg The subscription SBN message.
 */
void SBN_ProcessSubFromPeer(int PeerIdx, void *Msg)
{
    int idx = 0, RemappedFlag = 0;
    CFE_SB_MsgId_t MsgId;
    CFE_SB_Qos_t Qos;

    DEBUG_START();

    if(PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "cannot process subscription, peer index (%d) out of range",
             PeerIdx);
        return;
    }/* end if */

    UnPackSub(Msg, &MsgId, &Qos);

    /** If there's a filter, ignore the sub request.  */
    for(idx = 0; SBN.RemapTable && idx < SBN.RemapTable->Entries; idx++)
    {
        if(SBN.RemapTable->Entry[idx].ProcessorId
                == SBN.Hk.PeerStatus[PeerIdx].ProcessorId
            && SBN.RemapTable->Entry[idx].from == MsgId
            && SBN.RemapTable->Entry[idx].to == 0x0000)
        {
            return;
        }/* end if */
    }/* end for */

    /*****
     * If there's a remap that would generate that
     * MID, I need to subscribe locally to the "from". Usually this will
     * only be one "from" but it's possible that multiple "from"s map to
     * the same "to".
     */
    for(idx = 0; SBN.RemapTable && idx < SBN.RemapTable->Entries; idx++)
    {
        if(SBN.RemapTable->Entry[idx].ProcessorId
                == SBN.Hk.PeerStatus[PeerIdx].ProcessorId
            && SBN.RemapTable->Entry[idx].to == MsgId)
        {
            ProcessSubFromPeer(PeerIdx, SBN.RemapTable->Entry[idx].from, Qos);
            RemappedFlag = 1;
        }/* end if */
    }/* end for */

    /* if there's no remap ID's, subscribe to the original MID. */
    if(!RemappedFlag)
    {
        ProcessSubFromPeer(PeerIdx, MsgId, Qos);
    }/* end if */
}/* SBN_ProcessSubFromPeer */

static void ProcessUnsubFromPeer(int PeerIdx, CFE_SB_MsgId_t MsgId)
{
    int i = 0, idx = 0;
    uint32 Status = CFE_SUCCESS;

    if(IsPeerSubMsgId(&idx, MsgId, PeerIdx))
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_INFORMATION,
            "%s:Cannot process unsubscription from %s,msg 0x%04X not found",
            CFE_CPU_NAME, SBN.Hk.PeerStatus[PeerIdx].Name, htons(MsgId));
        return;
    }/* end if */

    /* remove sub from array for that peer and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the Subs[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for(i = idx; i < SBN.Hk.PeerStatus[PeerIdx].SubCount; i++)
    {
        memcpy(&SBN.Peers[PeerIdx].Subs[i],
            &SBN.Peers[PeerIdx].Subs[i + 1],
            sizeof(SBN_Subs_t));
    }/* end for */

    /* decrement sub cnt */
    SBN.Hk.PeerStatus[PeerIdx].SubCount--;

    /* unsubscribe to the msg id on the peer pipe */
    Status = CFE_SB_UnsubscribeLocal(MsgId, SBN.Peers[PeerIdx].Pipe);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "Unable to unsubscribe from MID 0x%04X", htons(MsgId));
        return;
    }/* end if */
}/* end ProcessUnsubFromPeer */

/**
 * \brief Process an unsubscription message from a peer.
 *
 * @param[in] PeerIdx The peer index (in SBN.Peer)
 * @param[in] Msg The unsubscription SBN message.
 */
void SBN_ProcessUnsubFromPeer(int PeerIdx, void *Msg)
{
    int idx = 0, RemappedFlag = 0;
    CFE_SB_MsgId_t MsgId = 0x0000;
    CFE_SB_Qos_t Qos;

    DEBUG_START();

    UnPackSub(Msg, &MsgId, &Qos);

    /** If there's a filter, ignore this unsub. */
    for(idx = 0; SBN.RemapTable && idx < SBN.RemapTable->Entries; idx++)
    {   
        if(SBN.RemapTable->Entry[idx].ProcessorId
                == SBN.Hk.PeerStatus[PeerIdx].ProcessorId
            && SBN.RemapTable->Entry[idx].from == MsgId
            && SBN.RemapTable->Entry[idx].to == 0x0000)
        {   
            return;
        }/* end if */
    }/* end for */
      
    /*****
     * If there's a remap that would generate that
     * MID, I need to unsubscribe locally to the "from". Usually this will
     * only be one "from" but it's possible that multiple "from"s map to
     * the same "to".
     */
    for(idx = 0; SBN.RemapTable && idx < SBN.RemapTable->Entries; idx++)
    {   
        if(SBN.RemapTable->Entry[idx].ProcessorId
                == SBN.Hk.PeerStatus[PeerIdx].ProcessorId
            && SBN.RemapTable->Entry[idx].to == MsgId)
        {   
            ProcessUnsubFromPeer(PeerIdx, SBN.RemapTable->Entry[idx].from);
            RemappedFlag = 1;
        }/* end if */
    }/* end for */
      
    /* if there's no remap ID's, subscribe to the original MID. */
    if(!RemappedFlag)
    {   
        ProcessUnsubFromPeer(PeerIdx, MsgId);
    }/* end if */
}/* end SBN_ProcessUnsubFromPeer */

/**
 * When SBN starts, it queries for all existing subscriptions. This method
 * processes those subscriptions.
 *
 * @param[in] Ptr SB message pointer.
 */
// TODO: make static, also do we need the PAYLOAD ifdef still?
void SBN_ProcessAllSubscriptions(CFE_SB_PrevSubMsg_t *Ptr)
{
    int                     i = 0;

    DEBUG_START();

#ifdef SBN_PAYLOAD
    if(Ptr->Payload.Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "entries value %d in SB PrevSubMsg exceeds max %d, aborting",
            (int)Ptr->Payload.Entries,
            CFE_SB_SUB_ENTRIES_PER_PKT);
        return;
    }/* end if */

    for(i = 0; i < Ptr->Payload.Entries; i++)
    {
        ProcessLocalSub(Ptr->Payload.Entry[i].MsgId, Ptr->Payload.Entry[i].Qos);
    }/* end for */
#else /* !SBN_PAYLOAD */
    if(Ptr->Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "entries value %d in SB PrevSubMsg exceeds max %d, aborting",
            Ptr->Entries, CFE_SB_SUB_ENTRIES_PER_PKT);
        return;
    }/* end if */

    for(i = 0; i < Ptr->Entries; i++)
    {
        ProcessLocalSub(Ptr->Entry[i].MsgId, Ptr->Entry[i].Qos);
    }/* end for */
#endif /* SBN_PAYLOAD */
}/* end SBN_ProcessAllSubscriptions */

/**
 * Removes all subscriptions (unsubscribe from the local SB )
 * for the specified peer, particularly when the peer connection has been lost.
 *
 * @param[in] PeerIdx The peer index (into SBN.Peer) to clear.
 */
void SBN_RemoveAllSubsFromPeer(int PeerIdx)
{
    int     i = 0;
    uint32 Status = CFE_SUCCESS;

    DEBUG_START();

    if(PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "cannot remove all subscriptions from peer (PeerIdx=%d)",
            PeerIdx);
        return;
    }/* end if */

    for(i = 0; i < SBN.Hk.PeerStatus[PeerIdx].SubCount; i++)
    {
        Status = CFE_SB_UnsubscribeLocal(SBN.Peers[PeerIdx].Subs[i].MsgId,
            SBN.Peers[PeerIdx].Pipe);
        if(Status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                "unable to unsubscribe from message id 0x%04X",
                htons(SBN.Peers[PeerIdx].Subs[i].MsgId));
        }/* end if */
    }/* end for */

    CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_INFORMATION,
        "unsubscribed %d message id's from %s",
        (int)SBN.Hk.PeerStatus[PeerIdx].SubCount,
        SBN.Hk.PeerStatus[PeerIdx].Name);

    SBN.Hk.PeerStatus[PeerIdx].SubCount = 0;
}/* end SBN_RemoveAllSubsFromPeer */
