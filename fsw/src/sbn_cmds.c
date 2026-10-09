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

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "sbn_app.h"
#include "sbn_encode.h"

static SBN_NetInterface_t *SBN_GetNetIfFromIdx(SBN_NetIdx_t Idx)
{
    if (Idx >= SBN_AppData.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx %d (max=%d)", Idx, SBN_AppData.NetCnt);
        return NULL;
    }

    return &SBN_AppData.Nets[Idx];
}

static SBN_PeerInterface_t *SBN_GetPeerFromIdx(SBN_NetInterface_t *NetIf, SBN_PeerIdx_t Idx)
{
    /* Note that the netif + peer lookup is often combined, so its easier to pass thru a prior error */
    if (NetIf == NULL)
    {
        return NULL;
    }

    if (Idx >= NetIf->PeerCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid PeerIdx %d (max=%d)", Idx, NetIf->PeerCnt);
        return NULL;
    }

    return &NetIf->Peers[Idx];
}

static void SBN_TransmitBuffer(SBN_TlmBase_t *TlmBuf)
{
    CFE_SB_Buffer_t *SbBufPtr;
    CFE_Status_t     Status;

    SbBufPtr = SBN_Encode_Output(TlmBuf);
    if (SbBufPtr != NULL)
    {
        Status = CFE_SB_TransmitBuffer(SbBufPtr, true);
    }
    else
    {
        Status = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    if (Status != CFE_SUCCESS)
    {
        SBN_Encode_ReleaseBuffer(TlmBuf);
    }
}

/************************************************************************/
/** \brief Noop command
**
**  \par Description
**       Processes a noop ground command.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_NOOP_CC
**
*************************************************************************/
CFE_Status_t SBN_NoopCmd(const SBN_NoopCmd_t *MsgPtr)
{
    EVSSendInfo(SBN_CMD_EID, "no-op command");

    SBN_AppData.CmdCnt++;

    return CFE_SUCCESS;
} /* end NoopCmd */

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
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_HK_RESET_CC
**
*************************************************************************/
CFE_Status_t SBN_ResetCmd(const SBN_ResetCmd_t *MsgPtr)
{
    EVSSendInfo(SBN_CMD_EID, "reset command");

    /*
    ** Don't increment counter because we're resetting anyway
    */
    SBN_InitializeCounters();

    return CFE_SUCCESS;
} /* end HKResetCmd */

/************************************************************************/
/** \brief Reset Peer counters command
**
**  \par Description
**       Reset the counters for a specified peer.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_HK_RESET_PEER_CC
**
*************************************************************************/
CFE_Status_t SBN_ResetPeerCmd(const SBN_ResetPeerCmd_t *MsgPtr)
{
    const SBN_PeerCmd_Payload_t *CmdPtr = &MsgPtr->Payload;
    SBN_PeerInterface_t         *Peer;

    Peer = SBN_GetPeerFromIdx(SBN_GetNetIfFromIdx(CmdPtr->NetIdx), CmdPtr->PeerIdx);
    if (Peer != NULL)
    {
        EVSSendInfo(SBN_CMD_EID, "hk reset peer command (NetIdx=%d, PeerIdx=%d)", CmdPtr->NetIdx, CmdPtr->PeerIdx);
        ++SBN_AppData.CmdCnt;

        SBN_InitializePeerCounters(Peer);
    }
    else
    {
        ++SBN_AppData.CmdErrCnt;
    }

    return CFE_SUCCESS;
} /* end HKResetPeerCmd */

/** \brief Housekeeping request command
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 */
CFE_Status_t SBN_SendHkCmd(const SBN_SendHkCmd_t *MsgPtr)
{
    SBN_TlmBase_t *TlmBuf;
    SBN_HkTlm_t   *OutMsg;

    static CFE_SB_MsgId_t HK_TLM_MID = CFE_SB_MSGID_RESERVED;

    EVSSendDbg(SBN_CMD_EID, "hk command");

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HK_TLM_MID))
    {
        HK_TLM_MID = CFE_SB_ValueToMsgId(SBN_HK_TLM_MID);
    }

    TlmBuf = SBN_Encode_GetBuffer(sizeof(*OutMsg));
    if (TlmBuf != NULL)
    {
        CFE_MSG_Init(CFE_MSG_PTR(TlmBuf->TelemetryHeader), HK_TLM_MID, sizeof(*OutMsg));
        OutMsg = (SBN_HkTlm_t *)TlmBuf;

        OutMsg->TlmBase.TlmId     = SBN_HK_CC;
        OutMsg->Payload.CmdCnt    = SBN_AppData.CmdCnt;
        OutMsg->Payload.CmdErrCnt = SBN_AppData.CmdErrCnt;
        OutMsg->Payload.SubCnt    = SBN_AppData.SubCnt;
        OutMsg->Payload.NetCnt    = SBN_AppData.NetCnt;

        /*
        ** Timestamp and send packet
        */
        SBN_TransmitBuffer(TlmBuf);
    }

    return CFE_SUCCESS;
} /* end HKCmd */

