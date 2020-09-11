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
#include "sbn_pack.h"

// TODO: instead of using void * for the buffer for SBN messages, use
// a struct that has the SBN header in packed bytes.

/**
 * Informs the software bus to send this application all subscription requests.
 */
SBN_Status_t SBN_SendSubsRequests(void)
{
    CFE_Status_t    CFE_Status = CFE_SUCCESS;
    CFE_SB_CmdHdr_t SBCmdMsg;

    /* Turn on SB subscription reporting */
    CFE_SB_InitMsg(&SBCmdMsg, CFE_SB_SUB_RPT_CTRL_MID, sizeof(CFE_SB_CmdHdr_t), true);
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t)&SBCmdMsg, CFE_SB_ENABLE_SUB_REPORTING_CC);
    CFE_Status = CFE_SB_SendMsg((CFE_SB_MsgPtr_t)&SBCmdMsg);
    if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_SUB_EID, "Unable to turn on sub reporting (status=%d)", CFE_Status);
        return SBN_ERROR;
    } /* end if */

    /* Request a list of previous subscriptions from SB */
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t)&SBCmdMsg, CFE_SB_SEND_PREV_SUBS_CC);
    CFE_Status = CFE_SB_SendMsg((CFE_SB_MsgPtr_t)&SBCmdMsg);
    if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_SUB_EID, "Unable to send prev subs request (status=%d)", CFE_Status);
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
} /* end SBN_SendSubsRequests */

/**
 * \brief Sends a local subscription over the wire to a peer.
 *
 * @param[in] SubType Whether this is a subscription or unsubscription.
 * @param[in] MsgID The CCSDS message ID being (un)subscribed.
 * @param[in] QoS The CCSDS quality of service being (un)subscribed.
 * @param[in] Peer The Peer interface
 */
static SBN_Status_t SendLocalSubToPeer(int SubType, CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t QoS, SBN_PeerInterface_t *Peer)
{
    uint8  Buf[SBN_PACKED_SUB_SZ];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, SBN_PACKED_SUB_SZ, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);

    Pack_MsgID(&Pack, MsgID);
    Pack_Data(&Pack, &QoS, sizeof(QoS)); /* 2 uint8's */

    return SBN_SendNetMsg(SubType, Pack.BufUsed, Buf, Peer);
} /* end SendLocalSubToPeer */

/**
 * \brief Sends all local subscriptions over the wire to a peer.
 *
 * @param[in] Peer The peer interface.
 */
SBN_Status_t SBN_SendLocalSubsToPeer(SBN_PeerInterface_t *Peer)
{
    uint8  Buf[SBN_PACKED_SUB_SZ];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, SBN_PACKED_SUB_SZ, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, SBN.SubCnt);

    int i = 0;
    for (i = 0; i < SBN.SubCnt; i++)
    {
        Pack_MsgID(&Pack, SBN.Subs[i].MsgID);
        /* 2 uint8's */
        Pack_Data(&Pack, &SBN.Subs[i].QoS, sizeof(SBN.Subs[i].QoS));
    } /* end for */

    return SBN_SendNetMsg(SBN_SUB_MSG, Pack.BufUsed, Buf, Peer);
} /* end SBN_SendLocalSubsToPeer */

/**
 * Utility to find the subscription index (SBN.Subs)
 * that is subscribed to the CCSDS message ID.
 *
 * @param[out] IdxPtr The subscription index found.
 * @param[in] MsgID The CCSDS message ID of the subscription being sought.
 * @return true if found.
 */
static int IsMsgIDSub(int *IdxPtr, CFE_SB_MsgId_t MsgID)
{
    int i = 0;

    for (i = 0; i < SBN.SubCnt; i++)
    {
        if (SBN.Subs[i].MsgID == MsgID)
        {
            if (IdxPtr)
            {
                *IdxPtr = i;
            } /* end if */

            return true;
        } /* end if */
    }     /* end for */

    return false;
} /* end IsMsgIDSub */

/**
 * \brief Is this peer subscribed to this message ID? If so, what is the index
 *        of the subscription?
 *
 * @param[out] SubIdxPtr The pointer to the subscription index value.
 * @param[in] MsgID The CCSDS message ID of the subscription being sought.
 * @param[in] Peer The peer interface.
 *
 * @return true if found.
 */
