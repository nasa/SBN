/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include "sbn_app.h"
#include <string.h>
#include <arpa/inet.h>
#include "cfe_msgids.h"
#include "sbn_pack.h"
#include "sbn_error.h"

// TODO: instead of using void * for the buffer for SBN messages, use
// a struct that has the SBN header in packed bytes.

/**
 * Informs the software bus to send this application all subscription requests.
 */
SBN_Status_t SBN_SendSubsRequests(void)
{
    CFE_Status_t            CFE_Status;
    CFE_MSG_CommandHeader_t CmdMsg;
    static CFE_SB_MsgId_t   SB_SUB_RPT_CTRL_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(SB_SUB_RPT_CTRL_MID))
    {
        SB_SUB_RPT_CTRL_MID = CFE_SB_ValueToMsgId(CFE_SB_SUB_RPT_CTRL_MID);
    }

    /* Turn on SB subscription reporting */
    CFE_MSG_Init((CFE_MSG_Message_t *)&CmdMsg, SB_SUB_RPT_CTRL_MID, sizeof(CmdMsg));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&CmdMsg, CFE_SB_ENABLE_SUB_REPORTING_CC);
    CFE_Status = CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&CmdMsg, true);
    if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_SUB_EID, "Unable to turn on sub reporting (status=%d)", CFE_Status);
        return SBN_ERROR;
    } /* end if */

    /* Request a list of previous subscriptions from SB */
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&CmdMsg, CFE_SB_SEND_PREV_SUBS_CC);
    CFE_Status = CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&CmdMsg, true);
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

    EVSSendDbg(SBN_PEER_EID, "send local sub to peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
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
    Pack_UInt16(&Pack, SBN_AppData.SubCnt);

    int i = 0;
    for (i = 0; i < SBN_AppData.SubCnt; i++)
    {
        Pack_MsgID(&Pack, SBN_AppData.Subs[i].MsgID);
        /* 2 uint8's */
        Pack_Data(&Pack, &SBN_AppData.Subs[i].QoS, sizeof(SBN_AppData.Subs[i].QoS));
    } /* end for */

    EVSSendDbg(SBN_PEER_EID, "send local subs to peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
    return SBN_SendNetMsg(SBN_SUB_MSG, Pack.BufUsed, Buf, Peer);
} /* end SBN_SendLocalSubsToPeer */

/**
 * Utility to find the subscription index (SBN_AppData.Subs)
 * that is subscribed to the CCSDS message ID.
 *
 * @param[out] IdxPtr The subscription index found.
 * @param[in] MsgID The CCSDS message ID of the subscription being sought.
 * @return true if found.
 */
static int IsMsgIDSub(int *IdxPtr, CFE_SB_MsgId_t MsgID)
{
    int i = 0;

    for (i = 0; i < SBN_AppData.SubCnt; i++)
    {
        if (CFE_SB_MsgId_Equal(SBN_AppData.Subs[i].MsgID, MsgID))
        {
            if (IdxPtr)
            {
                *IdxPtr = i;
            } /* end if */

            return true;
        } /* end if */
    } /* end for */

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
        if (CFE_SB_MsgId_Equal(Peer->Subs[i].MsgID, MsgID))
        {
            *SubIdxPtr = i;
            return true;
        } /* end if */
    } /* end for */

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
    SBN_Status_t          SBN_Status          = SBN_SUCCESS;
    static CFE_SB_MsgId_t LCMD_MID            = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t LHK_TLM_MID         = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t LHKNET_TLM_MID      = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t LHKPEER_TLM_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t LHKMYSUBS_TLM_MID   = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t LHKPEERSUBS_TLM_MID = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t EVENT_MSG_MID       = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(LCMD_MID))
    {
        LCMD_MID            = CFE_SB_ValueToMsgId(SBN_CMD_MID);
        LHK_TLM_MID         = CFE_SB_ValueToMsgId(SBN_HK_TLM_MID);
        LHKNET_TLM_MID      = CFE_SB_ValueToMsgId(SBN_HKNET_TLM_MID);
        LHKPEER_TLM_MID     = CFE_SB_ValueToMsgId(SBN_HKPEER_TLM_MID);
        LHKMYSUBS_TLM_MID   = CFE_SB_ValueToMsgId(SBN_HKMYSUBS_TLM_MID);
        LHKPEERSUBS_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKPEERSUBS_TLM_MID);
        EVENT_MSG_MID       = CFE_SB_ValueToMsgId(CFE_EVS_LONG_EVENT_MSG_MID);
    }

    /* don't send event messages */
    if (CFE_SB_MsgId_Equal(MsgID, EVENT_MSG_MID))
        return SBN_SUCCESS;

    /* don't send SBN messages */
    if (CFE_SB_MsgId_Equal(MsgID, LCMD_MID) || CFE_SB_MsgId_Equal(MsgID, LHK_TLM_MID)
        || CFE_SB_MsgId_Equal(MsgID, LHKNET_TLM_MID) || CFE_SB_MsgId_Equal(MsgID, LHKPEER_TLM_MID)
        || CFE_SB_MsgId_Equal(MsgID, LHKMYSUBS_TLM_MID) || CFE_SB_MsgId_Equal(MsgID, LHKPEERSUBS_TLM_MID))
        return SBN_SUCCESS;

    int SubIdx = 0;

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if (IsMsgIDSub(&SubIdx, MsgID))
    {
        SBN_AppData.Subs[SubIdx].InUseCtr++;
        EVSSendDbg(SBN_SUB_EID, "local sub already exists: in use: %d", SBN_AppData.Subs[SubIdx].InUseCtr);
        /* does not send to peers, as they already know */
        return SBN_SUCCESS;
    } /* end if */

    if (SBN_AppData.SubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        EVSSendErr(SBN_SUB_EID,
                   "local subscription ignored for MsgID 0x%04X, max (%d) met",
                   CFE_SB_MsgIdToValue(MsgID),
                   SBN_MAX_SUBS_PER_PEER);
        return SBN_ERROR;
    } /* end if */

    /* log new entry into Subs array */
    SBN_AppData.Subs[SBN_AppData.SubCnt].InUseCtr = 1;
    SBN_AppData.Subs[SBN_AppData.SubCnt].MsgID    = MsgID;
    SBN_AppData.Subs[SBN_AppData.SubCnt].QoS      = QoS;
    SBN_AppData.SubCnt++;

    int NetIdx = 0, PeerIdx = 0;
    for (NetIdx = 0; NetIdx < SBN_AppData.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN_AppData.Nets[NetIdx];
        for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            EVSSendDbg(SBN_PEER_EID,
                       "process local sub for MID %#04x sending to %d:%d",
                       CFE_SB_MsgIdToValue(MsgID),
                       Peer->SpacecraftID,
                       Peer->ProcessorID);
            SBN_Status = SendLocalSubToPeer(SBN_SUB_MSG, MsgID, QoS, Peer);

            if (SBN_Status != SBN_SUCCESS)
            {
                return SBN_Status;
            } /* end if */
        } /* end for */
    } /* end for */

    return SBN_Status;
} /* end ProcessLocalSub */