/** \brief Request for housekeeping for one network.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 */
CFE_Status_t SBN_SendHkNetCmd(const SBN_SendHkNetCmd_t *MsgPtr)
{
    const SBN_NetCmd_Payload_t *CmdPtr = &MsgPtr->Payload;
    SBN_NetInterface_t         *NetIf;
    SBN_TlmBase_t              *TlmBuf;
    SBN_HkNetTlm_t             *OutMsg;

    static CFE_SB_MsgId_t HKNET_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKNET_TLM_MID))
    {
        HKNET_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKNET_TLM_MID);
    }

    NetIf = SBN_GetNetIfFromIdx(CmdPtr->NetIdx);
    if (NetIf != NULL)
    {
        TlmBuf = SBN_Encode_GetBuffer(sizeof(*OutMsg));
    }
    else
    {
        TlmBuf = NULL;
    }

    if (TlmBuf != NULL)
    {
        EVSSendInfo(SBN_CMD_EID, "hk command, net=%d", CmdPtr->NetIdx);

        CFE_MSG_Init(CFE_MSG_PTR(TlmBuf->TelemetryHeader), HKNET_TLM_MID, sizeof(*OutMsg));
        OutMsg = (SBN_HkNetTlm_t *)TlmBuf;

        OutMsg->TlmBase.TlmId = SBN_HK_NET_CC;

        OutMsg->Payload.ProtocolIdx = NetIf->ProtocolIdx;
        OutMsg->Payload.PeerCnt     = NetIf->PeerCnt;

        /*
        ** Timestamp and send packet
        */
        SBN_TransmitBuffer(TlmBuf);
    }

    return CFE_SUCCESS;
} /* end HKNetCmd */

/** \brief Request for housekeeping for one peer.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 */
CFE_Status_t SBN_SendHkPeerCmd(const SBN_SendHkPeerCmd_t *MsgPtr)
{
    const SBN_PeerCmd_Payload_t *CmdPtr = &MsgPtr->Payload;
    SBN_PeerInterface_t         *Peer;
    SBN_TlmBase_t               *TlmBuf;
    SBN_HkPeerTlm_t             *OutMsg;

    static CFE_SB_MsgId_t HKPEER_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKPEER_TLM_MID))
    {
        HKPEER_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKPEER_TLM_MID);
    }

    Peer = SBN_GetPeerFromIdx(SBN_GetNetIfFromIdx(CmdPtr->NetIdx), CmdPtr->PeerIdx);
    if (Peer != NULL)
    {
        TlmBuf = SBN_Encode_GetBuffer(sizeof(*OutMsg));
    }
    else
    {
        TlmBuf = NULL;
    }

    if (TlmBuf != NULL)
    {
        EVSSendInfo(SBN_CMD_EID, "hk peer command, net=%d, peer=%d", CmdPtr->NetIdx, CmdPtr->PeerIdx);

        CFE_MSG_Init(CFE_MSG_PTR(TlmBuf->TelemetryHeader), HKPEER_TLM_MID, sizeof(*OutMsg));
        OutMsg = (SBN_HkPeerTlm_t *)TlmBuf;

        OutMsg->TlmBase.TlmId = SBN_HK_PEER_CC;

        OutMsg->Payload.ProcessorId = Peer->ProcessorID;
        OutMsg->Payload.LastSend    = OS_TimeGetTotalMicroseconds(Peer->LastSend);
        OutMsg->Payload.LastRecv    = OS_TimeGetTotalMicroseconds(Peer->LastRecv);
        OutMsg->Payload.SendCnt     = Peer->SendCnt;
        OutMsg->Payload.RecvCnt     = Peer->RecvCnt;
        OutMsg->Payload.SendErrCnt  = Peer->SendErrCnt;
        OutMsg->Payload.RecvErrCnt  = Peer->RecvErrCnt;
        OutMsg->Payload.SubCnt      = Peer->SubCnt;

        /*
        ** Timestamp and send packet
        */
        SBN_TransmitBuffer(TlmBuf);
    }

    return CFE_SUCCESS;
} /* end HKPeerCmd */

