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
 */

#include <string.h>
#include "sbn_subs.h"
#include "sbn_main_events.h"
#include <arpa/inet.h>

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

void SBN_SendLocalSubsToPeer(int PeerIdx)
{
    int             i = 0;
    SBN_SenderId_t  sender;
    SBN_NetSub_t    msg;

    DEBUG_START();

    memset(&sender, 0, sizeof(sender));
    strncpy(sender.AppName, "SBN", sizeof(sender.AppName));
    sender.ProcessorId = CFE_CPU_ID;

    memset(&msg, 0, sizeof(msg));
    msg.Hdr.Type = SBN_SUBSCRIBE_MSG;
    msg.Hdr.MsgSize = sizeof(msg);

    for(i = 0; i < SBN.LocalSubCnt; i++)
    {
        /* Load the Protocol Buffer with the subscription details */
        msg.MsgId = SBN.LocalSubs[i].MsgId;
        msg.Qos = SBN.LocalSubs[i].Qos;

        SBN_SendNetMsg((SBN_NetPkt_t *)&msg, PeerIdx, &sender);
    }/* end for */
}/* end SBN_SendLocalSubsToPeer */

int32 SBN_CheckSubscriptionPipe(void)
{
    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_SubRprtMsg_t *SubRprtMsgPtr;
    CFE_SB_SubEntries_t SubEntry;

    /* DEBUG_START(); chatty */

    while(CFE_SB_RcvMsg(&SBMsgPtr, SBN.SubPipe, CFE_SB_POLL) == CFE_SUCCESS)
    {
        switch(CFE_SB_GetMsgId(SBMsgPtr))
        {
            case CFE_SB_ONESUB_TLM_MID:
#ifdef SBN_PAYLOAD
                SubRprtMsgPtr = (CFE_SB_SubRprtMsg_t *) SBMsgPtr;
                SubEntry.MsgId = htons(SubRprtMsgPtr->Payload.MsgId);
                SubEntry.Qos = SubRprtMsgPtr->Payload.Qos;
                SubEntry.Pipe = SubRprtMsgPtr->Payload.Pipe;

                switch(SubRprtMsgPtr->Payload.SubType)
                {
                    case CFE_SB_SUBSCRIPTION:
                        SBN_ProcessLocalSub(&SubEntry);
                        break;
                    case CFE_SB_UNSUBSCRIPTION:
                        SBN_ProcessLocalUnsub(&SubEntry);
                        break;
                    default:
                        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                            "%s:Unexpected SubType %d in "
                            "SBN_CheckSubscriptionPipe",
                            CFE_CPU_NAME, SubRprtMsgPtr->Payload.SubType);
                }/* end switch */

                return SBN_TRUE;
#else /* !SBN_PAYLOAD */
                SubRprtMsgPtr = (CFE_SB_SubRprtMsg_t *) SBMsgPtr;
                SubEntry.MsgId = htons(SubRprtMsgPtr->MsgId);
                SubEntry.Qos = SubRprtMsgPtr->Qos;
                SubEntry.Pipe = SubRprtMsgPtr->Pipe;
                switch(SubRprtMsgPtr->SubType)
                {
                    case CFE_SB_SUBSCRIPTION:
                        SBN_ProcessLocalSub(&SubEntry);
                        break;
                    case CFE_SB_UNSUBSCRIPTION:
                        SBN_ProcessLocalUnsub(&SubEntry);
                        break;
                    default:
                        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                            "%s:Unexpected SubType %d in "
                            "SBN_CheckSubscriptionPipe",
                            CFE_CPU_NAME, SubRprtMsgPtr->SubType);
                }/* end switch */


                return SBN_TRUE;
#endif /* SBN_PAYLOAD */

            case CFE_SB_ALLSUBS_TLM_MID:
                SBN_ProcessAllSubscriptions((CFE_SB_PrevSubMsg_t *) SBMsgPtr);
                return SBN_TRUE;

            default:
                CFE_EVS_SendEvent(SBN_MSG_EID, CFE_EVS_ERROR,
                        "%s:Unexpected MsgId 0x%04X on SBN.SubPipe",
                        CFE_CPU_NAME, ntohs(CFE_SB_GetMsgId(SBMsgPtr)));

        }/* end switch */
    }/* end while */

    return SBN_FALSE;
}/* end SBN_CheckSubscriptionPipe */

