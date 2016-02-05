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
#include <arpa/inet.h>

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
} /* end SBN_SendSubsRequests */

void SBN_SendLocalSubsToPeer(int PeerIdx)
{
    int             i = 0;
    SBN_SenderId_t  sender;

    strncpy(sender.AppName, "SBN", sizeof(sender.AppName));
    sender.ProcessorId = CFE_CPU_ID;

    if(SBN.LocalSubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_LSC_ERR1_EID, CFE_EVS_ERROR,
            "%s:Error sending subs to %s,LclSubCnt=%d,max=%d", CFE_CPU_NAME,
            SBN.Peer[PeerIdx].Name, SBN.LocalSubCnt, SBN_MAX_SUBS_PER_PEER);
        return;
    } /* end if */

    for(i = 0; i < SBN.LocalSubCnt; i++)
    {
        /* Load the Protocol Buffer with the subscription details */
        SBN.MsgBuf.Sub.MsgId = SBN.LocalSubs[i].MsgId;
        SBN.MsgBuf.Sub.Qos = SBN.LocalSubs[i].Qos;

        SBN_SendNetMsg(SBN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), PeerIdx,
            &sender);
    }/* end for */
}/* end SBN_SendLocalSubsToPeer */

int32 SBN_CheckSubscriptionPipe(void)
{
    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_SubRprtMsg_t *SubRprtMsgPtr;
    CFE_SB_SubEntries_t SubEntry;
    int32 ResponseReceived = SBN_FALSE;

    while (CFE_SB_RcvMsg(&SBMsgPtr, SBN.SubPipe, CFE_SB_POLL) == CFE_SUCCESS)
    {
        switch (CFE_SB_GetMsgId(SBMsgPtr))
        {
            case CFE_SB_ONESUB_TLM_MID:
#ifdef SBN_PAYLOAD
                SubRprtMsgPtr = (CFE_SB_SubRprtMsg_t *) SBMsgPtr;
                SubEntry.MsgId = SubRprtMsgPtr->Payload.MsgId;
                SubEntry.Qos = SubRprtMsgPtr->Payload.Qos;
                SubEntry.Pipe = SubRprtMsgPtr->Payload.Pipe;
                if(SubRprtMsgPtr->Payload.SubType == CFE_SB_SUBSCRIPTION)
                    SBN_ProcessLocalSub(&SubEntry);
                else if(SubRprtMsgPtr->Payload.SubType == CFE_SB_UNSUBSCRIPTION)
                    SBN_ProcessLocalUnsub(&SubEntry);
                else
                    CFE_EVS_SendEvent(SBN_SUBTYPE_ERR_EID, CFE_EVS_ERROR,
                        "%s:Unexpected SubType %d in SBN_CheckSubscriptionPipe",
                        CFE_CPU_NAME, SubRprtMsgPtr->Payload.SubType);

                ResponseReceived = SBN_TRUE;
                break;
#else /* !SBN_PAYLOAD */
                SubRprtMsgPtr = (CFE_SB_SubRprtMsg_t *) SBMsgPtr;
                SubEntry.MsgId = SubRprtMsgPtr->MsgId;
                SubEntry.Qos = SubRprtMsgPtr->Qos;
                SubEntry.Pipe = SubRprtMsgPtr->Pipe;
                if(SubRprtMsgPtr->SubType == CFE_SB_SUBSCRIPTION)
                    SBN_ProcessLocalSub(&SubEntry);
                else if(SubRprtMsgPtr->SubType == CFE_SB_UNSUBSCRIPTION)
                    SBN_ProcessLocalUnsub(&SubEntry);
                else
                    CFE_EVS_SendEvent(SBN_SUBTYPE_ERR_EID, CFE_EVS_ERROR,
                        "%s:Unexpected SubType %d in SBN_CheckSubscriptionPipe",
                        CFE_CPU_NAME, SubRprtMsgPtr->SubType);

                ResponseReceived = SBN_TRUE;
                break;
#endif /* SBN_PAYLOAD */

            case CFE_SB_ALLSUBS_TLM_MID:
                SBN_ProcessAllSubscriptions((CFE_SB_PrevSubMsg_t *) SBMsgPtr);
                ResponseReceived = SBN_TRUE;
                break;

            default:
                CFE_EVS_SendEvent(SBN_MSGID_ERR_EID, CFE_EVS_ERROR,
                        "%s:Unexpected MsgId 0x%04X on SBN.SubPipe",
                        CFE_CPU_NAME, ntohs(CFE_SB_GetMsgId(SBMsgPtr)));

        }/* end switch */
    }/* end while */

    return ResponseReceived;
}/* end SBN_CheckSubscriptionPipe */

