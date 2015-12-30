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

void SBN_SendSubsRequests(void) {
    CFE_SB_CmdHdr_t SBCmdMsg;

    /* Turn on SB subscription reporting */
    CFE_SB_InitMsg(&SBCmdMsg.Pri, CFE_SB_CMD_MID, sizeof(CFE_SB_CmdHdr_t), TRUE);
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) &SBCmdMsg, CFE_SB_ENABLE_SUB_REPORTING_CC);
    CFE_SB_SendMsg((CFE_SB_MsgPtr_t) &SBCmdMsg);

    /* Request a list of previous subscriptions from SB */
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) &SBCmdMsg, CFE_SB_SEND_PREV_SUBS_CC);
    CFE_SB_SendMsg((CFE_SB_MsgPtr_t) &SBCmdMsg);
}

void SBN_SendLocalSubsToPeer(uint32 PeerIdx) {
    uint32 i;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_SendLocalSubsToPeer\n");
    }

    if (SBN.LocalSubCnt >= SBN_MAX_SUBS_PER_PEER) {
        CFE_EVS_SendEvent(SBN_LSC_ERR1_EID, CFE_EVS_ERROR,
                "%s:Error sending subs to %s,LclSubCnt=%d,max=%d", CFE_CPU_NAME,
                SBN.Peer[PeerIdx].Name, SBN.LocalSubCnt, SBN_MAX_SUBS_PER_PEER);
        return;
    }

    for (i = 0; i < SBN.LocalSubCnt; i++) {
        /* Load the Protocol Buffer with the subscription details */
        SBN.DataMsgBuf.Sub.MsgId = SBN.LocalSubs[i].MsgId;
        SBN.DataMsgBuf.Sub.Qos = SBN.LocalSubs[i].Qos;

        SBN_SendNetMsg(SBN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), PeerIdx, NULL, SBN_FALSE);
    }/* end for */
}/* end SBN_SendLocalSubsToPeer */

int32 SBN_CheckSubscriptionPipe(void) {
    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_SubRprtMsg_t *SubRprtMsgPtr;
    CFE_SB_SubEntries_t SubEntry;
    CFE_SB_SubEntries_t *SubEntryPtr = &SubEntry;
    int32 ResponseReceived = SBN_FALSE;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckSubscriptionPipe\n");
    }

    while (CFE_SB_RcvMsg(&SBMsgPtr, SBN.SubPipe, CFE_SB_POLL) == CFE_SUCCESS) {
        switch (CFE_SB_GetMsgId(SBMsgPtr)) {

            case CFE_SB_ONESUB_TLM_MID:
                SubRprtMsgPtr = (CFE_SB_SubRprtMsg_t *) SBMsgPtr;
                SubEntry.MsgId = SubRprtMsgPtr->MsgId;
                SubEntry.Qos = SubRprtMsgPtr->Qos;
                SubEntry.Pipe = SubRprtMsgPtr->Pipe;
                if (SubRprtMsgPtr->SubType == CFE_SB_SUBSCRIPTION)
                    SBN_ProcessLocalSub(SubEntryPtr);
                else if (SubRprtMsgPtr->SubType == CFE_SB_UNSUBSCRIPTION)
                    SBN_ProcessLocalUnsub(SubEntryPtr);
                else
                    CFE_EVS_SendEvent(SBN_SUBTYPE_ERR_EID, CFE_EVS_ERROR,
                            "%s:Unexpected SubType %d in SBN_CheckSubscriptionPipe",
                            CFE_CPU_NAME, SubRprtMsgPtr->SubType);

                ResponseReceived = SBN_TRUE;
                break;

            case CFE_SB_ALLSUBS_TLM_MID:
                SBN_ProcessAllSubscriptions((CFE_SB_PrevSubMsg_t *) SBMsgPtr);
                ResponseReceived = SBN_TRUE;
                break;

            default:
                CFE_EVS_SendEvent(SBN_MSGID_ERR_EID, CFE_EVS_ERROR,
                        "%s:Unexpected MsgId 0x%0x on SBN.SubPipe",
                        CFE_CPU_NAME, CFE_SB_GetMsgId(SBMsgPtr));

        }/* end switch */

    }/* end while */

    return ResponseReceived;
}/* end SBN_CheckSubscriptionPipe */