void SBN_ProcessSubFromPeer(int PeerIdx, CFE_SB_MsgId_t MsgId,
    CFE_SB_Qos_t Qos)
{
    int FirstOpenSlot = 0, idx = 0;

    DEBUG_START();

    if(PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "%s:Cannot process Subscription,PeerIdx(%d)OutOfRange",
             CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    if(SBN.Peer[PeerIdx].SubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "%s:Cannot process subscription from %s,max(%d)met.",
            CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if msg id already in the list, ignore */
    if(SBN_CheckPeerSubs4MsgId(&idx, MsgId, PeerIdx) == SBN_MSGID_FOUND)
    {
        return;
    }/* end if */

    /* SubscribeLocal suppresses the subscription report */
    CFE_SB_SubscribeLocal(ntohs(MsgId), SBN.Peer[PeerIdx].Pipe,
            SBN_DEFAULT_MSG_LIM);
    FirstOpenSlot = SBN.Peer[PeerIdx].SubCnt;

    /* log the subscription in the peer table */
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].MsgId = MsgId;
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].Qos = Qos;

    SBN.Peer[PeerIdx].SubCnt++;
}/* SBN_ProcessSubFromPeer */

void SBN_ProcessUnsubFromPeer(int PeerIdx, CFE_SB_MsgId_t MsgId)
{
    int             i = 0, idx = 0;

    DEBUG_START();

    if(SBN_CheckPeerSubs4MsgId(&idx, MsgId, PeerIdx)==SBN_MSGID_NOT_FOUND)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_INFORMATION,
            "%s:Cannot process unsubscription from %s,msg 0x%04X not found",
            CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, ntohs(MsgId));
        return;
    }/* end if */

    /* remove sub from array for that peer and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the Sub[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for(i = idx; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        CFE_PSP_MemCpy(&SBN.Peer[PeerIdx].Sub[i], &SBN.Peer[PeerIdx].Sub[i + 1],
            sizeof(SBN_Subs_t));
    }/* end for */

    /* decrement sub cnt */
    SBN.Peer[PeerIdx].SubCnt--;

    /* unsubscribe to the msg id on the peer pipe */
    CFE_SB_UnsubscribeLocal(ntohs(MsgId), SBN.Peer[PeerIdx].Pipe);
}/* SBN_ProcessUnsubFromPeer */

void SBN_ProcessLocalSub(CFE_SB_SubEntries_t *Ptr)
{
    int     i = 0, idx = 0;
    SBN_NetSub_t    msg;

    DEBUG_START();

    /* don't subscribe to event messages */
    if(ntohs(Ptr->MsgId) == CFE_EVS_EVENT_MSG_MID) return;

    if(SBN.LocalSubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
                "%s:Local Subscription Ignored for MsgId 0x%04X,max(%d)met",
                CFE_CPU_NAME, ntohs(Ptr->MsgId), SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if(SBN_CheckLocSubs4MsgId(&idx, Ptr->MsgId) == SBN_MSGID_FOUND)
    {
        SBN.LocalSubs[idx].InUseCtr++;
        /* does not send to peers, as they already know */
        return;
    }/* end if */

    /* log new entry into LocalSubs array */
    SBN.LocalSubs[SBN.LocalSubCnt].InUseCtr = 1;
    SBN.LocalSubs[SBN.LocalSubCnt].MsgId = Ptr->MsgId;
    SBN.LocalSubs[SBN.LocalSubCnt].Qos = Ptr->Qos;
    SBN.LocalSubCnt++;

    /* init message */
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.Hdr.SrcCpuName, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH);
    msg.Hdr.Type = SBN_SUBSCRIBE_MSG;
    msg.Hdr.MsgSize = sizeof(msg);

    msg.MsgId = Ptr->MsgId;
    msg.Qos = Ptr->Qos;

    /* send subscription to all peers if peer state is heartbeating */
    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if(SBN.Peer[i].State == SBN_HEARTBEATING)
        {
            SBN_SenderId_t sender;
            memset(&sender, 0, sizeof(sender));
            strncpy(sender.AppName, "SBN", sizeof(sender.AppName));
            sender.ProcessorId = CFE_CPU_ID;

            SBN_SendNetMsg((SBN_NetPkt_t *)&msg, i, &sender);
        }/* end if */
    }/* end for */
}/* end SBN_ProcessLocalSub */