/**
 * \brief I have seen a local unsubscription, send it on to peers if this is the
 * last instance of a subscription for this message ID.
 *
 * @param[in] MsgID The CCSDS Message ID of the local unsubscription.
 */
static SBN_Status_t ProcessLocalUnsub(CFE_SB_MsgId_t MsgID)
{
    SBN_Status_t SBN_Status;
    int          SubIdx;

    /* find idx of matching subscription */
    if (!IsMsgIDSub(&SubIdx, MsgID))
    {
        return SBN_SUCCESS; /* or should this be error? */
    } /* end if */

    SBN_AppData.Subs[SubIdx].InUseCtr--;

    /* do not modify the array and tell peers
    ** until the # of local subscriptions = 0
    */
    if (SBN_AppData.Subs[SubIdx].InUseCtr > 0)
    {
        return SBN_SUCCESS;
    } /* end if */

    /* remove sub from array for and
    ** shift all subscriptions in higher elements to fill the gap
    ** note that the Subs[] array has one extra element to allow for an
    ** unsub from a full table.
    */
    for (; SubIdx < SBN_AppData.SubCnt; SubIdx++)
    {
        memcpy(&SBN_AppData.Subs[SubIdx], &SBN_AppData.Subs[SubIdx + 1], sizeof(SBN_Subs_t));
    } /* end for */

    SBN_AppData.SubCnt--;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    int NetIdx = 0, PeerIdx = 0;
    for (NetIdx = 0; NetIdx < SBN_AppData.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN_AppData.Nets[NetIdx];
        for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            EVSSendInfo(SBN_PEER_EID, "process local unsub %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
            SBN_Status =
                SendLocalSubToPeer(SBN_UNSUB_MSG, SBN_AppData.Subs[PeerIdx].MsgID, SBN_AppData.Subs[PeerIdx].QoS, Peer);

            if (SBN_Status != SBN_SUCCESS)
            {
                return SBN_Status;
            } /* end if */
        } /* end for */
    } /* end for */

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

    CFE_SB_AllSubscriptionsTlm_t   *MsgPtr       = NULL; /* largest message format */
    CFE_SB_SingleSubscriptionTlm_t *SingleMsgPtr = NULL; /* utility "cast" */
    CFE_SB_MsgId_t                  MsgId        = CFE_SB_INVALID_MSG_ID;
    static CFE_SB_MsgId_t           SB_ONESUB_TLM_MID  = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t           SB_ALLSUBS_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(SB_ONESUB_TLM_MID))
    {
        SB_ONESUB_TLM_MID  = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
        SB_ALLSUBS_TLM_MID = CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID);
    }

    CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&MsgPtr, SBN_AppData.SubPipe, CFE_SB_POLL);
    switch (CFE_Status)
    {
        case CFE_SUCCESS:

            if (CFE_MSG_GetMsgId((CFE_MSG_Message_t *)MsgPtr, &MsgId) != CFE_SUCCESS)
            {
                EVSSendErr(SBN_MSG_EID, "unable to get message id");
                return SBN_ERROR;
            }

            if (CFE_SB_MsgId_Equal(MsgId, SB_ONESUB_TLM_MID))
            {
                SingleMsgPtr = (CFE_SB_SingleSubscriptionTlm_t *)MsgPtr;
                switch (SingleMsgPtr->Payload.SubType)
                {
                    case CFE_SB_SUBSCRIPTION:
                        return ProcessLocalSub(SingleMsgPtr->Payload.MsgId, SingleMsgPtr->Payload.Qos);
                    case CFE_SB_UNSUBSCRIPTION:
                        return ProcessLocalUnsub(SingleMsgPtr->Payload.MsgId);
                    default:
                        EVSSendErr(SBN_SUB_EID,
                                   "unexpected subscription type (%d) in SBN_CheckSubscriptionPipe",
                                   SingleMsgPtr->Payload.SubType);
                        return SBN_ERROR;
                } /* end switch */
            }
            else if (CFE_SB_MsgId_Equal(MsgId, SB_ALLSUBS_TLM_MID))
            {
                return SBN_ProcessAllSubscriptions(MsgPtr);
            }
            else
            {
                EVSSendErr(SBN_MSG_EID,
                           "unexpected message id (0x%04X) on SBN_AppData.SubPipe",
                           CFE_SB_MsgIdToValue(MsgId));
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
        EVSSendErr(SBN_SUB_EID,
                   "cannot process subscription from ProcessorID %d, max (%d) met",
                   Peer->ProcessorID,
                   SBN_MAX_SUBS_PER_PEER);
        return SBN_ERROR;
    } /* end if */

    /* SubscribeLocal suppresses the subscription report */
    CFE_Status = CFE_SB_SubscribeLocal(MsgID, Peer->Pipe, SBN_DEFAULT_MSG_LIM);
    if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_SUB_EID, "unable to subscribe to MID 0x%04X", CFE_SB_MsgIdToValue(MsgID));
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

    Filter_Context.MyProcessorID    = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID   = CFE_PSP_GetSpacecraftId();
    Filter_Context.PeerProcessorID  = Peer->ProcessorID;
    Filter_Context.PeerSpacecraftID = Peer->SpacecraftID;

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
    } /* end for */

    return AddSub(Peer, MsgID, QoS);
} /* ProcessSubFromPeer */