static int IsPeerSubMsgID(int *SubIdxPtr, CFE_SB_MsgId_t MsgID, SBN_PeerInterface_t *Peer)
{
    int i = 0;

    for (i = 0; i < Peer->SubCnt; i++)
    {
        if (Peer->Subs[i].MsgID == MsgID)
        {
            *SubIdxPtr = i;
            return true;
        } /* end if */
    }     /* end for */

    return false;

} /* end IsPeerSubMsgID */

/**
 * \brief I have seen a local subscription, send it on to peers if this is the
 * first instance of a subscription for this message ID.
 *
 * @param[in] MsgID The CCSDS Message ID of the local subscription.
 * @param[in] QoS The CCSDS quality of service of the local subscription.
 */
static SBN_Status_t ProcessLocalSub(CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t QoS)
{
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    /* don't send event messages */
    if (MsgID == CFE_EVS_LONG_EVENT_MSG_MID)
        return SBN_SUCCESS;

    /* don't send SBN messages */
    if (MsgID == SBN_CMD_MID || MsgID == SBN_TLM_MID)
        return SBN_SUCCESS;

    int SubIdx = 0;

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if (IsMsgIDSub(&SubIdx, MsgID))
    {
        SBN.Subs[SubIdx].InUseCtr++;
        /* does not send to peers, as they already know */
        return SBN_SUCCESS;
    } /* end if */

    if (SBN.SubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        EVSSendErr(SBN_SUB_EID, "local subscription ignored for MsgID 0x%04X, max (%d) met", ntohs(MsgID),
                   SBN_MAX_SUBS_PER_PEER);
        return SBN_ERROR;
    } /* end if */

    /* log new entry into Subs array */
    SBN.Subs[SBN.SubCnt].InUseCtr = 1;
    SBN.Subs[SBN.SubCnt].MsgID    = MsgID;
    SBN.Subs[SBN.SubCnt].QoS      = QoS;
    SBN.SubCnt++;

    int NetIdx = 0, PeerIdx = 0;
    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            SBN_Status = SendLocalSubToPeer(SBN_SUB_MSG, MsgID, QoS, Peer);

            if (SBN_Status != SBN_SUCCESS)
            {
                return SBN_Status;
            } /* end if */
        }     /* end for */
    }         /* end for */

    return SBN_Status;
} /* end ProcessLocalSub */

/**
 * \brief I have seen a local unsubscription, send it on to peers if this is the
 * last instance of a subscription for this message ID.
 *
 * @param[in] MsgID The CCSDS Message ID of the local unsubscription.
 * @param[in] QoS The CCSDS quality of service of the local unsubscription.
 */
static SBN_Status_t ProcessLocalUnsub(CFE_SB_MsgId_t MsgID)
{
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    int          SubIdx;

    /* find idx of matching subscription */
    if (!IsMsgIDSub(&SubIdx, MsgID))
    {
        return SBN_SUCCESS; /* or should this be error? */
    }                       /* end if */

    SBN.Subs[SubIdx].InUseCtr--;

    /* do not modify the array and tell peers
    ** until the # of local subscriptions = 0
    */
    if (SBN.Subs[SubIdx].InUseCtr > 0)
    {
        return SBN_SUCCESS;
    } /* end if */

    /* remove sub from array for and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the Subs[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for (; SubIdx < SBN.SubCnt; SubIdx++)
    {
        memcpy(&SBN.Subs[SubIdx], &SBN.Subs[SubIdx + 1], sizeof(SBN_Subs_t));
    } /* end for */

    SBN.SubCnt--;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    int NetIdx = 0, PeerIdx = 0;
    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            SBN_Status = SendLocalSubToPeer(SBN_UNSUB_MSG, SBN.Subs[PeerIdx].MsgID, SBN.Subs[PeerIdx].QoS, Peer);

            if (SBN_Status != SBN_SUCCESS)
            {
                return SBN_Status;
            } /* end if */
        }     /* end for */
    }         /* end for */

    return SBN_SUCCESS;
} /* end ProcessLocalUnsub */

/**
 * \brief Check the local pipe for subscription messages. Send them on to peers
 *        if there are any (new)subscriptions.
 *
 * @return SBN_SUCCESS if subscriptions received, SBN_IF_EMPTY if no subs recvd, SBN_ERROR otherwise.
 */