/** \brief Send My Subscriptions
 *
 *  \par Assumptions, External Events, and Notes:
 *       None
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 *  \sa #SBN_HK_MYSUBS_CC
 */
CFE_Status_t SBN_SendHkMySubsCmd(const SBN_SendHkMySubsCmd_t *MsgPtr)
{
    SBN_TlmBase_t     *TlmBuf;
    SBN_HkMySubsTlm_t *OutMsg;
    SBN_SubCnt_t       i;

    static CFE_SB_MsgId_t HKMYSUBS_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKMYSUBS_TLM_MID))
    {
        HKMYSUBS_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKMYSUBS_TLM_MID);
    }

    TlmBuf = SBN_Encode_GetBuffer(sizeof(*OutMsg));
    if (TlmBuf != NULL)
    {
        EVSSendInfo(SBN_CMD_EID, "hk subs command");

        CFE_MSG_Init(CFE_MSG_PTR(TlmBuf->TelemetryHeader), HKMYSUBS_TLM_MID, sizeof(*OutMsg));

        OutMsg = (SBN_HkMySubsTlm_t *)TlmBuf;

        OutMsg->TlmBase.TlmId = SBN_HK_MYSUBS_CC;

        OutMsg->Payload.SubCnt = SBN_AppData.SubCnt;
        for (i = 0; i < SBN_AppData.SubCnt; i++)
        {
            OutMsg->Payload.Subs[i] = SBN_AppData.Subs[i].MsgID;
        }

        /*
        ** Timestamp and send packet
        */
        SBN_TransmitBuffer(TlmBuf);
    }

    return CFE_SUCCESS;
} /* end MySubsCmd */

/** \brief Reloads the config table.
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 *  \sa #SBN_TBL_CC
 */
CFE_Status_t SBN_ReloadTblCmd(const SBN_ReloadTblCmd_t *MsgPtr)
{
    EVSSendInfo(SBN_CMD_EID, "reload tbl command");
    SBN_ReloadConfTbl();

    return CFE_SUCCESS;
}

/************************************************************************/
/** \brief Send A Peer's Subscriptions
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr   A #CFE_MSG_Message_t pointer that
**                         references the software bus message
**
**  \sa #SBN_HK_MYSUBS_CC
**
*************************************************************************/
CFE_Status_t SBN_SendHkPeerSubsCmd(const SBN_SendHkPeerSubsCmd_t *MsgPtr)
{
    const SBN_PeerCmd_Payload_t *CmdPtr = &MsgPtr->Payload;
    SBN_PeerInterface_t         *Peer;
    SBN_TlmBase_t               *TlmBuf;
    SBN_HkPeerSubsTlm_t         *OutMsg;
    SBN_SubCnt_t                 i;

    static CFE_SB_MsgId_t HKPEERSUBS_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKPEERSUBS_TLM_MID))
    {
        HKPEERSUBS_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKPEERSUBS_TLM_MID);
    }

    Peer = SBN_GetPeerFromIdx(SBN_GetNetIfFromIdx(CmdPtr->NetIdx), CmdPtr->PeerIdx);
    if (Peer != NULL)
    {
        TlmBuf = SBN_Encode_GetBuffer(sizeof(*OutMsg));
    }
    else
    {
        TlmBuf = NULL;
    }

    if (TlmBuf != NULL)
    {
        EVSSendInfo(SBN_CMD_EID, "hk subs command, net=%d peer=%d", CmdPtr->NetIdx, CmdPtr->PeerIdx);

        CFE_MSG_Init(CFE_MSG_PTR(TlmBuf->TelemetryHeader), HKPEERSUBS_TLM_MID, sizeof(*OutMsg));

        OutMsg = (SBN_HkPeerSubsTlm_t *)TlmBuf;

        OutMsg->TlmBase.TlmId   = SBN_HK_PEERSUBS_CC;
        OutMsg->Payload.NetIdx  = CmdPtr->NetIdx;
        OutMsg->Payload.PeerIdx = CmdPtr->PeerIdx;
        OutMsg->Payload.SubCnt  = Peer->SubCnt;

        for (i = 0; i < Peer->SubCnt; i++)
        {
            OutMsg->Payload.Subs[i] = Peer->Subs[i].MsgID;
        }

        /*
        ** Timestamp and send packet
        */
        SBN_TransmitBuffer(TlmBuf);
    }

    return CFE_SUCCESS;
} /* end PeerSubsCmd */

CFE_Status_t SBN_WakeupCmd(const SBN_WakeupCmd_t *MsgPtr)
{
    EVSSendDbg(SBN_CMD_EID, "wakeup");
    return CFE_SUCCESS;
}