/**
 * \brief Process a subscription message from a peer.
 *
 * @param[in] Peer The peer interface
 * @param[in] Msg The subscription SBN message.
 *
 * @return SBN_SUCCESS on successfully handling all subscriptions from peer, otherwise SBN_ERROR
 */
SBN_Status_t SBN_ProcessSubsFromPeer(SBN_PeerInterface_t *Peer, void *Msg)
{
    SBN_Status_t SBN_Status;
    Pack_t       Pack;
    char         VersionHash[SBN_IDENT_LEN];

    Pack_Init(&Pack, Msg, CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);

    Unpack_Data(&Pack, VersionHash, SBN_IDENT_LEN);

    if (strncmp(VersionHash, SBN_IDENT, SBN_IDENT_LEN))
    {
        EVSSendErr(SBN_PROTO_EID, "version number mismatch with peer CpuID %d", Peer->ProcessorID);
        return SBN_ERROR;
    }

    uint16 SubCnt = 0;
    Unpack_UInt16(&Pack, &SubCnt);

    int SubIdx = 0;
    for (SubIdx = 0; SubIdx < SubCnt; SubIdx++)
    {
        CFE_SB_MsgId_t MsgID = CFE_SB_INVALID_MSG_ID;
        Unpack_MsgID(&Pack, &MsgID);
        CFE_SB_Qos_t QoS = { 0 };
        Unpack_Data(&Pack, &QoS, sizeof(QoS));

        SBN_Status = ProcessSubFromPeer(Peer, MsgID, QoS);

        if (SBN_Status != SBN_SUCCESS)
        {
            return SBN_Status;
        } /* end if */
    } /* end for */

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
    CFE_Status_t     CFE_Status;
    SBN_ModuleIdx_t  FilterIdx;
    SBN_Filter_Ctx_t Filter_Context;
    SBN_Status_t     SBN_Status;

    int i = 0, idx = 0;

    Filter_Context.MyProcessorID    = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID   = CFE_PSP_GetSpacecraftId();
    Filter_Context.PeerProcessorID  = Peer->ProcessorID;
    Filter_Context.PeerSpacecraftID = Peer->SpacecraftID;

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
    } /* end for */

    if (!IsPeerSubMsgID(&idx, MsgID, Peer))
    {
        EVSSendInfo(SBN_SUB_EID,
                    "cannot process unsubscription from ProcessorID %d, msg 0x%04X not found",
                    Peer->ProcessorID,
                    CFE_SB_MsgIdToValue(MsgID));
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
    if ((CFE_Status = CFE_SB_UnsubscribeLocal(MsgID, Peer->Pipe)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_SUB_EID, "unable to unsubscribe from MID 0x%04X: %d", CFE_SB_MsgIdToValue(MsgID), CFE_Status);
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
} /* end ProcessUnsubFromPeer */

/**
 * \brief Process an unsubscription message from a peer.
 *
 * @param[in] Peer The peer interface
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

    uint16 SubCnt = 0;
    Unpack_UInt16(&Pack, &SubCnt);

    int SubIdx = 0;
    for (SubIdx = 0; SubIdx < SubCnt; SubIdx++)
    {
        CFE_SB_MsgId_t MsgID = CFE_SB_INVALID_MSG_ID;
        Unpack_MsgID(&Pack, &MsgID);
        CFE_SB_Qos_t QoS = { 0 };
        Unpack_Data(&Pack, &QoS, sizeof(QoS));

        ProcessUnsubFromPeer(Peer, MsgID); /* ignore return value, I want to unsub as much as I can */
    } /* end for */

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

    if (Ptr->Payload.Entries > CFE_MISSION_SB_SUB_ENTRIES_PER_PKT)
    {
        EVSSendErr(SBN_SUB_EID,
                   "entries value %d in SB PrevSubMsg exceeds max %d, aborting",
                   (int)Ptr->Payload.Entries,
                   CFE_MISSION_SB_SUB_ENTRIES_PER_PKT);
        return SBN_ERROR;
    } /* end if */

    EVSSendInfo(SBN_SUB_EID, "Processing all subscriptions...");

    for (i = 0; i < Ptr->Payload.Entries; i++)
    {
        SBN_Status = ProcessLocalSub(Ptr->Payload.Entry[i].MsgId, Ptr->Payload.Entry[i].Qos);
        if (SBN_Status != SBN_SUCCESS)
        {
            return SBN_Status;
        } /* end if */
    } /* end for */

    return SBN_Status;
} /* end SBN_ProcessAllSubscriptions */