void SBN_ProcessSubFromPeer(uint32 PeerIdx) {
    uint32 FirstOpenSlot;
    uint32 idx;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_ProcessSubFromPeer\n");
    }

    if (PeerIdx == SBN_ERROR) {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR1_EID, CFE_EVS_ERROR,
                "%s:Cannot process Subscription,PeerIdx(%d)OutOfRange",
                 CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    if (SBN.Peer[PeerIdx].SubCnt >= SBN_MAX_SUBS_PER_PEER) {
        CFE_EVS_SendEvent(SBN_SUB_ERR_EID, CFE_EVS_ERROR,
                "%s:Cannot process subscription from %s,max(%d)met.",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if msg id already in the list, ignore */
    if (SBN_CheckPeerSubs4MsgId(&idx, SBN.DataMsgBuf.Sub.MsgId,
            PeerIdx) == SBN_MSGID_FOUND)
    {
        CFE_EVS_SendEvent(SBN_DUP_SUB_EID, CFE_EVS_INFORMATION,
                "%s:subscription from %s ignored,msg 0x%04X already logged",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, SBN.DataMsgBuf.Sub.MsgId);
        return;
    }/* end if */

    /* SubscibeLocal suppresses the subscribption report */
    CFE_SB_SubscribeLocal(SBN.DataMsgBuf.Sub.MsgId, SBN.Peer[PeerIdx].Pipe,
            SBN_DEFAULT_MSG_LIM);

    FirstOpenSlot = SBN.Peer[PeerIdx].SubCnt;

    /* log the subscription in the peer table */
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].MsgId = SBN.DataMsgBuf.Sub.MsgId;
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].Qos = SBN.DataMsgBuf.Sub.Qos;

    SBN.Peer[PeerIdx].SubCnt++;

}/* SBN_ProcessSubFromPeer */

void SBN_ProcessUnsubFromPeer(uint32 PeerIdx) {
    uint32 i;
    uint32 idx;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_ProcessUnsubFromPeer\n");
    }

    if (PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR2_EID, CFE_EVS_ERROR,
                "%s:Cannot process unsubscription,PeerIdx(%d)OutOfRange",
                CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    if (SBN.Peer[PeerIdx].SubCnt == 0)
    {
        CFE_EVS_SendEvent(SBN_NO_SUBS_EID, CFE_EVS_INFORMATION,
                "%s:Cannot process unsubscription from %s,none logged",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name);
        return;
    }/* end if */

    if (SBN_CheckPeerSubs4MsgId(&idx, SBN.DataMsgBuf.Sub.MsgId,
            PeerIdx)==SBN_MSGID_NOT_FOUND)
    {
        CFE_EVS_SendEvent(SBN_SUB_NOT_FOUND_EID, CFE_EVS_INFORMATION,
                "%s:Cannot process unsubscription from %s,msg 0x%04X not found",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, SBN.DataMsgBuf.Sub.MsgId);
        return;
    }/* end if */

    /* remove sub from array for that peer and ... */
    /* shift all subscriptions in higher elements to fill the gap */
    for (i = idx; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        CFE_PSP_MemCpy(&SBN.Peer[PeerIdx].Sub[i], &SBN.Peer[PeerIdx].Sub[i + 1],
                sizeof(SBN_Subs_t));
    }/* end for */

    /* decrement sub cnt */
    SBN.Peer[PeerIdx].SubCnt--;

    /* unsubscribe to the msg id on the peer pipe */
    CFE_SB_UnsubscribeLocal(SBN.DataMsgBuf.Sub.MsgId, SBN.Peer[PeerIdx].Pipe);

}/* SBN_ProcessUnsubFromPeer */

