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

/*
 ** Include Files
 */
#include "sbn_error.h"
#include "sbn_app.h"
#include "sbn_connection.h"
#include "sbn_worker.h"
#include "sbn_outbound.h"
#include "sbn_filter.h"

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Connection_SendProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    SBN_Status_t SBN_Status;
    uint8       *ProtocolVer;

    ProtocolVer        = BufD->DataPtr;
    BufD->ActualDataSz = sizeof(*ProtocolVer);
    BufD->MsgType      = SBN_PROTO_MSG;

    *ProtocolVer = SBN_PROTO_VER;

    SBN_Status = SBN_Outbound_SendToPeer(BufD, &PeerRec->Intf);

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Connection_RecvProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec)
{
    SBN_Status_t Status;
    const uint8 *InMsg;
    uint8        Ver;

    /* NOTE: this glazes over the packing/unpacking because its a single byte.
     * should do this properly */
    InMsg = BufD->DataPtr;
    Ver   = 0;

    if (BufD->ActualDataSz == sizeof(Ver))
    {
        Ver = *InMsg;
    }
    else
    {
        Ver = 0;
    }

    if (Ver != SBN_PROTO_VER)
    {
        Status = SBN_ERROR;
        EVSSendErr(SBN_SB_EID,
                   "SBN protocol version mismatch with peer %d:%d, "
                   "my version=%d, peer version %d",
                   SBN_Peer_GetSpacecraftID(PeerRec),
                   SBN_Peer_GetProcessorID(PeerRec),
                   (int)SBN_PROTO_VER,
                   (int)Ver);
    }
    else
    {
        Status = SBN_SUCCESS;
        EVSSendInfo(SBN_SB_EID,
                    "SBN protocol version match with peer %d:%d",
                    SBN_Peer_GetSpacecraftID(PeerRec),
                    SBN_Peer_GetProcessorID(PeerRec));
    } /* end if */

    return Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 * NOTE: this is part of the SBN module interface
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Connection_HandleConnect(SBN_PeerInterface_t *Peer)
{
    static const char FAIL_PREFIX[] = "ERROR: could not connect peer:";

    if (Peer->Connected)
    {
        EVSSendErr(SBN_PEER_EID,
                   "%s peer %d:%d already connected",
                   FAIL_PREFIX,
                   Peer->SpacecraftID,
                   (int)(Peer->ProcessorID));
        return SBN_ERROR;
    } /* end if */

    EVSSendInfo(SBN_PEER_EID, "Peer %d:%d connected.", (int)Peer->SpacecraftID, (int)(Peer->ProcessorID));

    Peer->Connected = true;

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 * NOTE: this is part of the SBN module interface
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Connection_HandleDisconnect(SBN_PeerInterface_t *Peer)
{
    static const char FAIL_PREFIX[] = "ERROR: could not disconnect peer:";

    if (!Peer->Connected)
    {
        EVSSendErr(SBN_PEER_EID,
                   "%s already not connected to peer %d:%d",
                   FAIL_PREFIX,
                   Peer->SpacecraftID,
                   (int)(Peer->ProcessorID));
        return SBN_ERROR;
    }

    Peer->Connected = false; /* mark as disconnected */

    EVSSendInfo(SBN_PEER_EID, "Disconnected from peer %d:%d.", Peer->SpacecraftID, (int)(Peer->ProcessorID));

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Connection_CheckPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD)
{
    SBN_Status_t         SBN_Status;
    SBN_PeerInterface_t *Peer;
    const SBN_IfOps_t   *IfOps;

    Peer = SBN_Peer_GetIntfData(PeerRec);

    IfOps = Peer->Net->IfOps;

    /* Poll peer here to detect disconnections */
    if (IfOps && IfOps->PollPeer)
    {
        SBN_Status = IfOps->PollPeer(Peer);
    }
    else
    {
        SBN_Status = SBN_SUCCESS;
    }

    if (SBN_Status != SBN_SUCCESS)
    {
        EVSSendErr(SBN_PEERTASK_EID, "failed to poll peer %d:%d", (int)Peer->SpacecraftID, (int)Peer->ProcessorID);
    }
    else if (Peer->Connected && !PeerRec->Priv.ProtoMsgSent)
    {
        SBN_Status                 = SBN_Connection_SendProtocolVersionMsg(BufD, PeerRec);
        PeerRec->Priv.ProtoMsgSent = (SBN_Status == SBN_SUCCESS);
    }

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Checks for peers that have disconnected and updates state accordingly
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Connection_DoPollCycle(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    if (!PeerRec->Intf.Connected)
    {
        /* this will need to be re-sent */
        PeerRec->Priv.ProtoMsgSent = false;
    }

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Connection_PollCycle(void)
{
    SBN_ForEachConfiguredPeer(SBN_Connection_DoPollCycle, NULL);
}
