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

#include "sbn_app.h"
#include <string.h>
#include <arpa/inet.h>
#include "cfe_msgids.h"

// TODO: instead of using void * for the buffer for SBN messages, use
// a struct that has the SBN header in packed bytes.

/**
 * Informs the software bus to send this application all subscription requests.
 */
void SBN_SendSubsRequests(void)
{
    CFE_SB_CmdHdr_t     SBCmdMsg;

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
 * @param[in] MsgID The CCSDS message ID for the subscription.
 * @param[in] Qos The CCSDS quality of service for the subscription.
 *
 * @return The pointer into the buffer after the packed sub.
 */
static void *PackSub(void *SubMsg, CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t Qos)
{
    MsgID = CFE_MAKE_BIG16(MsgID);
    memcpy(SubMsg, &MsgID, sizeof(MsgID));
    SubMsg += sizeof(MsgID);
    memcpy(SubMsg, &Qos, sizeof(Qos));
    return SubMsg + sizeof(Qos);
}

/**
 * \brief Utility function to unpack SBN protocol subscription information.
 *
 * @param[in] SubMsg Source SBN msg buffer for subscription information.
 * @param[out] MsgIDPtr A pointer to the CCSDS message ID for the subscription.
 * @param[out] QosPtr A pointer to CCSDS quality of service for the
 *             subscription.
 */
static void UnPackSub(void *SubMsg, CFE_SB_MsgId_t *MsgIDPtr,
    CFE_SB_Qos_t *QosPtr)
{
    memcpy(MsgIDPtr, SubMsg, sizeof(*MsgIDPtr));
    *MsgIDPtr = CFE_MAKE_BIG16(*MsgIDPtr);
    memcpy(QosPtr, SubMsg + sizeof(*MsgIDPtr), sizeof(*QosPtr));
}

/**
 * \brief Sends a local subscription over the wire to a peer.
 *
 * @param[in] SubType Whether this is a subscription or unsubscription.
 * @param[in] MsgID The CCSDS message ID being (un)subscribed.
 * @param[in] Qos The CCSDS quality of service being (un)subscribed.
 * @param[in] Peer The Peer interface
 */
static void SendLocalSubToPeer(int SubType, CFE_SB_MsgId_t MsgID,
    CFE_SB_Qos_t Qos, SBN_PeerInterface_t *Peer)
{
    SBN_PackedSubs_t SubMsg;
    memset(&SubMsg, 0, sizeof(SubMsg));
    memcpy(SubMsg.VersionHash, SBN_IDENT, SBN_IDENT_LEN);
    SubMsg.SubCnt = CFE_MAKE_BIG16(1);

    PackSub(&SubMsg.Subs[0], MsgID, Qos);

    SBN_SendNetMsg(SubType,
        SBN_IDENT_LEN + sizeof(SubMsg.SubCnt) + SBN_PACKED_HDR_SIZE,
        (SBN_Payload_t *)&SubMsg, Peer);
}/* end SendLocalSubToPeer */

/**
 * \brief Sends all local subscriptions over the wire to a peer.
 *
 * @param[in] Peer The peer interface.
 */
void SBN_SendLocalSubsToPeer(SBN_PeerInterface_t *Peer)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    SBN_PackedSubs_t SubMsg;
    memset(&SubMsg, 0, sizeof(SubMsg));
    memcpy(SubMsg.VersionHash, SBN_IDENT, SBN_IDENT_LEN);
    SubMsg.SubCnt = CFE_MAKE_BIG16(SBN.Hk.SubCount);

    int i = 0;
    for(i = 0; i < SBN.Hk.SubCount; i++)
    {
        PackSub(&SubMsg.Subs[i], SBN.LocalSubs[i].MsgID,
            SBN.LocalSubs[i].Qos);
    }/* end for */

    SBN_SendNetMsg(SBN_SUBSCRIBE_MSG, 
        SBN_IDENT_LEN + sizeof(SubMsg.Subs)
            + SBN_PACKED_HDR_SIZE * SBN.Hk.SubCount,
        (SBN_Payload_t *)&SubMsg, Peer);
}/* end SBN_SendLocalSubsToPeer */