SBN_Status_t SBN_CheckSubscriptionPipe(void)
{
    CFE_Status_t CFE_Status = CFE_SUCCESS;

    CFE_SB_MsgPtr_t                 SBMsgPtr;
    CFE_SB_SingleSubscriptionTlm_t *SubRprtMsgPtr;

    CFE_Status = CFE_SB_RcvMsg(&SBMsgPtr, SBN.SubPipe, CFE_SB_POLL);
    switch (CFE_Status)
    {
        case CFE_SUCCESS:
            SubRprtMsgPtr = (CFE_SB_SingleSubscriptionTlm_t *)SBMsgPtr;

            switch (CFE_SB_GetMsgId(SBMsgPtr))
            {
                case CFE_SB_ONESUB_TLM_MID:
                    switch (SubRprtMsgPtr->Payload.SubType)
                    {
                        case CFE_SB_SUBSCRIPTION:
                            return ProcessLocalSub(SubRprtMsgPtr->Payload.MsgId, SubRprtMsgPtr->Payload.Qos);
                        case CFE_SB_UNSUBSCRIPTION:
                            return ProcessLocalUnsub(SubRprtMsgPtr->Payload.MsgId);
                        default:
                            EVSSendErr(SBN_SUB_EID, "unexpected subscription type (%d) in SBN_CheckSubscriptionPipe",
                                       SubRprtMsgPtr->Payload.SubType);
                            return SBN_ERROR;
                    } /* end switch */
                    break;

                case CFE_SB_ALLSUBS_TLM_MID:
                    return SBN_ProcessAllSubscriptions((CFE_SB_AllSubscriptionsTlm_t *)SBMsgPtr);

                default:
                    EVSSendErr(SBN_MSG_EID, "unexpected message id (0x%04X) on SBN.SubPipe",
                               ntohs(CFE_SB_GetMsgId(SBMsgPtr)));
                    return SBN_ERROR;
            } /* end switch */

            break;
        case CFE_SB_NO_MESSAGE:
            return SBN_IF_EMPTY;
        default:
            EVSSendErr(SBN_MSG_EID, "err from rcvmsg on sub pipe");
            return SBN_ERROR;
    } /* end switch */

    return SBN_ERROR;
} /* end SBN_CheckSubscriptionPipe */

/**
 * \brief Record keep the subscription locally so that when we no longer have any peers subscribed
 *        to this MID, I unsubscribe from the MID.
 *
 * @param[in] Peer The peer interface.
 * @param[in] MsgID The subscription SBN message ID.
 * @param[in] QoS The subscription quality of service.
 *
 * @return SBN_SUCCESS on successfully adding the sub to my records, otherwise SBN_ERROR
 */
static SBN_Status_t AddSub(SBN_PeerInterface_t *Peer, CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t QoS)
{
    int          idx        = 0;
    CFE_Status_t CFE_Status = SBN_SUCCESS;

    /* if msg id already in the list, ignore */
    if (IsPeerSubMsgID(&idx, MsgID, Peer))
    {
        return SBN_SUCCESS;
    } /* end if */

    if (Peer->SubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        EVSSendErr(SBN_SUB_EID, "cannot process subscription from ProcessorID %d, max (%d) met", Peer->ProcessorID,
                   SBN_MAX_SUBS_PER_PEER);
        return SBN_ERROR;
    } /* end if */

    /* SubscribeLocal suppresses the subscription report */
    CFE_Status = CFE_SB_SubscribeLocal(MsgID, Peer->Pipe, SBN_DEFAULT_MSG_LIM);
    if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_SUB_EID, "unable to subscribe to MID 0x%04X", MsgID);
        return SBN_ERROR;
    } /* end if */

    /* log the subscription in the peer table */
    Peer->Subs[Peer->SubCnt].MsgID = MsgID;
    Peer->Subs[Peer->SubCnt].QoS   = QoS;

    Peer->SubCnt++;

    return SBN_SUCCESS;
} /* end AddSub */

/**
 * \brief Process a subscription from a peer.
 *
 * @param[in] Peer The peer interface.
 * @param[in] MsgID The subscription SBN message ID.
 * @param[in] QoS The subscription quality of service.
 *
 * @return SBN_SUCCESS on successfully handling subscription from peer, otherwise SBN_ERROR
 */