void SBN_ProcessSubFromPeer(int PeerIdx)
{
    int FirstOpenSlot = 0, idx = 0;

    if(PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR1_EID, CFE_EVS_ERROR,
                "%s:Cannot process Subscription,PeerIdx(%d)OutOfRange",
                 CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    if(SBN.Peer[PeerIdx].SubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_ERR_EID, CFE_EVS_ERROR,
                "%s:Cannot process subscription from %s,max(%d)met.",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if msg id already in the list, ignore */
    if(SBN_CheckPeerSubs4MsgId(&idx, SBN.MsgBuf.Sub.MsgId,
            PeerIdx) == SBN_MSGID_FOUND)
    {
        CFE_EVS_SendEvent(SBN_DUP_SUB_EID, CFE_EVS_INFORMATION,
                "%s:subscription from %s ignored,msg 0x%04X already logged",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name,
                ntohs(SBN.MsgBuf.Sub.MsgId));
        return;
    }/* end if */

    /* SubscribeLocal suppresses the subscription report */
    CFE_SB_SubscribeLocal(ntohs(SBN.MsgBuf.Sub.MsgId), SBN.Peer[PeerIdx].Pipe,
            SBN_DEFAULT_MSG_LIM);
    FirstOpenSlot = SBN.Peer[PeerIdx].SubCnt;

    /* log the subscription in the peer table */
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].MsgId = SBN.MsgBuf.Sub.MsgId;
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].Qos = SBN.MsgBuf.Sub.Qos;

    SBN.Peer[PeerIdx].SubCnt++;
}/* SBN_ProcessSubFromPeer */

void SBN_ProcessUnsubFromPeer(int PeerIdx)
{
    int     i = 0, idx = 0;

    if(PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR2_EID, CFE_EVS_ERROR,
                "%s:Cannot process unsubscription,PeerIdx(%d)OutOfRange",
                CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    if(SBN.Peer[PeerIdx].SubCnt == 0)
    {
        CFE_EVS_SendEvent(SBN_NO_SUBS_EID, CFE_EVS_INFORMATION,
                "%s:Cannot process unsubscription from %s,none logged",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name);
        return;
    }/* end if */

    if(SBN_CheckPeerSubs4MsgId(&idx, SBN.MsgBuf.Sub.MsgId,
            PeerIdx)==SBN_MSGID_NOT_FOUND)
    {
        CFE_EVS_SendEvent(SBN_SUB_NOT_FOUND_EID, CFE_EVS_INFORMATION,
                "%s:Cannot process unsubscription from %s,msg 0x%04X not found",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, ntohs(SBN.MsgBuf.Sub.MsgId));
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
    CFE_SB_UnsubscribeLocal(ntohs(SBN.MsgBuf.Sub.MsgId),
        SBN.Peer[PeerIdx].Pipe);
}/* SBN_ProcessUnsubFromPeer */

void SBN_ProcessLocalSub(CFE_SB_SubEntries_t *Ptr)
{
    int     i = 0, idx = 0;
    SBN_SenderId_t sender;

    strncpy(sender.AppName, "SBN", sizeof(sender.AppName));
    sender.ProcessorId = CFE_CPU_ID;

    if(SBN.LocalSubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_LSC_ERR2_EID, CFE_EVS_ERROR,
                "%s:Local Subscription Ignored for MsgId 0x%04X,max(%d)met",
                CFE_CPU_NAME, ntohs(Ptr->MsgId), SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if(SBN_CheckLocSubs4MsgId(&idx, Ptr->MsgId) == SBN_MSGID_FOUND)
    {
        /* check idx before writing */
        if(idx < SBN.LocalSubCnt) SBN.LocalSubs[idx].InUseCtr++;
        return;
    }/* end if */

    /* log new entry into LocalSubs array */
    SBN.LocalSubs[SBN.LocalSubCnt].InUseCtr = 1;
    SBN.LocalSubs[SBN.LocalSubCnt].MsgId = Ptr->MsgId;
    SBN.LocalSubs[SBN.LocalSubCnt].Qos = Ptr->Qos;
    SBN.LocalSubCnt++;

    /* initialize the network msg */
    SBN.MsgBuf.Hdr.Type = SBN_SUBSCRIBE_MSG;
    strncpy(SBN.MsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);
    SBN.MsgBuf.Sub.MsgId = Ptr->MsgId;
    SBN.MsgBuf.Sub.Qos = Ptr->Qos;

    /* send subscription to all peers if peer state is heartbeating */
    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if(SBN.Peer[i].State == SBN_HEARTBEATING)
            SBN_SendNetMsg(SBN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), i, &sender);
    }/* end for */
}/* end SBN_ProcessLocalSub */

void SBN_ProcessLocalUnsub(CFE_SB_SubEntries_t *Ptr)
{
    int             i = 0, idx = 0;
    SBN_SenderId_t  sender;

    strncpy(sender.AppName, "SBN", sizeof(sender.AppName));
    sender.ProcessorId = CFE_CPU_ID;

    /* find idx of matching subscription */
    if(SBN_CheckLocSubs4MsgId(&idx, Ptr->MsgId) == SBN_MSGID_NOT_FOUND)
        return;

    SBN.LocalSubs[idx].InUseCtr--;

    /* do not modify the array and tell peers
    ** until the # of local subscriptions = 0
    */
    if(SBN.LocalSubs[idx].InUseCtr != 0)
        return;

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
    SBN.MsgBuf.Hdr.Type = SBN_UN_SUBSCRIBE_MSG;
    strncpy(SBN.MsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);
    SBN.MsgBuf.Sub.MsgId = Ptr->MsgId;
    SBN.MsgBuf.Sub.Qos = Ptr->Qos;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if(SBN.Peer[i].State == SBN_HEARTBEATING)
        {
            SBN_SendNetMsg(SBN_UN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), i,
                &sender);
        }/* end if */
    }/* end for */
}/* end SBN_ProcessLocalUnsub */