/**
 * Utility to find the subscription index (SBN.LocalSubs)
 * that is subscribed to the CCSDS message ID.
 *
 * @param[out] IdxPtr The subscription index found.
 * @param[in] MsgID The CCSDS message ID of the subscription being sought.
 * @return TRUE if found.
 */
static int IsMsgIDSub(int *IdxPtr, CFE_SB_MsgId_t MsgID)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    int     i = 0;

    for(i = 0; i < SBN.Hk.SubCount; i++)
    {
        if(SBN.LocalSubs[i].MsgID == MsgID)
        {
            if (IdxPtr)
            {
                *IdxPtr = i;
            }/* end if */

            return TRUE;
        }/* end if */
    }/* end for */

    return FALSE;
}/* end IsMsgIDSub */

/**
 * \brief Is this peer subscribed to this message ID? If so, what is the index
 *        of the subscription?
 *
 * @param[out] SubIdxPtr The pointer to the subscription index value.
 * @param[in] MsgID The CCSDS message ID of the subscription being sought.
 * @param[in] Peer The peer interface.
 *
 * @return TRUE if found.
 */
static int IsPeerSubMsgID(int *SubIdxPtr, CFE_SB_MsgId_t MsgID,
    SBN_PeerInterface_t *Peer)
{
    int     i = 0;

    for(i = 0; i < Peer->Status.SubCount; i++)
    {
        if(Peer->Subs[i].MsgID == MsgID)
        {
            *SubIdxPtr = i;
            return TRUE;
        }/* end if */
    }/* end for */

    return FALSE;

}/* end IsPeerSubMsgID */

/**
 * \brief I have seen a local subscription, send it on to peers if this is the
 * first instance of a subscription for this message ID.
 *
 * @param[in] MsgID The CCSDS Message ID of the local subscription.
 * @param[in] Qos The CCSDS quality of service of the local subscription.
 */
static void ProcessLocalSub(CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t Qos)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    /* don't subscribe to event messages */
    if(MsgID == CFE_EVS_EVENT_MSG_MID) return;

    if(SBN.Hk.SubCount >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                "local subscription ignored for MsgID 0x%04X, max (%d) met",
                ntohs(MsgID), SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    int SubIdx = 0;

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if(IsMsgIDSub(&SubIdx, MsgID))
    {
        SBN.LocalSubs[SubIdx].InUseCtr++;
        /* does not send to peers, as they already know */
        return;
    }/* end if */

    /* log new entry into LocalSubs array */
    SBN.LocalSubs[SBN.Hk.SubCount].InUseCtr = 1;
    SBN.LocalSubs[SBN.Hk.SubCount].MsgID = MsgID;
    SBN.LocalSubs[SBN.Hk.SubCount].Qos = Qos;
    SBN.Hk.SubCount++;

    int NetIdx = 0, PeerIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        for(PeerIdx = 0; PeerIdx < Net->Status.PeerCount; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            SendLocalSubToPeer(SBN_SUBSCRIBE_MSG, MsgID, Qos, Peer);
        }/* end for */
    }/* end for */
}/* end ProcessLocalSub */

/**
 * \brief I have seen a local unsubscription, send it on to peers if this is the
 * last instance of a subscription for this message ID.
 *
 * @param[in] MsgID The CCSDS Message ID of the local unsubscription.
 * @param[in] Qos The CCSDS quality of service of the local unsubscription.
 */
static void ProcessLocalUnsub(CFE_SB_MsgId_t MsgID)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    int SubIdx;
    /* find idx of matching subscription */
    if(!IsMsgIDSub(&SubIdx, MsgID))
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
    for(; SubIdx < SBN.Hk.SubCount; SubIdx++)
    {
        memcpy(&SBN.LocalSubs[SubIdx], &SBN.LocalSubs[SubIdx + 1],
            sizeof(SBN_Subs_t));
    }/* end for */

    SBN.Hk.SubCount--;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    int NetIdx = 0, PeerIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        for(PeerIdx = 0; PeerIdx < Net->Status.PeerCount; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            SendLocalSubToPeer(SBN_UN_SUBSCRIBE_MSG,
                SBN.LocalSubs[PeerIdx].MsgID,
                SBN.LocalSubs[PeerIdx].Qos, Peer);
        }/* end for */
    }/* end for */
}/* end ProcessLocalUnsub */