void SBN_ProcessLocalSub(CFE_SB_SubEntries_t *Ptr) {
    uint32 i;
    uint32 index;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_ProcessLocalSub\n");
    }

    if (SBN.LocalSubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_LSC_ERR2_EID, CFE_EVS_ERROR,
                "%s:Local Subscription Ignored for MsgId 0x%04x,max(%d)met",
                CFE_CPU_NAME, Ptr->MsgId, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if (SBN_CheckLocSubs4MsgId(&index, Ptr->MsgId) == SBN_MSGID_FOUND)
    {
        /* check index before writing */
        if (index < SBN.LocalSubCnt)
        {
            SBN.LocalSubs[index].InUseCtr++;
        }/* end if */
        return;
    }/* end if */

    /* log new entry into LocalSubs array */
    SBN.LocalSubs[SBN.LocalSubCnt].InUseCtr = 1;
    SBN.LocalSubs[SBN.LocalSubCnt].MsgId = Ptr->MsgId;
    SBN.LocalSubs[SBN.LocalSubCnt].Qos = Ptr->Qos;
    SBN.LocalSubCnt++;

    /* initialize the network msg */
    SBN.DataMsgBuf.Hdr.Type = SBN_SUBSCRIBE_MSG;
    strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);
    SBN.DataMsgBuf.Sub.MsgId = Ptr->MsgId;
    SBN.DataMsgBuf.Sub.Qos = Ptr->Qos;

    /* send subscription to all peers if peer state is heartbeating */
    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].State == SBN_HEARTBEATING)
        {
            SBN_SendNetMsg(SBN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), i, NULL, SBN_FALSE);
        }/* end if */
    }/* end for */

}/* end SBN_ProcessLocalSub */

void SBN_ProcessLocalUnsub(CFE_SB_SubEntries_t *Ptr) {
    uint32 i;
    uint32 index;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_ProcessLocalUnsub\n");
    }

    /* find index of matching subscription */
    if (SBN_CheckLocSubs4MsgId(&index, Ptr->MsgId) == SBN_MSGID_NOT_FOUND)
        return;

    SBN.LocalSubs[index].InUseCtr--;

    /* do not modify the array and tell peers until the # of local subscriptions = 0 */
    if (SBN.LocalSubs[index].InUseCtr != 0)
        return;

    /* move array up one */
    for (i = index; i < SBN.LocalSubCnt; i++)
    {
        SBN.LocalSubs[i].InUseCtr = SBN.LocalSubs[i + 1].InUseCtr;
        SBN.LocalSubs[i].MsgId = SBN.LocalSubs[i + 1].MsgId;
        SBN.LocalSubs[i].Qos = SBN.LocalSubs[i + 1].Qos;
    }/* end for */

    SBN.LocalSubCnt--;

    /* initialize the network msg */
    SBN.DataMsgBuf.Hdr.Type = SBN_UN_SUBSCRIBE_MSG;
    strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);
    SBN.DataMsgBuf.Sub.MsgId = Ptr->MsgId;
    SBN.DataMsgBuf.Sub.Qos = Ptr->Qos;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].State == SBN_HEARTBEATING)
        {
            SBN_SendNetMsg(SBN_UN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), i,
                    NULL, SBN_FALSE);
        }/* end if */
    }/* end for */

}/* end SBN_ProcessLocalUnsub */

void SBN_ProcessAllSubscriptions(CFE_SB_PrevSubMsg_t *Ptr) {
    uint32 i;
    CFE_SB_SubEntries_t SubEntry;
    CFE_SB_SubEntries_t *SubEntryPtr = &SubEntry;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_ProcessAllSubscriptions\n");
    }

    if (Ptr->Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        CFE_EVS_SendEvent(SBN_ENTRY_ERR_EID, CFE_EVS_ERROR,
                "%s:Entries value %d in SB PrevSubMsg exceeds max %d, aborting",
                CFE_CPU_NAME, Ptr->Entries, CFE_SB_SUB_ENTRIES_PER_PKT);
        return;
    }/* end if */

    for (i = 0; i < Ptr->Entries; i++)
    {
        SubEntry.MsgId = Ptr->Entry[i].MsgId;
        SubEntry.Pipe = Ptr->Entry[i].Pipe;
        SubEntry.Qos = Ptr->Entry[i].Qos;

        SBN_ProcessLocalSub(SubEntryPtr);
    }/* end for */

}/* end SBN_ProcessAllSubscriptions */