void SBN_ProcessAllSubscriptions(CFE_SB_PrevSubMsg_t *Ptr)
{
    int                     i = 0;
    CFE_SB_SubEntries_t     SubEntry;

#ifdef SBN_PAYLOAD
    if(Ptr->Payload.Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        CFE_EVS_SendEvent(SBN_ENTRY_ERR_EID, CFE_EVS_ERROR,
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
        CFE_EVS_SendEvent(SBN_ENTRY_ERR_EID, CFE_EVS_ERROR,
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

    if(PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR3_EID, CFE_EVS_ERROR,
            "%s:Cannot remove all subs from peer,PeerIdx(%d)OutOfRange",
            CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    for(i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        CFE_SB_UnsubscribeLocal(ntohs(SBN.Peer[PeerIdx].Sub[i].MsgId),
            SBN.Peer[PeerIdx].Pipe);
    }/* end for */

    CFE_EVS_SendEvent(SBN_UNSUB_CNT_EID, CFE_EVS_INFORMATION,
        "%s:UnSubscribed %d MsgIds from %s", CFE_CPU_NAME,
        SBN.Peer[PeerIdx].SubCnt, SBN.Peer[PeerIdx].Name);

    SBN.Peer[PeerIdx].SubCnt = 0;
}/* end SBN_RemoveAllSubsFromPeer */

void SBN_ShowPeerSubs(int PeerIdx)
{
    int     i = 0;

    OS_printf("\n%s:PeerIdx %d, Name %s, State %s\n", CFE_CPU_NAME, PeerIdx,
        SBN.Peer[PeerIdx].Name, SBN_LIB_StateNum2Str(SBN.Peer[PeerIdx].State));
    OS_printf("%s:Rcvd %d subscriptions from %s\n", CFE_CPU_NAME,
        SBN.Peer[PeerIdx].SubCnt, SBN.Peer[PeerIdx].Name);

    for(i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        OS_printf("0x%04X  ", ntohs(SBN.Peer[PeerIdx].Sub[i].MsgId));
        if(((i + 1) % 5) == 0)
            OS_printf("\n");
    }/* end for */
}/* end SBN_ShowPeerSubs */

void SBN_ShowLocSubs(void)
{
    int     i = 0;

    OS_printf("\n%s:%d Local Subscriptions\n", CFE_CPU_NAME, SBN.LocalSubCnt);
    for(i = 0; i < SBN.LocalSubCnt; i++)
    {
        OS_printf("0x%04X  ", ntohs(SBN.LocalSubs[i].MsgId));
        if(((i + 1) % 5) == 0) OS_printf("\n");
    }/* end for */
}/* end SBN_ShowLocSubs */

void SBN_ShowAllSubs(void)
{
    int     i = 0;

    SBN_ShowLocSubs();
    OS_printf("\n");
    for(i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if(SBN.Peer[i].InUse == SBN_IN_USE)
            SBN_ShowPeerSubs(i);
    }/* end for */
}/* end SBN_ShowAllSubs */