/**
 * \brief Check the local pipe for subscription messages. Send them on to peers
 *        if there are any (new)subscriptions.
 * @return TRUE if subscriptions received.
 */
int32 SBN_CheckSubscriptionPipe(void)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    int32 RecvSub = FALSE;
    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_SubRprtMsg_t *SubRprtMsgPtr;

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

static void AddSub(SBN_PeerInterface_t *Peer, CFE_SB_MsgId_t MsgID,
    CFE_SB_Qos_t Qos)
{   
    int idx = 0, FirstOpenSlot = Peer->Status.SubCount;
    uint32 Status = CFE_SUCCESS;
    
    /* if msg id already in the list, ignore */
    if(IsPeerSubMsgID(&idx, MsgID, Peer))
    {   
        return;
    }/* end if */

    if(Peer->Status.SubCount >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "cannot process subscription from '%s', max (%d) met",
            Peer->Status.Name, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */
    
    /* SubscribeLocal suppresses the subscription report */
    Status = CFE_SB_SubscribeLocal(MsgID, Peer->Pipe, SBN_DEFAULT_MSG_LIM);
    if(Status != CFE_SUCCESS)
    {   
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "Unable to subscribe to MID 0x%04X", htons(MsgID));
        return;
    }/* end if */
    
    /* log the subscription in the peer table */ 
    Peer->Subs[FirstOpenSlot].MsgID = MsgID;
    Peer->Subs[FirstOpenSlot].Qos = Qos;
    
    Peer->Status.SubCount++;
}/* end AddSub */

static void ProcessSubFromPeer(SBN_PeerInterface_t *Peer, CFE_SB_MsgId_t MsgID,
    CFE_SB_Qos_t Qos)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    /** If there's a filter, ignore the sub request.  */
    int idx = 0;
    for(idx = 0; SBN.RemapTable && idx < SBN.RemapTable->Entries; idx++)
    {
        if(SBN.RemapTable->Entry[idx].ProcessorID == Peer->Status.ProcessorID
            && SBN.RemapTable->Entry[idx].from == MsgID
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
    boolean RemappedFlag = FALSE;
    for(idx = 0; SBN.RemapTable && idx < SBN.RemapTable->Entries; idx++)
    {
        if(SBN.RemapTable->Entry[idx].ProcessorID
                == Peer->Status.ProcessorID
            && SBN.RemapTable->Entry[idx].to == MsgID)
        {
            AddSub(Peer, SBN.RemapTable->Entry[idx].from, Qos);
            RemappedFlag = TRUE;
        }/* end if */
    }/* end for */

    /* if there's no remap ID's, subscribe to the original MID. */
    if(!RemappedFlag)
    {
        AddSub(Peer, MsgID, Qos);
    }/* end if */
}/* ProcessSubFromPeer */

/**
 * \brief Process a subscription message from a peer.
 *
 * @param[in] PeerIdx The peer index (in SBN.Peer)
 * @param[in] Msg The subscription SBN message.
 */
void SBN_ProcessSubsFromPeer(SBN_PeerInterface_t *Peer, void *Msg)
{
    SBN_PackedSubs_t *SubMsg = Msg;

    if(strncmp(SubMsg->VersionHash, SBN_IDENT, SBN_IDENT_LEN))
    {
        CFE_EVS_SendEvent(SBN_PROTO_EID, CFE_EVS_ERROR,
            "version number mismatch with peer CpuID %d",
            Peer->Status.ProcessorID);
    }

    int SubCnt = CFE_MAKE_BIG16(SubMsg->SubCnt);

    int SubIdx = 0;
    for(SubIdx = 0; SubIdx < SubCnt; SubIdx++)
    {
        CFE_SB_MsgId_t MsgID;
        CFE_SB_Qos_t Qos;

        UnPackSub((void *)&SubMsg->Subs[SubIdx], &MsgID, &Qos);

        ProcessSubFromPeer(Peer, MsgID, Qos);
    }/* end for */
}/* SBN_ProcessSubsFromPeer */

static void ProcessUnsubFromPeer(SBN_PeerInterface_t *Peer,
    CFE_SB_MsgId_t MsgID)
{
    int i = 0, idx = 0;
    uint32 Status = CFE_SUCCESS;

    if(IsPeerSubMsgID(&idx, MsgID, Peer))
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_INFORMATION,
            "%s:Cannot process unsubscription from %s,msg 0x%04X not found",
            CFE_CPU_NAME, Peer->Status.Name, htons(MsgID));
        return;
    }/* end if */

    /* remove sub from array for that peer and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the Subs[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for(i = idx; i < Peer->Status.SubCount; i++)
    {
        memcpy(&Peer->Subs[i],
            &Peer->Subs[i + 1],
            sizeof(SBN_Subs_t));
    }/* end for */

    /* decrement sub cnt */
    Peer->Status.SubCount--;

    /* unsubscribe to the msg id on the peer pipe */
    Status = CFE_SB_UnsubscribeLocal(MsgID, Peer->Pipe);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "Unable to unsubscribe from MID 0x%04X", htons(MsgID));
        return;
    }/* end if */
}/* end ProcessUnsubFromPeer */