/**
 * Removes all subscriptions (unsubscribe from the local SB )
 * for the specified peer, particularly when the peer connection has been lost.
 *
 * @param[in] Peer The peer interface
 *
 * @return SBN_SUCCESS always (whether or not there were some unsubs that failed.)
 */
SBN_Status_t SBN_RemoveAllSubsFromPeer(SBN_PeerInterface_t *Peer)
{
    int          i = 0;
    CFE_Status_t CFE_Status;

    for (i = 0; i < Peer->SubCnt; i++)
    {
        /** Ignore CFE_SB_BAD_ARGUMENT errors -- currently not checking if the pipe is valid before unsubscribing **/
        CFE_Status = CFE_SB_UnsubscribeLocal(Peer->Subs[i].MsgID, Peer->Pipe);
        if (CFE_Status != CFE_SUCCESS && CFE_Status != CFE_SB_BAD_ARGUMENT)
        {
            EVSSendErr(SBN_SUB_EID,
                       "unable to unsubscribe from message id 0x%04X: 0x%08X",
                       CFE_SB_MsgIdToValue(Peer->Subs[i].MsgID),
                       CFE_Status);
            /* but continue processing... */
        } /* end if */
    } /* end for */

    EVSSendInfo(SBN_SUB_EID, "unsubscribed %d message id's from ProcessorID %d", (int)Peer->SubCnt, Peer->ProcessorID);

    Peer->SubCnt = 0;

    return SBN_SUCCESS;
} /* end SBN_RemoveAllSubsFromPeer */