void SBN_ProcessLocalUnsub(CFE_SB_SubEntries_t *Ptr)
{
    int             i = 0, idx = 0;
    SBN_SenderId_t  sender;
    SBN_NetSub_t    msg;

    DEBUG_START();

    memset(&sender, 0, sizeof(sender));
    strncpy(sender.AppName, "SBN", sizeof(sender.AppName));
    sender.ProcessorId = CFE_CPU_ID;

    /* find idx of matching subscription */
    if(SBN_CheckLocSubs4MsgId(&idx, Ptr->MsgId) == SBN_MSGID_NOT_FOUND)
    {
        return;
    }/* end if */

    SBN.LocalSubs[idx].InUseCtr--;

    /* do not modify the array and tell peers
    ** until the # of local subscriptions = 0
    */
    if(SBN.LocalSubs[idx].InUseCtr > 0)
    {
        return;
    }/* end if */

    /* remove sub from array for and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the LocalSubs[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for(i = idx; i < SBN.LocalSubCnt; i++)
    {
        CFE_PSP_MemCpy(&SBN.LocalSubs[i], &SBN.LocalSubs[i + 1],
            sizeof(SBN_Subs_t));
    }/* end for */

    SBN.LocalSubCnt--;

    /* initialize the network msg */
    memset(&msg, 0, sizeof(msg));
    msg.Hdr.Type = SBN_UN_SUBSCRIBE_MSG;
    strncpy(msg.Hdr.SrcCpuName, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH);
    msg.MsgId = Ptr->MsgId;
    msg.Qos = Ptr->Qos;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if(SBN.Peer[i].State == SBN_HEARTBEATING)
        {
            SBN_SendNetMsg((SBN_NetPkt_t *)&msg, i, &sender);
        }/* end if */
    }/* end for */
}/* end SBN_ProcessLocalUnsub */

void SBN_ProcessAllSubscriptions(CFE_SB_PrevSubMsg_t *Ptr)
{
    int                     i = 0;
    CFE_SB_SubEntries_t     SubEntry;

    DEBUG_START();

#ifdef SBN_PAYLOAD
    if(Ptr->Payload.Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "%s:Entries value %d in SB PrevSubMsg exceeds max %d, aborting",
            CFE_CPU_NAME, Ptr->Payload.Entries, CFE_SB_SUB_ENTRIES_PER_PKT);
        return;
    }/* end if */

    for(i = 0; i < Ptr->Payload.Entries; i++)
    {
        SubEntry.MsgId = htons(Ptr->Payload.Entry[i].MsgId);
        SubEntry.Pipe = Ptr->Payload.Entry[i].Pipe;
        SubEntry.Qos = Ptr->Payload.Entry[i].Qos;

        SBN_ProcessLocalSub(&SubEntry);
    }/* end for */
#else /* !SBN_PAYLOAD */
    if(Ptr->Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_ERROR,
            "%s:Entries value %d in SB PrevSubMsg exceeds max %d, aborting",
            CFE_CPU_NAME, Ptr->Entries, CFE_SB_SUB_ENTRIES_PER_PKT);
        return;
    }/* end if */

    for(i = 0; i < Ptr->Entries; i++)
    {
        SubEntry.MsgId = htons(Ptr->Entry[i].MsgId);
        SubEntry.Pipe = Ptr->Entry[i].Pipe;
        SubEntry.Qos = Ptr->Entry[i].Qos;

        SBN_ProcessLocalSub(&SubEntry);
    }/* end for */
#endif /* SBN_PAYLOAD */
}/* end SBN_ProcessAllSubscriptions */

int SBN_CheckLocSubs4MsgId(int *IdxPtr, CFE_SB_MsgId_t MsgId)
{
    int     i = 0;

    DEBUG_START();

    for(i = 0; i < SBN.LocalSubCnt; i++)
    {
        if(SBN.LocalSubs[i].MsgId == MsgId)
        {
            *IdxPtr = i;
            return SBN_MSGID_FOUND;
        }/* end if */
    }/* end for */

    return SBN_MSGID_NOT_FOUND;
}/* end SBN_CheckLocSubs4MsgId */

int SBN_CheckPeerSubs4MsgId(int *SubIdxPtr, CFE_SB_MsgId_t MsgId,
        int PeerIdx)
{
    int     i = 0;

    DEBUG_START();

    for(i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        if(SBN.Peer[PeerIdx].Sub[i].MsgId == MsgId)
        {
            *SubIdxPtr = i;
            return SBN_MSGID_FOUND;
        }/* end if */
    }/* end for */

    return SBN_MSGID_NOT_FOUND;

}/* end SBN_CheckPeerSubs4MsgId */

void SBN_RemoveAllSubsFromPeer(int PeerIdx)
{
    int     i = 0;

    DEBUG_START();

    if(PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "%s:Cannot remove all subs from peer,PeerIdx(%d)OutOfRange",
            CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    for(i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        CFE_SB_UnsubscribeLocal(ntohs(SBN.Peer[PeerIdx].Sub[i].MsgId),
            SBN.Peer[PeerIdx].Pipe);
    }/* end for */

    CFE_EVS_SendEvent(SBN_SUB_EID, CFE_EVS_INFORMATION,
        "%s:UnSubscribed %d MsgIds from %s", CFE_CPU_NAME,
        SBN.Peer[PeerIdx].SubCnt, SBN.Peer[PeerIdx].Name);

    SBN.Peer[PeerIdx].SubCnt = 0;
}/* end SBN_RemoveAllSubsFromPeer */