/**
 * \brief Process an unsubscription message from a peer.
 *
 * @param[in] PeerIdx The peer index (in SBN.Peer)
 * @param[in] Msg The unsubscription SBN message.
 */
void SBN_ProcessUnsubsFromPeer(SBN_PeerInterface_t *Peer, void *Msg)
{
    uint32 AppID = 0;
    CFE_ES_GetAppID(&AppID);

    SBN_PackedSubs_t *SubMsg = Msg;

    if(strncmp(SubMsg->VersionHash, SBN_IDENT, SBN_IDENT_LEN))
    {
        CFE_EVS_SendEvent(SBN_PROTO_EID, CFE_EVS_ERROR,
            "version number mismatch with peer CpuID %d",
            Peer->Status.ProcessorID);
    }

    int SubCnt = CFE_MAKE_BIG16(SubMsg->SubCnt);

    int SubIdx = 0;
    for(SubIdx = 0; SubIdx < SubCnt; SubIdx++)
    {
        int idx = 0, RemappedFlag = 0;
        CFE_SB_MsgId_t MsgID = 0x0000;
        CFE_SB_Qos_t Qos;

        UnPackSub((void *)&SubMsg->Subs[SubIdx], &MsgID, &Qos);

        /** If there's a filter, ignore this unsub. */
        for(idx = 0; SBN.RemapTable && idx < SBN.RemapTable->Entries; idx++)
        {   
            if(SBN.RemapTable->Entry[idx].ProcessorID
                    == Peer->Status.ProcessorID
                && SBN.RemapTable->Entry[idx].from == MsgID
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
            if(SBN.RemapTable->Entry[idx].ProcessorID
                    == Peer->Status.ProcessorID
                && SBN.RemapTable->Entry[idx].to == MsgID)
            {   
                ProcessUnsubFromPeer(Peer, SBN.RemapTable->Entry[idx].from);
                RemappedFlag = 1;
            }/* end if */
        }/* end for */
          
        /* if there's no remap ID's, subscribe to the original MID. */
        if(!RemappedFlag)
        {   
            ProcessUnsubFromPeer(Peer, MsgID);
        }/* end if */
    }/* end for */
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
        ProcessLocalSub(Ptr->Entry[i].MsgID, Ptr->Entry[i].Qos);
    }/* end for */
#endif /* SBN_PAYLOAD */
}/* end SBN_ProcessAllSubscriptions */

/**
 * Removes all subscriptions (unsubscribe from the local SB )
 * for the specified peer, particularly when the peer connection has been lost.
 *
 * @param[in] PeerIdx The peer index (into SBN.Peer) to clear.
 */
void SBN_RemoveAllSubsFromPeer(SBN_PeerInterface_t *Peer)
{
    int i = 0;
    uint32 Status = CFE_SUCCESS;

    for(i = 0; i < Peer->Status.SubCount; i++)
    {
        Status = CFE_SB_UnsubscribeLocal(Peer->Subs[i].MsgID,
            Peer->Pipe);
        if(Status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                "unable to unsubscribe from message id 0x%04X",
                htons(Peer->Subs[i].MsgID));
        }/* end if */
    }/* end for */

    CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_INFORMATION,
        "unsubscribed %d message id's from %s",
        (int)Peer->Status.SubCount,
        Peer->Status.Name);

    Peer->Status.SubCount = 0;
}/* end SBN_RemoveAllSubsFromPeer */