static SBN_Status_t ProcessSubFromPeer(SBN_PeerInterface_t *Peer, CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t QoS)
{
    SBN_ModuleIdx_t  FilterIdx;
    SBN_Filter_Ctx_t Filter_Context;
    SBN_Status_t     SBN_Status;

    Filter_Context.MyProcessorID   = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID  = CFE_PSP_GetSpacecraftId();
    Filter_Context.PeerProcessorID = Peer->ProcessorID;
    Filter_Context.MySpacecraftID  = Peer->SpacecraftID;

    for (FilterIdx = 0; FilterIdx < Peer->FilterCnt; FilterIdx++)
    {
        if (Peer->Filters[FilterIdx]->RemapMID == NULL)
        {
            continue;
        } /* end if */

        SBN_Status = (Peer->Filters[FilterIdx]->RemapMID)(&MsgID, &Filter_Context);

        if (SBN_Status != SBN_SUCCESS)
        {
            return SBN_Status;
        } /* end if */
    }     /* end for */

    return AddSub(Peer, MsgID, QoS);
} /* ProcessSubFromPeer */

/**
 * \brief Process a subscription message from a peer.
 *
 * @param[in] PeerIdx The peer index (in SBN.Peer)
 * @param[in] Msg The subscription SBN message.
 *
 * @return SBN_SUCCESS on successfully handling all subscriptions from peer, otherwise SBN_ERROR
 */
SBN_Status_t SBN_ProcessSubsFromPeer(SBN_PeerInterface_t *Peer, void *Msg)
{
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    Pack_t       Pack;
    char         VersionHash[SBN_IDENT_LEN];

    Pack_Init(&Pack, Msg, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);

    Unpack_Data(&Pack, VersionHash, SBN_IDENT_LEN);

    if (strncmp(VersionHash, SBN_IDENT, SBN_IDENT_LEN))
    {
        EVSSendErr(SBN_PROTO_EID, "version number mismatch with peer CpuID %d", Peer->ProcessorID);
        return SBN_ERROR;
    }

    uint16 SubCnt;
    Unpack_UInt16(&Pack, &SubCnt);

    int SubIdx = 0;
    for (SubIdx = 0; SubIdx < SubCnt; SubIdx++)
    {
        CFE_SB_MsgId_t MsgID;
        Unpack_MsgID(&Pack, &MsgID);
        CFE_SB_Qos_t QoS;
        Unpack_Data(&Pack, &QoS, sizeof(QoS));

        SBN_Status = ProcessSubFromPeer(Peer, MsgID, QoS);

        if (SBN_Status != SBN_SUCCESS)
        {
            return SBN_Status;
        } /* end if */
    }     /* end for */

    return SBN_SUCCESS;
} /* SBN_ProcessSubsFromPeer */

/**
 * \brief Process an unsubscription message from a peer.
 *
 * @param[in] Peer The peer interface
 * @param[in] MsgID The unsubscription SBN message ID.
 *
 * @return SBN_SUCCESS on successful unsubscription from peer, otherwise SBN_ERROR
 */
static SBN_Status_t ProcessUnsubFromPeer(SBN_PeerInterface_t *Peer, CFE_SB_MsgId_t MsgID)
{
    SBN_ModuleIdx_t  FilterIdx;
    SBN_Filter_Ctx_t Filter_Context;
    SBN_Status_t     SBN_Status;

    int i = 0, idx = 0;

    Filter_Context.MyProcessorID   = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID  = CFE_PSP_GetSpacecraftId();
    Filter_Context.PeerProcessorID = Peer->ProcessorID;
    Filter_Context.MySpacecraftID  = Peer->SpacecraftID;

    for (FilterIdx = 0; FilterIdx < Peer->FilterCnt; FilterIdx++)
    {
        if (Peer->Filters[FilterIdx]->RemapMID == NULL)
        {
            continue;
        } /* end if */

        SBN_Status = (Peer->Filters[FilterIdx]->RemapMID)(&MsgID, &Filter_Context);

        if (SBN_Status != SBN_SUCCESS)
        {
            /* assume the filter generated an event */
            return SBN_Status;
        } /* end if */
    }     /* end for */

    if (!IsPeerSubMsgID(&idx, MsgID, Peer))
    {
        EVSSendInfo(SBN_SUB_EID, "cannot process unsubscription from ProcessorID %d, msg 0x%04X not found",
                    Peer->ProcessorID, MsgID);
        return SBN_SUCCESS;
    } /* end if */

    /* remove sub from array for that peer and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the Subs[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for (i = idx; i < Peer->SubCnt; i++)
    {
        memcpy(&Peer->Subs[i], &Peer->Subs[i + 1], sizeof(SBN_Subs_t));
    } /* end for */

    /* decrement sub cnt */
    Peer->SubCnt--;

    /* unsubscribe to the msg id on the peer pipe */
    if (CFE_SB_UnsubscribeLocal(MsgID, Peer->Pipe) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_SUB_EID, "unable to unsubscribe from MID 0x%04X", MsgID);
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
} /* end ProcessUnsubFromPeer */

