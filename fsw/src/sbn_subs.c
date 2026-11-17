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
#include "sbn_outbound.h"
#include "sbn_filter.h"

// TODO: instead of using void * for the buffer for SBN messages, use
// a struct that has the SBN header in packed bytes.

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_SubChange_t *SBN_Subs_GetChangeEntry(SBN_SubJournal_t *Journal, uint32 Version)
{
    SBN_SubChange_t *Entry;

    Entry = &Journal->Entries[Version & SBN_SUBSCRIPTION_UPDATE_QUEUE_MASK];

    return Entry;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_SubChange_t *SBN_Subs_GetPrevChangeEntry(SBN_SubJournal_t *Journal, uint32 Version)
{
    uint32           Age;
    SBN_SubChange_t *Entry;

    Age = Journal->CurrVersion - Version;
    if (Age > (SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH - 2))
    {
        /* too old! */
        Entry = NULL;
    }
    else
    {
        Entry = SBN_Subs_GetChangeEntry(Journal, Version);
    }

    return Entry;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Subs_RegisterChange(SBN_SubJournal_t *Journal, bool IsSubscribe, CFE_SB_MsgId_t MsgId, CFE_SB_Qos_t QoS)
{
    SBN_SubChange_t *ChangeRec;

    ChangeRec = SBN_Subs_GetChangeEntry(Journal, Journal->CurrVersion);

    ChangeRec->IsSubscribe = IsSubscribe;
    ChangeRec->MsgId       = MsgId;
    ChangeRec->QoS         = QoS;

    ++Journal->CurrVersion;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Adds an entry to the SBN_SubscriptionSet_t
 *
 *-----------------------------------------------------------------*/
bool SBN_Subs_AddToSet(SBN_SubscriptionSet_t *SubSet, CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t QoS)
{
    if (SubSet->SubCnt >= SBN_MISSION_MAX_SUBS_PER_PEER)
    {
        return false;
    }

    SubSet->Subs[SubSet->SubCnt].MsgID = MsgID;
    SubSet->Subs[SubSet->SubCnt].QoS   = QoS;
    ++SubSet->SubCnt;

    return true;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Removes the specified entry from the SBN_SubscriptionSet_t
 *
 *-----------------------------------------------------------------*/
bool SBN_Subs_RemoveFromSet(SBN_SubscriptionSet_t *SubSet, SBN_SubCnt_t Idx)
{
    if (SubSet->SubCnt <= Idx)
    {
        return false;
    }

    /* Remove the entry by shifting everything beyond it up one slot */
    --SubSet->SubCnt;
    if (SubSet->SubCnt > Idx)
    {
        memmove(&SubSet->Subs[Idx], &SubSet->Subs[Idx + 1], sizeof(SBN_Subs_t) * (SubSet->SubCnt - Idx));
    }
    memset(&SubSet->Subs[SubSet->SubCnt], 0, sizeof(SBN_Subs_t));

    return true;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Utility to find the subscription index that refers to the message ID.
 *
 *-----------------------------------------------------------------*/
static bool SBN_Subs_CheckInSet(SBN_SubCnt_t *IdxPtr, SBN_SubscriptionSet_t *SubSet, CFE_SB_MsgId_t MsgID)
{
    SBN_SubCnt_t i;

    for (i = 0; i < SubSet->SubCnt; i++)
    {
        if (CFE_SB_MsgId_Equal(SubSet->Subs[i].MsgID, MsgID))
        {
            if (IdxPtr)
            {
                *IdxPtr = i;
            } /* end if */

            return true;
        } /* end if */
    } /* end for */

    return false;
} /* end SBN_Subs_CheckLocalMsgId */

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
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

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Sends a local subscription update over the wire to a peer.
 * This can be a single subscribe or unsubscribe event
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t
SBN_Subs_SendChangeToPeer(SBN_BufferDesc_t *BufD, const SBN_SubChange_t *SubChange, SBN_PeerRecord_t *PeerRec)
{
    SBN_PackState_t Pack;

    SBN_Pack_Init(&Pack, BufD->DataPtr, BufD->MaxDataSz, 0);
    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, 1);

    SBN_Pack_MsgID(&Pack, SubChange->MsgId);
    SBN_Pack_Data(&Pack, &SubChange->QoS, sizeof(SubChange->QoS)); /* 2 uint8's */

    EVSSendDbg(SBN_PEER_EID,
               "send local sub to peer %d:%d",
               SBN_Peer_GetSpacecraftID(PeerRec),
               SBN_Peer_GetProcessorID(PeerRec));

    BufD->ActualDataSz = SBN_Pack_GetPosition(&Pack);

    if (SubChange->IsSubscribe)
    {
        BufD->MsgType = SBN_SUB_MSG;
    }
    else
    {
        BufD->MsgType = SBN_UNSUB_MSG;
    }

    return SBN_Outbound_SendToPeer(BufD, SBN_Peer_GetIntfData(PeerRec));
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Subs_SendLocalToPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    SBN_Status_t           Status;
    SBN_PackState_t        Pack;
    uint32                 SubsVersion;
    SBN_SubscriptionSet_t *SubSet;
    SBN_SubCnt_t           i;

    SubsVersion = SBN_AppData.SubQ.CurrVersion;
    SubSet      = &SBN_AppData.SubSet;

    SBN_Pack_Init(&Pack, BufD->DataPtr, BufD->MaxDataSz, 0);

    SBN_Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    SBN_Pack_UInt16(&Pack, SubSet->SubCnt);

    for (i = 0; i < SubSet->SubCnt; i++)
    {
        SBN_Pack_MsgID(&Pack, SubSet->Subs[i].MsgID);
        /* 2 uint8's */
        SBN_Pack_Data(&Pack, &SubSet->Subs[i].QoS, sizeof(SubSet->Subs[i].QoS));
    } /* end for */

    EVSSendDbg(SBN_PEER_EID,
               "send local subs to peer %d:%d",
               SBN_Peer_GetSpacecraftID(PeerRec),
               SBN_Peer_GetProcessorID(PeerRec));

    BufD->ActualDataSz = SBN_Pack_GetPosition(&Pack);
    BufD->MsgType      = SBN_SUB_MSG;

    Status = SBN_Outbound_SendToPeer(BufD, SBN_Peer_GetIntfData(PeerRec));

    if (Status == SBN_SUCCESS)
    {
        /* Record that we sent this version of the subs table to the peer */
        PeerRec->Priv.PeerSentSubVersion = SubsVersion;
    }

    return Status;
} /* end SBN_Subs_SendLocalToPeer */

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Utility to find the local subscription index that refers to the message ID.
 *
 *-----------------------------------------------------------------*/
static bool SBN_Subs_CheckLocalMsgId(SBN_SubCnt_t *IdxPtr, CFE_SB_MsgId_t MsgID)
{
    return SBN_Subs_CheckInSet(IdxPtr, &SBN_AppData.SubSet, MsgID);
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Utility to find the peer subscription index that refers to the message ID.
 *
 *-----------------------------------------------------------------*/
static bool SBN_Subs_CheckPeerMsgId(SBN_SubCnt_t *SubIdxPtr, CFE_SB_MsgId_t MsgID, SBN_PeerInterface_t *Peer)
{
    return SBN_Subs_CheckInSet(SubIdxPtr, &Peer->SubSet, MsgID);
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Tracks a local subscription (add)
 *
 * param[in] MsgID The CCSDS Message ID of the local subscription.
 * param[in] QoS The CCSDS quality of service of the local subscription.
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_ProcessLocalSub(CFE_SB_MsgId_t MsgID, CFE_SB_Qos_t QoS)
{
    SBN_SubCnt_t SubIdx = 0;

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
    {
        return SBN_SUCCESS;
    }

    /* don't send SBN messages */
    if (CFE_SB_MsgId_Equal(MsgID, LCMD_MID) || CFE_SB_MsgId_Equal(MsgID, LHK_TLM_MID)
        || CFE_SB_MsgId_Equal(MsgID, LHKNET_TLM_MID) || CFE_SB_MsgId_Equal(MsgID, LHKPEER_TLM_MID)
        || CFE_SB_MsgId_Equal(MsgID, LHKMYSUBS_TLM_MID) || CFE_SB_MsgId_Equal(MsgID, LHKPEERSUBS_TLM_MID))
    {
        return SBN_SUCCESS;
    }

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if (SBN_Subs_CheckLocalMsgId(&SubIdx, MsgID))
    {
        /* does not send to peers, as they already know */
        return SBN_SUCCESS;
    } /* end if */

    if (!SBN_Subs_AddToSet(&SBN_AppData.SubSet, MsgID, QoS))
    {
        EVSSendErr(SBN_SUB_EID,
                   "local subscription ignored for MsgID 0x%04X, max (%d) met",
                   CFE_SB_MsgIdToValue(MsgID),
                   SBN_MISSION_MAX_SUBS_PER_PEER);
        return SBN_ERROR;
    } /* end if */

    /* track this change individually so it can be propagated to peers */
    SBN_Subs_RegisterChange(&SBN_AppData.SubQ, true, MsgID, QoS);

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Tracks a local unsubscription (remove)
 *
 * param[in] MsgID The CCSDS Message ID of the local subscription.
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_ProcessLocalUnsub(CFE_SB_MsgId_t MsgID)
{
    SBN_SubCnt_t SubIdx;

    /* find idx of matching subscription */
    if (!SBN_Subs_CheckLocalMsgId(&SubIdx, MsgID))
    {
        return SBN_SUCCESS; /* or should this be error? */
    } /* end if */

    /* remove sub from array for and
     * shift all subscriptions in higher elements to fill the gap */
    SBN_Subs_RemoveFromSet(&SBN_AppData.SubSet, SubIdx);

    /* track this change individually */
    SBN_Subs_RegisterChange(&SBN_AppData.SubQ, false, MsgID, CFE_SB_DEFAULT_QOS);

    return SBN_SUCCESS;
} /* end ProcessLocalUnsub */

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Handles a full subscription update from SB
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Subs_ProcessAllSubscriptions(const CFE_SB_AllSubscriptionsTlm_t *Ptr)
{
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    int          i;

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
        SBN_Status = SBN_ProcessLocalSub(Ptr->Payload.Entry[i].MsgId, Ptr->Payload.Entry[i].Qos);
        if (SBN_Status != SBN_SUCCESS)
        {
            return SBN_Status;
        } /* end if */
    } /* end for */

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Handles a singular/incremental subscription update from SB
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Subs_ProcessSingleSubscription(const CFE_SB_SingleSubscriptionTlm_t *Ptr)
{
    SBN_Status_t SBN_Status;

    if (Ptr->Payload.SubType == CFE_SB_SUBSCRIPTION)
    {
        SBN_Status = SBN_ProcessLocalSub(Ptr->Payload.MsgId, Ptr->Payload.Qos);
    }
    else if (Ptr->Payload.SubType == CFE_SB_UNSUBSCRIPTION)
    {
        SBN_Status = SBN_ProcessLocalUnsub(Ptr->Payload.MsgId);
    }
    else
    {
        EVSSendErr(SBN_SUB_EID, "unexpected subscription type (%d) in SBN_Subs_PollCycle", Ptr->Payload.SubType);
        SBN_Status = SBN_ERROR;
    }

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Subs_PollCycle(void)
{
    CFE_Status_t CFE_Status;
    SBN_Status_t SBN_Status;

    const CFE_SB_Buffer_t *BufPtr;
    CFE_SB_MsgId_t         MsgId              = CFE_SB_INVALID_MSG_ID;
    static CFE_SB_MsgId_t  SB_ONESUB_TLM_MID  = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t  SB_ALLSUBS_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(SB_ONESUB_TLM_MID))
    {
        SB_ONESUB_TLM_MID  = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
        SB_ALLSUBS_TLM_MID = CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID);
    }

    SBN_Status = SBN_ERROR;
    CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&BufPtr, SBN_AppData.SubPipe, CFE_SB_POLL);
    if (CFE_Status == CFE_SB_NO_MESSAGE || CFE_Status == CFE_SB_TIME_OUT)
    {
        /* This is OK/benign -- nothing wrong here */
        SBN_Status = SBN_IF_EMPTY;
    }
    else if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_MSG_EID, "err from rcvmsg on sub pipe");
    }
    else if (CFE_MSG_GetMsgId(&BufPtr->Msg, &MsgId) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_MSG_EID, "unable to get message id");
    }
    else if (CFE_SB_MsgId_Equal(MsgId, SB_ONESUB_TLM_MID))
    {
        SBN_Status = SBN_Subs_ProcessSingleSubscription((const CFE_SB_SingleSubscriptionTlm_t *)BufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, SB_ALLSUBS_TLM_MID))
    {
        SBN_Status = SBN_Subs_ProcessAllSubscriptions((const CFE_SB_AllSubscriptionsTlm_t *)BufPtr);
    }
    else
    {
        EVSSendErr(SBN_MSG_EID, "unexpected message id (0x%04X) on SBN_AppData.SubPipe", CFE_SB_MsgIdToValue(MsgId));
    }

    return SBN_Status;
} /* end SBN_Subs_PollCycle */

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Subs_ProcessMsgFromPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    SBN_Status_t         SBN_Status;
    SBN_UnpackState_t    Pack;
    char                 VersionHash[SBN_IDENT_LEN];
    SBN_SubCnt_t         SubIdx;
    uint16               SubCnt;
    CFE_SB_MsgId_t       MsgID;
    CFE_SB_Qos_t         QoS;
    bool                 IsSubscribe = (BufD->MsgType == SBN_SUB_MSG);
    SBN_PeerInterface_t *Peer        = SBN_Peer_GetIntfData(PeerRec);

    SBN_Unpack_Init(&Pack, BufD->DataPtr, BufD->ActualDataSz);

    SBN_Unpack_Data(&Pack, VersionHash, SBN_IDENT_LEN);

    if (strncmp(VersionHash, SBN_IDENT, SBN_IDENT_LEN))
    {
        EVSSendErr(SBN_PROTO_EID, "version number mismatch with peer CpuID %d", SBN_Peer_GetProcessorID(PeerRec));
        return SBN_ERROR;
    }

    SubCnt = 0;
    SBN_Unpack_UInt16(&Pack, &SubCnt);

    if (SubCnt > SBN_MISSION_MAX_SUBS_PER_PEER)
    {
        SubCnt = SBN_MISSION_MAX_SUBS_PER_PEER;
    }

    for (SubIdx = 0; SubIdx < SubCnt; SubIdx++)
    {
        MsgID = CFE_SB_INVALID_MSG_ID;
        QoS   = CFE_SB_DEFAULT_QOS;

        SBN_Unpack_MsgID(&Pack, &MsgID);
        SBN_Unpack_Data(&Pack, &QoS, sizeof(QoS));

        SBN_Status = SBN_Filter_Remap(&MsgID, Peer);
        if (SBN_Status == SBN_SUCCESS)
        {
            /* The main thread might be processing subscriptions, so all we can do is
             * add to this list */
            /* Track this individual change */
            SBN_Subs_RegisterChange(&PeerRec->Priv.SubQ, IsSubscribe, MsgID, QoS);
        }
    }

    return SBN_SUCCESS;
} /* SBN_ProcessSubsFromPeer */

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Subs_UpdateDisconnectedPeerLocalSubs(SBN_PeerRecord_t *PeerRec)
{
    SBN_PeerInterface_t *Peer = SBN_Peer_GetIntfData(PeerRec);

    if (CFE_RESOURCEID_TEST_DEFINED(Peer->Pipe))
    {
        /* The peer has disconnected but our local pipe is still open */
        /* Deleting the pipe will also purge all subscriptions, we do not really
         * need to call Unsubscribe for each one. */
        CFE_SB_DeletePipe(Peer->Pipe);
        Peer->Pipe = CFE_SB_INVALID_PIPE;

        EVSSendInfo(SBN_SUB_EID, "deleted local pipe for ProcessorID %d", SBN_Peer_GetProcessorID(PeerRec));

        /* A disconnected peer has no subs */
        memset(&Peer->SubSet, 0, sizeof(Peer->SubSet));
        PeerRec->Priv.LocalPipeCurrSubVersion = PeerRec->Priv.SubQ.CurrVersion;

        /* Flag that the peer is unsynchronized w/ our subs and needs a full update */
        /* Since the CurrVersion is of our local subs is shared between all peers, do not change it,
         * but rather make it so the version we sent to the peer is far behind */
        PeerRec->Priv.PeerSentSubVersion = SBN_AppData.SubQ.CurrVersion - SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH;
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Sends the oldest unsent incremental subscription update to the remote peer
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Subs_IncrementPeerSubVersion(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    SBN_SubChange_t *ChangePtr;
    SBN_Status_t     SBN_Status;

    ChangePtr = SBN_Subs_GetPrevChangeEntry(&SBN_AppData.SubQ, PeerRec->Priv.PeerSentSubVersion);
    if (ChangePtr == NULL)
    {
        /* This means the peer is too far out of sync to do an incremental update */
        SBN_Status = SBN_SUBS_NOT_SYNCED;
    }
    else
    {
        SBN_Status = SBN_Subs_SendChangeToPeer(BufD, ChangePtr, PeerRec);

        if (SBN_Status == SBN_SUCCESS)
        {
            /* incremental update worked, increment version one step */
            ++PeerRec->Priv.PeerSentSubVersion;
        }
    }

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Applies the oldest unapplied incremental subscription update to the local pipe
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Subs_IncrementLocalPipeSubVersion(SBN_PeerRecord_t *PeerRec)
{
    SBN_SubChange_t     *ChangePtr;
    SBN_Status_t         SBN_Status;
    SBN_SubCnt_t         SubIdx;
    bool                 WasSubscribed;
    SBN_PeerInterface_t *Peer = SBN_Peer_GetIntfData(PeerRec);

    /* Attempt to make the change.  This is "best effort" because if we do not
     * succeed, there is no real recourse.  Trying again with the same MsgId probably
     * will not have a better outcome.  This probably should be noted somehow so
     * a complete resync can be done */

    ChangePtr = SBN_Subs_GetPrevChangeEntry(&PeerRec->Priv.SubQ, PeerRec->Priv.LocalPipeCurrSubVersion);
    if (ChangePtr == NULL)
    {
        /* This means the peer is too far out of sync to do an incremental update */
        /* This should never really happen.  There should be no way for us to get this far
         * out of sync with the subscriptions that the peer has sent us.  There is also no real
         * recovery for this, because the information is lost */
        SBN_Status = SBN_SUBS_NOT_SYNCED;

        /* Jump forward in the history.  This is not correct but no other recourse. */
        PeerRec->Priv.LocalPipeCurrSubVersion =
            PeerRec->Priv.SubQ.CurrVersion - (SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH - 4);
    }
    else
    {
        WasSubscribed = SBN_Subs_CheckPeerMsgId(&SubIdx, ChangePtr->MsgId, Peer);

        if (WasSubscribed == ChangePtr->IsSubscribe)
        {
            /* nothing to change */
        }
        else if (WasSubscribed)
        {
            /* Apply Unsubscription (best effort) */
            CFE_SB_UnsubscribeLocal(ChangePtr->MsgId, Peer->Pipe);
            SBN_Subs_RemoveFromSet(&Peer->SubSet, SubIdx);
        }
        else if (SBN_Subs_AddToSet(&Peer->SubSet, ChangePtr->MsgId, ChangePtr->QoS))
        {
            /* Apply Subscription (best effort) */
            CFE_SB_SubscribeLocal(ChangePtr->MsgId, PeerRec->Intf.Pipe, SBN_PLATFORM_PEER_MSG_LIM);
        }
        else
        {
            EVSSendErr(SBN_SUB_EID,
                       "cannot process subscription from ProcessorID %d, max (%d) met",
                       SBN_Peer_GetProcessorID(PeerRec),
                       SBN_MISSION_MAX_SUBS_PER_PEER);
        }

        /* incremental update worked */
        ++PeerRec->Priv.LocalPipeCurrSubVersion;
        SBN_Status = SBN_SUCCESS;
    }

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Subs_SetupLocalPipe(SBN_PeerRecord_t *PeerRec)
{
    CFE_Status_t           CFE_Status;
    SBN_SubCnt_t           i;
    char                   LocalPipeName[OS_MAX_API_NAME];
    SBN_PeerInterface_t   *Peer   = SBN_Peer_GetIntfData(PeerRec);
    SBN_SubscriptionSet_t *SubSet = &Peer->SubSet;

    if (CFE_RESOURCEID_TEST_DEFINED(Peer->Pipe))
    {
        CFE_SB_DeletePipe(Peer->Pipe);
    }

    snprintf(LocalPipeName,
             sizeof(LocalPipeName),
             "SBNPEER_%lx",
             CFE_ResourceId_ToInteger(CFE_RESOURCEID_UNWRAP(PeerRec->PeerId)));
    CFE_Status = CFE_SB_CreatePipe(&Peer->Pipe, SBN_PLATFORM_PEER_PIPE_DEPTH, LocalPipeName);
    if (CFE_Status != CFE_SUCCESS)
    {
        Peer->Pipe = CFE_SB_INVALID_PIPE;
    }
    else
    {
        /* Now subscribe to everything we know about */
        for (i = 0; i < SubSet->SubCnt; ++i)
        {
            /* Best effort; if the peer sent a bad MsgID, it will fail and be ignored (we tried) */
            CFE_SB_SubscribeLocal(SubSet->Subs[i].MsgID, Peer->Pipe, SBN_PLATFORM_PEER_MSG_LIM);
        }
    }
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Subs_UpdateConnectedPeerRemoteSubs(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    SBN_Status_t SBN_Status;
    uint32       LastVersion;

    while (PeerRec->Priv.PeerSentSubVersion != SBN_AppData.SubQ.CurrVersion)
    {
        LastVersion = PeerRec->Priv.PeerSentSubVersion;

        /* First try to send an incremental update to the peer */
        SBN_Status = SBN_Subs_IncrementPeerSubVersion(BufD, PeerRec);

        if (SBN_Status == SBN_SUBS_NOT_SYNCED)
        {
            /* incremental update not possible, do full update */
            SBN_Subs_SendLocalToPeer(PeerRec, BufD);
        }

        /* Note - if either of the above functions worked, it should have updated PeerSentSubVersion */
        if (PeerRec->Priv.PeerSentSubVersion == LastVersion)
        {
            /* no update was possible, something is broken, do not try endlessly */
            break;
        }
    }
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Subs_UpdateConnectedPeerLocalSubs(SBN_PeerRecord_t *PeerRec)
{
    SBN_Status_t SBN_Status;
    uint32       LastVersion;

    while (PeerRec->Priv.LocalPipeCurrSubVersion != PeerRec->Priv.SubQ.CurrVersion)
    {
        LastVersion = PeerRec->Priv.LocalPipeCurrSubVersion;

        /* First try to apply an incremental update to our local pipe */
        if (CFE_RESOURCEID_TEST_DEFINED(PeerRec->Intf.Pipe))
        {
            SBN_Status = SBN_Subs_IncrementLocalPipeSubVersion(PeerRec);
        }
        else
        {
            /* There may not be a local pipe when initially establishing the connection */
            /* By definition, the local subs are out of sync */
            SBN_Status = SBN_SUBS_NOT_SYNCED;
        }

        if (SBN_Status == SBN_SUBS_NOT_SYNCED)
        {
            /* [re-]create the pipe to get a clean slate */
            SBN_Subs_SetupLocalPipe(PeerRec);
        }

        /* Note - if either of the above functions worked, it should have updated PeerSentSubVersion */
        if (PeerRec->Priv.LocalPipeCurrSubVersion == LastVersion)
        {
            /* no update was possible, something is broken, do not try endlessly */
            break;
        }
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Invokes SBN_Subs_UpdateConnectedPeerLocalSubs() if a peer is connected
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Subs_DoSynchronizePeer(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    if (PeerRec->Intf.Connected)
    {
        SBN_Subs_UpdateConnectedPeerLocalSubs(PeerRec);
    }
    else
    {
        SBN_Subs_UpdateDisconnectedPeerLocalSubs(PeerRec);
    }

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Subs_SynchronizeAllPeers(void)
{
    SBN_ForEachConfiguredPeer(SBN_Subs_DoSynchronizePeer, NULL);
}