int32 SBN_CheckLocSubs4MsgId(uint32 *IdxPtr, CFE_SB_MsgId_t MsgId) {
    uint32 i;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckLocSubs4MsgId\n");
    }

    for (i = 0; i < SBN.LocalSubCnt; i++)
    {
        if (SBN.LocalSubs[i].MsgId == MsgId)
        {
            *IdxPtr = i;
            return SBN_MSGID_FOUND;
        }/* end if */
    }/* end for */

    return SBN_MSGID_NOT_FOUND;

}/* end SBN_CheckLocSubs4MsgId */

int32 SBN_CheckPeerSubs4MsgId(uint32 *SubIdxPtr, CFE_SB_MsgId_t MsgId,
        uint32 PeerIdx) {
    uint32 i;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckPeerSubs4MsgId\n");
    }

    for (i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        if (SBN.Peer[PeerIdx].Sub[i].MsgId == MsgId)
        {
            *SubIdxPtr = i;
            return SBN_MSGID_FOUND;
        }/* end if */
    }/* end for */

    return SBN_MSGID_NOT_FOUND;

}/* end SBN_CheckPeerSubs4MsgId */

void SBN_RemoveAllSubsFromPeer(uint32 PeerIdx) {
    uint32 i;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_RemoveAllSubsFromPeer\n");
    }

    if (PeerIdx == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR3_EID, CFE_EVS_ERROR,
                "%s:Cannot remove all subs from peer,PeerIdx(%d)OutOfRange",
                CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    for (i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        CFE_SB_UnsubscribeLocal(SBN.Peer[PeerIdx].Sub[i].MsgId,
                SBN.Peer[PeerIdx].Pipe);
    }/* end for */

    CFE_EVS_SendEvent(SBN_UNSUB_CNT_EID, CFE_EVS_INFORMATION,
            "%s:UnSubscribed %d MsgIds from %s", CFE_CPU_NAME,
            SBN.Peer[PeerIdx].SubCnt, SBN.Peer[PeerIdx].Name);

    SBN.Peer[PeerIdx].SubCnt = 0;

}/* end SBN_RemoveAllSubsFromPeer */

void SBN_ShowPeerSubs(uint32 PeerIdx) {
    uint32 i;

    OS_printf("\n%s:PeerIdx %d, Name %s, State %s\n", CFE_CPU_NAME, PeerIdx,
            SBN.Peer[PeerIdx].Name, SBN_LIB_StateNum2Str(SBN.Peer[PeerIdx].State));
    OS_printf("%s:Rcvd %d subscriptions from %s\n", CFE_CPU_NAME,
            SBN.Peer[PeerIdx].SubCnt, SBN.Peer[PeerIdx].Name);

    for (i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        OS_printf("0x%04X  ", SBN.Peer[PeerIdx].Sub[i].MsgId);
        if (((i + 1) % 5) == 0)
            OS_printf("\n");
    }/* end for */

}/* end SBN_ShowPeerSubs */

void SBN_ShowLocSubs(void) {
    uint32 i;

    OS_printf("\n%s:%d Local Subscriptions\n", CFE_CPU_NAME, SBN.LocalSubCnt);
    for (i = 0; i < SBN.LocalSubCnt; i++)
    {
        OS_printf("0x%04X  ", SBN.LocalSubs[i].MsgId);
        if (((i + 1) % 5) == 0)
            OS_printf("\n");
    }/* end for */

}/* end SBN_ShowLocSubs */

void SBN_ShowAllSubs(void) {
    uint32 i;

    SBN_ShowLocSubs();
    OS_printf("\n");
    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].InUse == SBN_IN_USE)
        {
            SBN_ShowPeerSubs(i);
        }/* end if */
    }/* end for */
}/* end SBN_ShowAllSubs */


