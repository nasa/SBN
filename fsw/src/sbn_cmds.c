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
#include "sbn_pack.h"

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
    uint8 *Ptr     = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8  NetIdx  = *Ptr++;
    uint8  PeerIdx = *Ptr;

    if (NetIdx < 0 || NetIdx >= SBN_AppData.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx %d (max=%d)", NetIdx, SBN_AppData.NetCnt);
        return CFE_SUCCESS; /* Return value should still be success */
    } /* end if */

    SBN_NetInterface_t *Net = &SBN_AppData.Nets[NetIdx];

    if (PeerIdx < 0 || PeerIdx >= Net->PeerCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid PeerIdx %d (max=%d)", PeerIdx, Net->PeerCnt);
        return CFE_SUCCESS; /* Return value should still be success */
    } /* end if */

    SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

    EVSSendInfo(SBN_CMD_EID, "hk reset peer command (NetIdx=%d, PeerIdx=%d)", NetIdx, PeerIdx);
    SBN_AppData.CmdCnt++;

    SBN_InitializePeerCounters(Peer);

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
    EVSSendDbg(SBN_CMD_EID, "hk command");

    uint8                 HKBuf[SBN_HK_LEN];
    CFE_MSG_Message_t    *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    SBN_PackState_t       Pack;
    static CFE_SB_MsgId_t HK_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HK_TLM_MID))
    {
        HK_TLM_MID = CFE_SB_ValueToMsgId(SBN_HK_TLM_MID);
    }

    CFE_MSG_Init(HKMsg, HK_TLM_MID, SBN_HK_LEN);

    SBN_Pack_Init(&Pack,
                  HKBuf + sizeof(CFE_MSG_TelemetryHeader_t),
                  SBN_HK_LEN - sizeof(CFE_MSG_TelemetryHeader_t),
                  true);

    SBN_Pack_UInt8(&Pack, SBN_HK_CC);
    SBN_Pack_UInt16(&Pack, SBN_AppData.CmdCnt);
    SBN_Pack_UInt16(&Pack, SBN_AppData.CmdErrCnt);
    SBN_Pack_UInt16(&Pack, SBN_AppData.SubCnt);
    SBN_Pack_UInt16(&Pack, SBN_AppData.NetCnt);

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);

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
    uint8 *Ptr    = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8  NetIdx = *Ptr;

    if (NetIdx > SBN_AppData.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx (%d, max is %d)", NetIdx, SBN_AppData.NetCnt - 1);
        return CFE_SUCCESS; /* Return value should still be success */
    } /* end if */

    EVSSendInfo(SBN_CMD_EID, "hk command, net=%d", NetIdx);

    uint8                 HKBuf[SBN_HKNET_LEN];
    CFE_MSG_Message_t    *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    SBN_PackState_t       Pack;
    static CFE_SB_MsgId_t HKNET_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKNET_TLM_MID))
    {
        HKNET_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKNET_TLM_MID);
    }

    CFE_MSG_Init(HKMsg, HKNET_TLM_MID, SBN_HKNET_LEN);

    SBN_Pack_Init(&Pack,
                  HKBuf + sizeof(CFE_MSG_TelemetryHeader_t),
                  SBN_HKNET_LEN - sizeof(CFE_MSG_TelemetryHeader_t),
                  true);

    SBN_Pack_UInt8(&Pack, SBN_HK_NET_CC);
    SBN_Pack_UInt8(&Pack, SBN_AppData.Nets[NetIdx].ProtocolIdx);
    SBN_Pack_UInt16(&Pack, SBN_AppData.Nets[NetIdx].PeerCnt);

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);

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
    uint8          *Ptr     = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8           NetIdx  = *Ptr++;
    uint8           PeerIdx = *Ptr;
    SBN_Timestamp_t TimeTemp;

    if (NetIdx > SBN_AppData.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx (%d, max is %d)", NetIdx, SBN_AppData.NetCnt - 1);
        return CFE_SUCCESS; /* Return value should still be success */
    } /* end if */

    if (PeerIdx > SBN_AppData.Nets[NetIdx].PeerCnt)
    {
        EVSSendErr(SBN_CMD_EID,
                   "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)",
                   NetIdx,
                   PeerIdx,
                   SBN_AppData.Nets[NetIdx].PeerCnt - 1);
        return CFE_SUCCESS; /* Return value should still be success */
    } /* end if */

    SBN_PeerInterface_t *Peer = &SBN_AppData.Nets[NetIdx].Peers[PeerIdx];

    EVSSendInfo(SBN_CMD_EID, "hk peer command, net=%d, peer=%d", NetIdx, PeerIdx);

    uint8                 HKBuf[SBN_HKPEER_LEN];
    CFE_MSG_Message_t    *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    SBN_PackState_t       Pack;
    static CFE_SB_MsgId_t HKPEER_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKPEER_TLM_MID))
    {
        HKPEER_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKPEER_TLM_MID);
    }

    CFE_MSG_Init(HKMsg, HKPEER_TLM_MID, SBN_HKPEER_LEN);

    SBN_Pack_Init(&Pack,
                  HKBuf + sizeof(CFE_MSG_TelemetryHeader_t),
                  SBN_HKPEER_LEN - sizeof(CFE_MSG_TelemetryHeader_t),
                  true);

    SBN_Pack_UInt8(&Pack, SBN_HK_PEER_CC);
    SBN_Pack_UInt32(&Pack, Peer->ProcessorID);
    TimeTemp = 100 * OS_TimeGetTotalMicroseconds(Peer->LastSend);
    SBN_Pack_Time(&Pack, TimeTemp);
    TimeTemp = 100 * OS_TimeGetTotalMicroseconds(Peer->LastRecv);
    SBN_Pack_Time(&Pack, TimeTemp);
    SBN_Pack_UInt16(&Pack, Peer->SendCnt);
    SBN_Pack_UInt16(&Pack, Peer->RecvCnt);
    SBN_Pack_UInt16(&Pack, Peer->SendErrCnt);
    SBN_Pack_UInt16(&Pack, Peer->RecvErrCnt);
    SBN_Pack_UInt16(&Pack, Peer->SubCnt);

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);

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
    EVSSendInfo(SBN_CMD_EID, "hk subs command");

    uint8                 HKBuf[SBN_HKMYSUBS_LEN];
    CFE_MSG_Message_t    *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    SBN_PackState_t       Pack;
    static CFE_SB_MsgId_t HKMYSUBS_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKMYSUBS_TLM_MID))
    {
        HKMYSUBS_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKMYSUBS_TLM_MID);
    }

    CFE_MSG_Init(HKMsg, HKMYSUBS_TLM_MID, SBN_HKMYSUBS_LEN);

    SBN_Pack_Init(&Pack,
                  HKBuf + sizeof(CFE_MSG_TelemetryHeader_t),
                  SBN_HKMYSUBS_LEN - sizeof(CFE_MSG_TelemetryHeader_t),
                  true);

    SBN_Pack_UInt8(&Pack, SBN_HK_MYSUBS_CC);
    SBN_Pack_UInt16(&Pack, SBN_AppData.SubCnt);
    int i;
    for (i = 0; i < SBN_AppData.SubCnt; i++)
    {
        SBN_Pack_MsgID(&Pack, SBN_AppData.Subs[i].MsgID);
    }

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);

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
    uint8 *Ptr     = (uint8 *)MsgPtr + sizeof(CFE_MSG_CommandHeader_t);
    uint8  NetIdx  = *Ptr++;
    uint8  PeerIdx = *Ptr;

    if (NetIdx >= SBN_AppData.NetCnt)
    {
        EVSSendErr(SBN_CMD_EID, "Invalid NetIdx (%d, max is %d)", NetIdx, SBN_AppData.NetCnt - 1);
        return CFE_SUCCESS; /* Return value should still be success */
    } /* end if */

    if (PeerIdx >= SBN_AppData.Nets[NetIdx].PeerCnt)
    {
        EVSSendErr(SBN_CMD_EID,
                   "Invalid PeerIdx (NetIdx=%d PeerIdx=%d, max is %d)",
                   NetIdx,
                   PeerIdx,
                   SBN_AppData.Nets[NetIdx].PeerCnt - 1);
        return CFE_SUCCESS; /* Return value should still be success */
    } /* end if */

    EVSSendInfo(SBN_CMD_EID, "hk subs command, net=%d peer=%d", NetIdx, PeerIdx);

    SBN_PeerInterface_t *Peer = &SBN_AppData.Nets[NetIdx].Peers[PeerIdx];

    uint8                 HKBuf[SBN_HKPEERSUBS_LEN];
    CFE_MSG_Message_t    *HKMsg = (CFE_MSG_Message_t *)HKBuf;
    SBN_PackState_t       Pack;
    static CFE_SB_MsgId_t HKPEERSUBS_TLM_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(HKPEERSUBS_TLM_MID))
    {
        HKPEERSUBS_TLM_MID = CFE_SB_ValueToMsgId(SBN_HKPEERSUBS_TLM_MID);
    }

    CFE_MSG_Init(HKMsg, HKPEERSUBS_TLM_MID, SBN_HKPEERSUBS_LEN);

    SBN_Pack_Init(&Pack,
                  HKBuf + sizeof(CFE_MSG_TelemetryHeader_t),
                  SBN_HKPEERSUBS_LEN - sizeof(CFE_MSG_TelemetryHeader_t),
                  true);

    SBN_Pack_UInt8(&Pack, SBN_HK_PEERSUBS_CC);
    SBN_Pack_UInt16(&Pack, NetIdx);
    SBN_Pack_UInt16(&Pack, PeerIdx);
    SBN_Pack_UInt16(&Pack, Peer->SubCnt);

    int i;
    for (i = 0; i < Peer->SubCnt; i++)
    {
        SBN_Pack_MsgID(&Pack, Peer->Subs[i].MsgID);
    }

    /*
    ** Timestamp and send packet
    */
    CFE_SB_TimeStampMsg(HKMsg);
    CFE_SB_TransmitMsg(HKMsg, true);

    return CFE_SUCCESS;
} /* end PeerSubsCmd */

CFE_Status_t SBN_WakeupCmd(const SBN_WakeupCmd_t *MsgPtr)
{
    EVSSendDbg(SBN_CMD_EID, "wakeup");
    return CFE_SUCCESS;
}