/**
 * \brief Process an unsubscription message from a peer.
 *
 * @param[in] PeerIdx The peer index (in SBN.Peer)
 * @param[in] Msg The unsubscription SBN message.
 *
 * @return SBN_SUCCESS always (whether or not there were some unsubs that failed.)
 */
SBN_Status_t SBN_ProcessUnsubsFromPeer(SBN_PeerInterface_t *Peer, void *Msg)
{
    Pack_t Pack;

    Pack_Init(&Pack, Msg, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);

    char VersionHash[SBN_IDENT_LEN];

    Unpack_Data(&Pack, VersionHash, SBN_IDENT_LEN);

    if (strncmp(VersionHash, SBN_IDENT, SBN_IDENT_LEN))
    {
        EVSSendInfo(SBN_PROTO_EID, "version number mismatch with peer CpuID %d", Peer->ProcessorID);
    }

    uint16 SubCnt;
    Unpack_UInt16(&Pack, &SubCnt);

    int SubIdx = 0;
    for (SubIdx = 0; SubIdx < SubCnt; SubIdx++)
    {
        CFE_SB_MsgId_t MsgID;
        Unpack_MsgID(&Pack, &MsgID);
        CFE_SB_Qos_t QoS;
        Unpack_Data(&Pack, &QoS, sizeof(QoS));

        ProcessUnsubFromPeer(Peer, MsgID); /* ignore return value, I want to unsub as much as I can */
    }                                      /* end for */

    return SBN_SUCCESS;
} /* end SBN_ProcessUnsubsFromPeer() */

/**
 * When SBN starts, it queries for all existing subscriptions. This method
 * processes those subscriptions.
 *
 * @param[in] Ptr SB message pointer.
 *
 * @return SBN_SUCCESS on successful subscriptions, otherwise SBN_ERROR.
 */
SBN_Status_t SBN_ProcessAllSubscriptions(CFE_SB_AllSubscriptionsTlm_t *Ptr)
{
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    int          i          = 0;

    if (Ptr->Payload.Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        EVSSendErr(SBN_SUB_EID, "entries value %d in SB PrevSubMsg exceeds max %d, aborting", (int)Ptr->Payload.Entries,
                   CFE_SB_SUB_ENTRIES_PER_PKT);
        return SBN_ERROR;
    } /* end if */

    for (i = 0; i < Ptr->Payload.Entries; i++)
    {
        SBN_Status = ProcessLocalSub(Ptr->Payload.Entry[i].MsgId, Ptr->Payload.Entry[i].Qos);
        if (SBN_Status != SBN_SUCCESS)
        {
            return SBN_Status;
        } /* end if */
    }     /* end for */

    return SBN_Status;
} /* end SBN_ProcessAllSubscriptions */

/**
 * Removes all subscriptions (unsubscribe from the local SB )
 * for the specified peer, particularly when the peer connection has been lost.
 *
 * @param[in] PeerIdx The peer index (into SBN.Peer) to clear.
 *
 * @return SBN_SUCCESS always (whether or not there were some unsubs that failed.)
 */
SBN_Status_t SBN_RemoveAllSubsFromPeer(SBN_PeerInterface_t *Peer)
{
    int          i          = 0;
    CFE_Status_t CFE_Status = CFE_SUCCESS;

    for (i = 0; i < Peer->SubCnt; i++)
    {
        CFE_Status = CFE_SB_UnsubscribeLocal(Peer->Subs[i].MsgID, Peer->Pipe);
        if (CFE_Status != CFE_SUCCESS)
        {
            EVSSendErr(SBN_SUB_EID, "unable to unsubscribe from message id 0x%04X", Peer->Subs[i].MsgID);
            /* but continue processing... */
        } /* end if */
    }     /* end for */

    EVSSendInfo(SBN_SUB_EID, "unsubscribed %d message id's from ProcessorID %d", (int)Peer->SubCnt, Peer->ProcessorID);

    Peer->SubCnt = 0;

    return SBN_SUCCESS;
} /* end SBN_RemoveAllSubsFromPeer */
