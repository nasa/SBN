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
#include <string.h>

#include "sbn_pack.h"
#include "sbn_app.h"
#include "cfe_es.h" /* PerfLog */
#include "cfe_platform_cfg.h"
#include "cfe_msgids.h"
#include "sbn_error.h"
#include "sbn_tbl.h"
#include "sbn_dispatch.h"
#include "sbn_module.h"

/** \brief SBN global application data, indexed by AppID. */
SBN_AppData_t SBN_AppData;

static SBN_Status_t SBN_UnloadNets(void);

/**
 * Packs a CCSDS message with an SBN message header.
 *
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in network (big-endian) byte order.
 * \param[out] SBNBuf The buffer to pack into.
 * \param[in] MsgSz The size of the payload.
 * \param[in] MsgType The SBN message type.
 * \param[in] ProcessorID The ProcessorID of the sender (should be CFE_CPU_ID)
 * \param[in] SpacecraftID The SpacecraftID of the sender
 * \param[in] Msg The payload (CCSDS message or SBN sub/unsub.)
 */
void SBN_PackMsg(void             *SBNBuf,
                 SBN_MsgSz_t       MsgSz,
                 SBN_MsgType_t     MsgType,
                 SBN_ProcessorID_t ProcessorID,
                 SBN_ProcessorID_t SpacecraftID,
                 void             *Msg)
{
    SBN_PackState_t Pack;
    SBN_Pack_Init(&Pack, SBNBuf, MsgSz + SBN_PACKED_HDR_SZ, false);

    SBN_Pack_UInt32(&Pack, MsgSz);
    SBN_Pack_UInt8(&Pack, MsgType);
    SBN_Pack_UInt32(&Pack, ProcessorID);
    SBN_Pack_UInt32(&Pack, SpacecraftID);

    if (!Msg || !MsgSz)
    {
        /* valid to have a NULL pointer/empty size payload */
        return;
    } /* end if */

    SBN_Pack_Data(&Pack, Msg, MsgSz);
}

/**
 * Unpacks a CCSDS message with an SBN message header.
 *
 * \param[in] SBNBuf The buffer to unpack.
 * \param[out] MsgSzPtr The payload size.
 * \param[out] MsgTypePtr The SBN message type.
 * \param[out] ProcessorIDPtr The ProcessorID of the sender.
 * \param[out] SpacecraftIDPtr The SpacecraftID of the sender.
 * \param[out] Msg The payload (a CCSDS message, or SBN sub/unsub).
 * \return true if we were able to unpack the message.
 *
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in platform byte order.
 * \todo Use a type for SBNBuf.
 */
bool SBN_UnpackMsg(void               *SBNBuf,
                   SBN_MsgSz_t        *MsgSzPtr,
                   SBN_MsgType_t      *MsgTypePtr,
                   SBN_ProcessorID_t  *ProcessorIDPtr,
                   SBN_SpacecraftID_t *SpacecraftIDPtr,
                   void               *Msg)
{
    uint8             t;
    SBN_UnpackState_t Pack;

    SBN_Unpack_Init(&Pack, SBNBuf, SBN_MAX_PACKED_MSG_SZ);

    *MsgSzPtr = 0;
    SBN_Unpack_UInt32(&Pack, MsgSzPtr);

    t = 0;
    SBN_Unpack_UInt8(&Pack, &t);
    *MsgTypePtr = t;

    SBN_Unpack_UInt32(&Pack, ProcessorIDPtr);
    SBN_Unpack_UInt32(&Pack, SpacecraftIDPtr);

    if (!*MsgSzPtr)
    {
        return true;
    } /* end if */

    if (*MsgSzPtr < 0 || *MsgSzPtr > CFE_MISSION_SB_MAX_SB_MSG_SIZE)
    {
        return false;
    } /* end if */

    SBN_Unpack_Data(&Pack, Msg, *MsgSzPtr);

    return true;
}

/**
 * Called by a protocol module to signal that a peer has been connected.
 *
 * @param[in] Peer The peer that has been connected.
 * @return SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Connected(SBN_PeerInterface_t *Peer)
{
    static const char FAIL_PREFIX[] = "ERROR: could not disconnect peer:";
    SBN_Status_t      SBN_Status;
    CFE_Status_t      CFE_Status;
    char              PipeName[OS_MAX_API_NAME];
    uint8             ProtocolVer;

    if (Peer->Connected != 0)
    {
        EVSSendErr(SBN_PEER_EID,
                   "%s peer %d:%d already connected",
                   FAIL_PREFIX,
                   Peer->SpacecraftID,
                   (int)(Peer->ProcessorID));
        return SBN_ERROR;
    } /* end if */

    /* create a pipe name string similar to SBN_0_Pipe */
    snprintf(PipeName, OS_MAX_API_NAME, "SBN_%d_%d_Pipe", (int)(Peer->ProcessorID), (int)(Peer->SpacecraftID));
    CFE_Status = CFE_SB_CreatePipe(&(Peer->Pipe), SBN_PLATFORM_PEER_PIPE_DEPTH, PipeName);

    if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "%s: could not create peer pipe '%s'", FAIL_PREFIX, PipeName);

        return SBN_ERROR;
    } /* end if */

    EVSSendInfo(SBN_PEER_EID, "Created peer pipe '%s'", PipeName);

    CFE_Status = CFE_SB_SetPipeOpts(Peer->Pipe, CFE_SB_PIPEOPTS_IGNOREMINE);
    if (CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "%s: could not set pipe options '%s'", FAIL_PREFIX, PipeName);

        return SBN_ERROR;
    } /* end if */

    EVSSendInfo(SBN_PEER_EID, "Peer %d:%d connected.", Peer->SpacecraftID, (int)(Peer->ProcessorID));

    ProtocolVer = SBN_PROTO_VER;
    SBN_Status  = SBN_SendNetMsg(SBN_PROTO_MSG, sizeof(ProtocolVer), &ProtocolVer, Peer);
    if (SBN_Status != SBN_SUCCESS)
    {
        return SBN_Status;
    } /* end if */

    /* set this to current time so we don't think we've already timed out */
    OS_GetLocalTime(&Peer->LastRecv);

    SBN_Status = SBN_SendLocalSubsToPeer(SBN_Peer_GetPrivateData(Peer));

    Peer->Connected = 1;

    return SBN_Status;
} /* end SBN_Connected() */

/**
 * Called by a protocol module to signal that a peer has been disconnected.
 *
 * @param[in] Peer The peer that has been disconnected.
 * @return SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Disconnected(SBN_PeerInterface_t *Peer)
{
    static const char FAIL_PREFIX[] = "ERROR: could not disconnect peer:";
    CFE_Status_t      CFE_Status;

    if (Peer->Connected == 0)
    {
        EVSSendErr(SBN_PEER_EID,
                   "%s already not connected to peer %d:%d",
                   FAIL_PREFIX,
                   Peer->SpacecraftID,
                   (int)(Peer->ProcessorID));
        return SBN_ERROR;
    }

    Peer->Connected = 0; /**< mark as disconnected before deleting pipe */

    if ((CFE_Status = CFE_SB_DeletePipe(Peer->Pipe)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID,
                   "%s could not delete pipe when disconnecting peer %d:%d: 0x%08x",
                   FAIL_PREFIX,
                   Peer->SpacecraftID,
                   Peer->ProcessorID,
                   CFE_Status);
    }
    Peer->Pipe = CFE_SB_INVALID_PIPE;

    memset(&Peer->Counts, 0, sizeof(Peer->Counts));

    Peer->SubCnt = 0; /* reset sub count, in case this is a reconnection */

    EVSSendInfo(SBN_PEER_EID, "Disconnected from peer %d:%d.", Peer->SpacecraftID, (int)(Peer->ProcessorID));

    return SBN_SUCCESS;
} /* end SBN_Disconnected() */

SBN_Status_t SBN_MatchRecvPeerToTask(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    SBN_RecvPeerTaskData_t *D = Arg;
    SBN_NetRecord_t        *NetRec;
    SBN_PeerInterface_t    *Peer;

    Peer = SBN_Peer_GetIntfData(PeerRec);

    if (CFE_RESOURCEID_TEST_EQUAL(Peer->RecvTaskID, D->RecvTaskID))
    {
        NetRec = SBN_LocateNetRecordByID(PeerRec->NetId);

        if (SBN_NetRecordIsMatch(NetRec, PeerRec->NetId))
        {
            D->Peer   = Peer;
            D->Net    = SBN_Net_GetIntfData(NetRec);
            D->PeerId = SBN_PeerRecordGetID(PeerRec);
            D->NetId  = PeerRec->NetId;

            return SBN_FINISHED; /* just needs to be nonzero for iteration to stop */
        }
    }

    return SBN_SUCCESS;
}

/**
 * \brief Receive task created for each direct peer-based connection.
 * Spanwed from SBN_PeerPoll()
 */
void SBN_RecvPeerTask(void)
{
    SBN_RecvPeerTaskData_t D;
    memset(&D, 0, sizeof(D));

    CFE_ES_GetTaskID(&D.RecvTaskID);

    SBN_ForEachConfiguredPeer(SBN_MatchRecvPeerToTask, &D);

    if (D.Peer == NULL)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unable to connect task to peer struct");
        return;
    } /* end if */

    while (1)
    {
        /* Reset the buffer descriptor */
        memset(&D.BufD, 0, sizeof(D.BufD));
        D.BufD.MaxDataSz = sizeof(D.NetMsgBuf);
        D.BufD.DataPtr   = D.NetMsgBuf;

        D.Status = D.Net->IfOps->RecvFromPeer(D.Net,
                                              D.Peer,
                                              &D.BufD.MsgType,
                                              &D.BufD.ActualDataSz,
                                              &D.BufD.ProcessorID,
                                              &D.BufD.SpacecraftID,
                                              &D.NetMsgBuf);

        if (D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        } /* end if */

        if (D.Status == SBN_SUCCESS)
        {
            OS_GetLocalTime(&D.Peer->LastRecv);

            D.Status = SBN_ProcessPeerMsg(SBN_Peer_GetPrivateData(D.Peer), &D.BufD);

            if (D.Status != SBN_SUCCESS)
            {
                D.Peer->RecvTaskID = CFE_ES_TASKID_UNDEFINED;
                return;
            } /* end if */
        }
        else
        {
            EVSSendErr(SBN_PEER_EID, "recv error (%d)", D.Status);
            D.Peer->Counts.RecvErrCnt++;
            D.Peer->RecvTaskID = CFE_ES_TASKID_UNDEFINED;
            return;
        } /* end if */
    } /* end while */
} /* end SBN_RecvPeerTask() */

SBN_Status_t SBN_MatchRecvNetToTask(SBN_NetRecord_t *NetRec, void *Arg)
{
    SBN_RecvNetTaskData_t *D = Arg;
    SBN_NetInterface_t    *Net;

    Net = SBN_Net_GetIntfData(NetRec);

    if (CFE_RESOURCEID_TEST_EQUAL(Net->RecvTaskID, D->RecvTaskID))
    {
        D->Net   = Net;
        D->NetId = SBN_NetRecordGetID(NetRec);

        return SBN_FINISHED; /* just needs to be nonzero for iteration to stop */
    }

    return SBN_SUCCESS;
}

/**
 * \brief Receive task created for each net-based connection.
 * Spanwed from SBN_PeerPoll()
 */
void SBN_RecvNetTask(void)
{
    static const char     FAIL_PREFIX_STARTUP[] = "ERROR: could not start SBN Receive Net Task:";
    static const char     FAIL_PREFIX_RUNNING[] = "ERROR: during SBN Receive Net Task:";
    SBN_RecvNetTaskData_t D;
    memset(&D, 0, sizeof(D));

    CFE_ES_GetTaskID(&D.RecvTaskID);

    SBN_ForEachConfiguredNet(SBN_MatchRecvNetToTask, &D);

    if (D.Net == NULL)
    {
        EVSSendErr(SBN_PEERTASK_EID, "%s unable to connect task to net struct", FAIL_PREFIX_STARTUP);
        return;
    } /* end if */

    while (1)
    {
        /* Reset the buffer descriptor */
        memset(&D.BufD, 0, sizeof(D.BufD));
        D.BufD.MaxDataSz = sizeof(D.NetMsgBuf);
        D.BufD.DataPtr   = D.NetMsgBuf;

        EVSSendDbg(SBN_PEERTASK_EID, "Try to receive from net...");
        D.Status = D.Net->IfOps->RecvFromNet(D.Net,
                                             &D.BufD.MsgType,
                                             &D.BufD.ActualDataSz,
                                             &D.BufD.ProcessorID,
                                             &D.BufD.SpacecraftID,
                                             &D.NetMsgBuf);

        if (D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        } /* end if */

        if (D.Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_PEERTASK_EID,
                       "%s RecvFromNet failed for net %d: status=0x%08X",
                       FAIL_PREFIX_RUNNING,
                       SBN_NetId_AsInt(D.NetId),
                       D.Status);
            break;
        } /* end if */

        D.Peer = SBN_GetPeer(D.Net, D.BufD.ProcessorID, D.BufD.SpacecraftID);
        if (!D.Peer)
        {
            EVSSendErr(SBN_PEERTASK_EID, "unknown peer (ProcessorID=%d)", (int)D.BufD.ProcessorID);
            break;
        } /* end if */

        OS_GetLocalTime(&D.Peer->LastRecv);

        D.Status = SBN_ProcessPeerMsg(SBN_Peer_GetPrivateData(D.Peer), &D.BufD);

        if (D.Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_PEERTASK_EID, "SBN_ProcessPeerMsg failed: 0x%08X", D.Status);
            break;
        } /* end if */
    } /* end while */

    /* Unset the task id so that it can be created if necessary */
    D.Net->RecvTaskID = CFE_ES_TASKID_UNDEFINED;
} /* end SBN_RecvNetTask() */

/**
 * Checks all interfaces for messages from peers.
 * Receive messages from the specified peer, injecting them onto the local
 * software bus.
 */
SBN_Status_t SBN_RecvNetMsgs(void)
{
    SBN_Status_t         SBN_Status;
    SBN_PeerRecord_t    *PeerRec;
    SBN_PeerInterface_t *Peer;
    uint8                MsgCnt;
    SBN_NetIdx_t         NetIdx;
    SBN_NetRecord_t     *NetRec;
    SBN_PeerIdx_t        PeerIdx;
    SBN_NetInterface_t  *Net;
    SBN_BufferDesc_t     BufD;

    for (NetIdx = 0; NetIdx < SBN_AppData.NetCnt; NetIdx++)
    {
        NetRec = SBN_LocateNetRecordByExtIdx(NetIdx);
        Net    = SBN_Net_GetIntfData(NetRec);

        if (Net == NULL || !Net->Configured || Net->TaskFlags & SBN_TASK_RECV)
        {
            continue; /* separate task handles receiving from a net */
        } /* end if */

        if (Net->IfOps->RecvFromNet)
        {
            // TODO: make configurable
            for (MsgCnt = 0; MsgCnt < 100; MsgCnt++) /* read at most 100 messages from the net */
            {
                /* Reset Descriptor */
                memset(&BufD, 0, sizeof(BufD));
                BufD.MaxDataSz = sizeof(SBN_AppData.MsgBuffer);
                BufD.DataPtr   = SBN_AppData.MsgBuffer;

                SBN_Status = Net->IfOps->RecvFromNet(Net,
                                                     &BufD.MsgType,
                                                     &BufD.ActualDataSz,
                                                     &BufD.ProcessorID,
                                                     &BufD.SpacecraftID,
                                                     BufD.DataPtr);

                if (SBN_Status == SBN_IF_EMPTY)
                {
                    break; /* no (more) messages for this net, continue to next net */
                } /* end if */

                /* for UDP, the message received may not be from the peer
                 * expected.
                 */
                PeerRec = SBN_GetPeerRecord(NetRec, BufD.ProcessorID, BufD.SpacecraftID);

                if (!PeerRec)
                {
                    EVSSendInfo(SBN_PEERTASK_EID, "unknown peer (ProcessorID=%d)", (int)BufD.ProcessorID);
                    /* may be a misconfiguration on my part...? continue processing msgs... */
                    continue;
                } /* end if */

                Peer = SBN_Peer_GetIntfData(PeerRec);
                OS_GetLocalTime(&Peer->LastRecv);
                SBN_ProcessPeerMsg(PeerRec, &BufD);
            } /* end for */
        }
        else if (Net->IfOps->RecvFromPeer)
        {
            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                PeerRec = SBN_LocatePeerRecordInNet(NetRec, PeerIdx);
                if (PeerRec == NULL)
                {
                    continue;
                }

                Peer = SBN_Peer_GetIntfData(PeerRec);

                // TODO: make configurable
                for (MsgCnt = 0; MsgCnt < 100; MsgCnt++) /* read at most 100 messages from peer */
                {
                    /* Reset Descriptor */
                    memset(&BufD, 0, sizeof(BufD));
                    BufD.MaxDataSz = sizeof(SBN_AppData.MsgBuffer);
                    BufD.DataPtr   = SBN_AppData.MsgBuffer;

                    memset(SBN_AppData.MsgBuffer, 0, sizeof(SBN_AppData.MsgBuffer));

                    SBN_Status = Net->IfOps->RecvFromPeer(Net,
                                                          Peer,
                                                          &BufD.MsgType,
                                                          &BufD.ActualDataSz,
                                                          &BufD.ProcessorID,
                                                          &BufD.SpacecraftID,
                                                          BufD.DataPtr);

                    if (SBN_Status == SBN_IF_EMPTY)
                    {
                        break; /* no (more) messages for this peer, continue to next peer */
                    } /* end if */

                    OS_GetLocalTime(&Peer->LastRecv);

                    SBN_Status = SBN_ProcessPeerMsg(PeerRec, &BufD);

                    if (SBN_Status != SBN_SUCCESS)
                    {
                        break; /* continue to next peer */
                    } /* end if */
                } /* end for */
            } /* end for */
        }
        else
        {
            EVSSendErr(SBN_PEER_EID, "neither RecvFromPeer nor RecvFromNet defined for net #%d", (int)NetIdx);

            /* meanwhile, continue to next net... */
        } /* end if */
    } /* end for */

    return SBN_SUCCESS;
} /* end SBN_RecvNetMsgs */

/**
 * Sends a message to a peer using the module's send API.
 *
 * @param MsgType SBN type of the message
 * @param MsgSz Size of the message
 * @param Msg Message to send
 * @param Peer The peer to send the message to.
 * @return Number of characters sent on success, -1 on error.
 *
 */
SBN_Status_t SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer)
{
    SBN_NetInterface_t *Net;
    SBN_Status_t        SBN_Status;

    Net = Peer->Net;

    if (CFE_RESOURCEID_TEST_DEFINED(Peer->SendTaskID))
    {
        if (OS_MutSemTake(SBN_AppData.SendMutex) != OS_SUCCESS)
        {
            EVSSendErr(SBN_PEER_EID, "unable to take send mutex");
            return SBN_ERROR;
        } /* end if */
    } /* end if */

    SBN_Status = Net->IfOps->Send(Peer, MsgType, MsgSz, Msg);

    if (SBN_Status != SBN_SUCCESS)
    {
        Peer->Counts.SendErrCnt++;
    }
    else
    {
        Peer->Counts.SendCnt++;
    } /* end if */

    /* for clients that need a poll or heartbeat, update time even when failing */
    OS_GetLocalTime(&Peer->LastSend);

    if (CFE_RESOURCEID_TEST_DEFINED(Peer->SendTaskID))
    {
        if (OS_MutSemGive(SBN_AppData.SendMutex) != OS_SUCCESS)
        {
            EVSSendErr(SBN_PEER_EID, "unable to give send mutex");
            return SBN_ERROR;
        } /* end if */
    } /* end if */

    return SBN_Status;
} /* end SBN_SendNetMsg */

SBN_Status_t SBN_MatchSendPeerToTask(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    SBN_SendTaskData_t  *D = Arg;
    SBN_PeerInterface_t *Peer;

    Peer = SBN_Peer_GetIntfData(PeerRec);

    if (CFE_RESOURCEID_TEST_EQUAL(Peer->SendTaskID, D->SendTaskID))
    {
        D->Peer   = Peer;
        D->PeerId = SBN_PeerRecordGetID(PeerRec);
        D->NetId  = PeerRec->NetId;

        return SBN_FINISHED; /* just needs to be nonzero for iteration to stop */
    }

    return SBN_SUCCESS;
}

/**
 * \brief When a peer is connected, a task is created to listen to the relevant
 * pipe for messages to send to that peer.
 */
void SBN_SendTask(void)
{
    SBN_SendTaskData_t D;
    SBN_Filter_Ctx_t   Filter_Context;
    CFE_MSG_Size_t     MsgSz;
    SBN_MsgSz_t        SBN_MsgSz;
    SBN_ModuleIdx_t    FilterIdx;

    MsgSz                         = 0;
    Filter_Context.MyProcessorID  = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID = CFE_PSP_GetSpacecraftId();

    memset(&D, 0, sizeof(D));

    CFE_ES_GetTaskID(&D.SendTaskID);
    SBN_ForEachConfiguredPeer(SBN_MatchSendPeerToTask, &D);

    if (D.Peer == NULL)
    {
        EVSSendErr(SBN_PEER_EID, "error connecting send task");
        return;
    } /* end if */

    while (1)
    {
        if (!D.Peer->Connected)
        {
            OS_TaskDelay(SBN_PLATFORM_MAIN_LOOP_DELAY);
            continue;
        } /* end if */

        if (CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&D.MsgPtr, D.Peer->Pipe, CFE_SB_PEND_FOREVER) != CFE_SUCCESS)
        {
            break;
        } /* end if */

        Filter_Context.PeerProcessorID  = D.Peer->ProcessorID;
        Filter_Context.PeerSpacecraftID = D.Peer->SpacecraftID;

        for (FilterIdx = 0; FilterIdx < D.Peer->FilterCnt; FilterIdx++)
        {
            if (D.Peer->Filters[FilterIdx]->FilterSend == NULL)
            {
                continue;
            } /* end if */

            D.Status = (D.Peer->Filters[FilterIdx]->FilterSend)(D.MsgPtr, &Filter_Context);
            if (D.Status == SBN_IF_EMPTY) /* filter requests not sending this msg, see below for loop */
            {
                break;
            } /* end if */

            if (D.Status != SBN_SUCCESS)
            {
                break;
            } /* end if */
        } /* end for */

        if (FilterIdx < D.Peer->FilterCnt)
        {
            /* one of the above filters suggested rejecting this message */
            continue;
        } /* end if */

        if (CFE_MSG_GetSize(D.MsgPtr, &MsgSz) != CFE_SUCCESS)
        {
            continue;
        } /* end if */

        SBN_MsgSz = (SBN_MsgSz_t)MsgSz;

        D.Status = SBN_SendNetMsg(SBN_APP_MSG, SBN_MsgSz, D.MsgPtr, D.Peer);

        if (D.Status == SBN_ERROR)
        {
            break;
        } /* end if */
    } /* end while */

    /* mark peer as not having a task so that sending will create a new one */
    D.Peer->SendTaskID = CFE_ES_TASKID_UNDEFINED;
} /* end SBN_SendTask() */

/**
 * Iterate through all peers, examining the pipe to see if there are messages
 * I need to send to that peer.
 */
static SBN_Status_t SBN_CheckPeerPipes(void)
{
    SBN_Status_t         SBN_Status;
    CFE_Status_t         CFE_Status;
    int                  ReceivedFlag;
    int                  iter;
    CFE_MSG_Message_t   *MsgPtr;
    CFE_MSG_Size_t       MsgSz;
    SBN_MsgSz_t          SBN_MsgSz;
    SBN_Filter_Ctx_t     Filter_Context;
    SBN_NetIdx_t         NetIdx;
    SBN_NetRecord_t     *NetRec;
    SBN_NetInterface_t  *Net;
    SBN_PeerIdx_t        PeerIdx;
    SBN_ModuleIdx_t      FilterIdx;
    SBN_PeerRecord_t    *PeerRec;
    SBN_PeerInterface_t *Peer;
    char                 SendTaskName[32];

    MsgPtr                        = NULL;
    MsgSz                         = 0;
    Filter_Context.MyProcessorID  = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID = CFE_PSP_GetSpacecraftId();

    /**
     * \note This processes one message per peer, then start again until no
     * peers have pending messages. At max only process SBN_PLATFORM_MAX_MSG_PER_WAKEUP
     * per peer per wakeup otherwise I will starve other processing.
     */
    for (iter = 0; iter < SBN_PLATFORM_MAX_MSG_PER_WAKEUP; iter++)
    {
        ReceivedFlag = 0;

        for (NetIdx = 0; NetIdx < SBN_AppData.NetCnt; NetIdx++)
        {
            NetRec = SBN_LocateNetRecordByExtIdx(NetIdx);
            Net    = SBN_Net_GetIntfData(NetRec);
            if (Net == NULL || !Net->Configured)
            {
                continue;
            }

            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                PeerRec = SBN_LocatePeerRecordInNet(NetRec, PeerIdx);
                Peer    = SBN_Peer_GetIntfData(PeerRec);
                if (Peer == NULL)
                {
                    continue;
                }

                // Poll peer here to detect disconnections and to reconnect
                if (Net->IfOps->PollPeer(Peer) != SBN_SUCCESS)
                {
                    EVSSendErr(SBN_PEERTASK_EID, "failed to poll peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
                }

                if (Peer->Connected == 0)
                {
                    EVSSendDbg(SBN_PEERTASK_EID, "not connected to peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
                    continue;
                } /* end if */

                EVSSendDbg(SBN_PEERTASK_EID, "SBN connected to peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);

                if (Peer->TaskFlags & SBN_TASK_SEND)
                {
                    if (!CFE_RESOURCEID_TEST_DEFINED(Peer->SendTaskID))
                    {
                        /* TODO: logic/controls to prevent hammering? */
                        snprintf(SendTaskName,
                                 32,
                                 "sendT_%d_%d_%d",
                                 (int)NetIdx,
                                 (int)(Peer->ProcessorID),
                                 (int)(Peer->SpacecraftID));

                        CFE_Status =
                            CFE_ES_CreateChildTask(&(Peer->SendTaskID),
                                                   SendTaskName,
                                                   (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_SendTask,
                                                   NULL,
                                                   CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(SBN_SendTaskData_t),
                                                   0,
                                                   0);

                        if (CFE_Status != CFE_SUCCESS)
                        {
                            EVSSendErr(SBN_PEER_EID,
                                       "error creating send task for peer %d:%d",
                                       Peer->SpacecraftID,
                                       Peer->ProcessorID);

                            return SBN_ERROR;
                        } /* end if */
                    } /* end if */

                    continue;
                } /* end if */

                /* if peer data is not in use, go to next peer */
                if (CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&MsgPtr, Peer->Pipe, CFE_SB_POLL) != CFE_SUCCESS)
                {
                    continue;
                } /* end if */

                ReceivedFlag = 1;

                Filter_Context.PeerProcessorID  = Peer->ProcessorID;
                Filter_Context.PeerSpacecraftID = Peer->SpacecraftID;

                for (FilterIdx = 0; FilterIdx < Peer->FilterCnt; FilterIdx++)
                {
                    if (Peer->Filters[FilterIdx]->FilterSend == NULL)
                    {
                        continue;
                    } /* end if */

                    SBN_Status = (Peer->Filters[FilterIdx]->FilterSend)(MsgPtr, &Filter_Context);

                    if (SBN_Status == SBN_IF_EMPTY) /* filter requests not sending this msg, see below for loop */
                    {
                        break;
                    } /* end if */

                    if (SBN_Status != SBN_SUCCESS)
                    {
                        /* something fatal happened, exit */
                        return SBN_Status;
                    } /* end if */
                } /* end for */

                if (FilterIdx < Peer->FilterCnt)
                {
                    /* one of the above filters suggested rejecting this message */
                    continue;
                } /* end if */

                if (CFE_MSG_GetSize(MsgPtr, &MsgSz) != CFE_SUCCESS)
                {
                    continue;
                } /* end if */

                SBN_MsgSz = (SBN_MsgSz_t)MsgSz;

                SBN_SendNetMsg(SBN_APP_MSG, SBN_MsgSz, MsgPtr, Peer);
            } /* end for */
        } /* end for */

        if (!ReceivedFlag)
        {
            break;
        } /* end if */
    } /* end for */
    return SBN_SUCCESS;
} /* end SBN_CheckPeerPipes */

/**
 * Iterate through all nets and create receive tasks if they do not yet exist.
 */
static SBN_Status_t SBN_PeerPoll(void)
{
    CFE_Status_t         CFE_Status;
    SBN_NetIdx_t         NetIdx;
    SBN_NetRecord_t     *NetRec;
    SBN_NetInterface_t  *Net;
    char                 RecvTaskName[32];
    SBN_PeerIdx_t        PeerIdx;
    SBN_PeerRecord_t    *PeerRec;
    SBN_PeerInterface_t *Peer;

    for (NetIdx = 0; NetIdx < SBN_AppData.NetCnt; NetIdx++)
    {
        NetRec = SBN_LocateNetRecordByExtIdx(NetIdx);
        Net    = SBN_Net_GetIntfData(NetRec);
        if (Net == NULL || !Net->Configured)
        {
            continue;
        }

        if (Net->IfOps->RecvFromNet && Net->TaskFlags & SBN_TASK_RECV)
        {
            if (!CFE_RESOURCEID_TEST_DEFINED(Net->RecvTaskID))
            {
                EVSSendInfo(SBN_PEER_EID, "Creating recv task for net %d", (int)NetIdx);

                /* TODO: add logic/controls to prevent hammering */
                snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_rs_%d", (int)NetIdx);
                CFE_Status =
                    CFE_ES_CreateChildTask(&(Net->RecvTaskID),
                                           RecvTaskName,
                                           (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_RecvNetTask,
                                           NULL,
                                           CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(SBN_RecvNetTaskData_t),
                                           0,
                                           0);

                if (CFE_Status != CFE_SUCCESS)
                {
                    EVSSendErr(SBN_PEER_EID, "error creating task for net %d", (int)NetIdx);
                    return SBN_ERROR;
                } /* end if */
            } /* end if */
        }
        else
        {
            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                PeerRec = SBN_LocatePeerRecordInNet(NetRec, PeerIdx);
                Peer    = SBN_Peer_GetIntfData(PeerRec);
                if (Peer == NULL)
                {
                    continue;
                }

                if (Net->IfOps->RecvFromPeer && Peer->TaskFlags & SBN_TASK_RECV)
                {
                    if (!CFE_RESOURCEID_TEST_DEFINED(Peer->RecvTaskID))
                    {
                        /* TODO: add logic/controls to prevent hammering */
                        snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_recv_%d", (int)PeerIdx);
                        CFE_Status = CFE_ES_CreateChildTask(&(Peer->RecvTaskID),
                                                            RecvTaskName,
                                                            (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_RecvPeerTask,
                                                            NULL,
                                                            CFE_PLATFORM_ES_DEFAULT_STACK_SIZE
                                                                + 2 * sizeof(SBN_RecvPeerTaskData_t),
                                                            0,
                                                            0);
                        /* TODO: more accurate stack size required */

                        if (CFE_Status != CFE_SUCCESS)
                        {
                            EVSSendErr(SBN_PEER_EID, "error creating task for %d", (int)(Peer->ProcessorID));
                            return SBN_ERROR;
                        } /* end if */
                    } /* end if */
                }
                else
                {
                    Net->IfOps->PollPeer(Peer);
                } /* end if */
            } /* end for */
        } /* end if */
    } /* end for */

    return SBN_SUCCESS;
} /* end SBN_PeerPoll */

/**
 * Loops through all hosts and peers, initializing all.
 *
 * @return SBN_SUCCESS if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
static SBN_Status_t SBN_InitInterfaces(void)
{
    SBN_NetIdx_t         NetIdx;
    SBN_NetRecord_t     *NetRec;
    SBN_NetInterface_t  *Net;
    SBN_PeerIdx_t        PeerIdx;
    SBN_PeerRecord_t    *PeerRec;
    SBN_PeerInterface_t *Peer;
    SBN_NetIdx_t         NetCnt;

    NetCnt = 0;
    for (NetIdx = 0; NetIdx < SBN_AppData.NetCnt; NetIdx++)
    {
        NetRec = SBN_LocateNetRecordByExtIdx(NetIdx);
        Net    = SBN_Net_GetIntfData(NetRec);
        if (Net == NULL || !Net->Configured)
        {
            EVSSendErr(SBN_INIT_EID, "network #%u not configured (skipping)", (unsigned int)NetIdx);
        }
        else
        {
            EVSSendInfo(SBN_INIT_EID, "initializing net: %u", (unsigned int)NetIdx);

            Net->IfOps->InitNet(Net);
            ++NetCnt;

            EVSSendInfo(SBN_INIT_EID, "Net %d has %d peers", NetIdx, Net->PeerCnt);
            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                PeerRec = SBN_LocatePeerRecordInNet(NetRec, PeerIdx);
                Peer    = SBN_Peer_GetIntfData(PeerRec);
                if (Peer == NULL)
                {
                    continue;
                }

                EVSSendInfo(SBN_PEER_EID,
                            "initializing net: %d peer: %d: sc: %d cpu: %d",
                            (int)NetIdx,
                            (int)PeerIdx,
                            Peer->SpacecraftID,
                            Peer->ProcessorID);

                Net->IfOps->InitPeer(Peer);
            }
        }
    } /* end for */

    EVSSendInfo(SBN_INIT_EID, "configured, %d nets", (int)NetCnt);

    return SBN_SUCCESS;
} /* end SBN_InitInterfaces */

/**
 * This function waits for the scheduler (SCH) to wake this code up, so that
 * nothing transpires until the cFE is fully operational.
 *
 * @param[in] iTimeOut The time to wait for the scheduler to notify this code.
 * @return CFE_SUCCESS on success, otherwise an error value.
 */
static SBN_Status_t SBN_WaitForWakeup(int32 iTimeOut)
{
    CFE_Status_t           CFE_Status;
    SBN_Status_t           SBN_Status;
    const CFE_SB_Buffer_t *BufPtr;

    /* Wait for WakeUp messages from scheduler */
    CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&BufPtr, SBN_AppData.CmdPipe, iTimeOut);

    switch (CFE_Status)
    {
        case CFE_SB_NO_MESSAGE:
        case CFE_SB_TIME_OUT:
            break;
        case CFE_SUCCESS:
            SBN_HandleCommand(BufPtr);
            break;
        default:
            return SBN_ERROR;
    } /* end switch */

    /* For sbn, we still want to perform cyclic processing
    ** if the SBN_WaitForWakeup time out
    ** cyclic processing at timeout rate
    */
    CFE_ES_PerfLogEntry(SBN_PERF_RECV_ID);

    SBN_RecvNetMsgs();

    SBN_Status = SBN_CheckSubscriptionPipe();
    switch (SBN_Status)
    {
        case SBN_IF_EMPTY:
            break;
        case SBN_ERROR:
            EVSSendErr(SBN_PEER_EID, "SBN_CheckSubscriptionPipe failed.");
            break;
        default:
            /* no error */
            break;
    };

    SBN_CheckPeerPipes();

    SBN_PeerPoll();

    CFE_ES_PerfLogExit(SBN_PERF_RECV_ID);

    return SBN_SUCCESS;
} /* end SBN_WaitForWakeup */

SBN_Status_t SBN_ValidateConfTbl(void *TableData)
{
    const SBN_ConfTbl_t    *PendingTbl = TableData;
    const SBN_Peer_Entry_t *PeerConf;
    SBN_ModuleIdx_t         ModuleIdx;
    SBN_PeerIdx_t           PeerIdx;
    SBN_PeerIdx_t           PeerCounts[SBN_PLATFORM_MAX_NETS];
    const char             *NetProtoName[SBN_PLATFORM_MAX_NETS];
    uint32                  NumErrs;

    if (PendingTbl->ProtocolCnt > SBN_MISSION_MAX_MOD_CNT || PendingTbl->FilterCnt > SBN_MISSION_MAX_MOD_CNT
        || PendingTbl->PeerCnt > SBN_MISSION_MAX_PEER_CNT)
    {
        /* Invalid count */
        return SBN_ERROR;
    }

    memset(PeerCounts, 0, sizeof(PeerCounts));
    memset(NetProtoName, 0, sizeof(NetProtoName));
    NumErrs = 0;

    for (PeerIdx = 0; PeerIdx < PendingTbl->PeerCnt; ++PeerIdx)
    {
        PeerConf = &PendingTbl->Peers[PeerIdx];

        /* Make sure the peer refers to a valid protocol in the protocol section */
        for (ModuleIdx = 0; ModuleIdx < PendingTbl->ProtocolCnt; ++ModuleIdx)
        {
            if (strncmp(PendingTbl->ProtocolModules[ModuleIdx].Name,
                        PeerConf->ProtocolName,
                        sizeof(PeerConf->ProtocolName))
                == 0)
            {
                break;
            }
        }

        if (ModuleIdx == PendingTbl->ProtocolCnt)
        {
            /* do not print the string since we do not know if its null terminated */
            EVSSendErr(SBN_TBL_EID, "invalid module name on peer %u", (unsigned int)PeerIdx);
            ++NumErrs;
        }

        if (PeerConf->NetNum >= SBN_PLATFORM_MAX_NETS)
        {
            EVSSendErr(SBN_TBL_EID, "network index too large (%d>%d)", PeerConf->NetNum, SBN_PLATFORM_MAX_NETS);
            ++NumErrs;
        } /* end if */
        else
        {
            ++PeerCounts[PeerConf->NetNum];

            if (NetProtoName[PeerConf->NetNum] == NULL)
            {
                NetProtoName[PeerConf->NetNum] = PeerConf->ProtocolName;
            }
            else if (strncmp(NetProtoName[PeerConf->NetNum], PeerConf->ProtocolName, sizeof(PeerConf->ProtocolName))
                     != 0)
            {
                /* do not print the string since we do not know if its null terminated */
                EVSSendErr(SBN_TBL_EID,
                           "protocol mismatch on peer %u (net %u)",
                           (unsigned int)PeerIdx,
                           (unsigned int)PeerConf->NetNum);
                ++NumErrs;
            }
        }
    }

    if (NumErrs > 0)
    {
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

static void SBN_InitProtoModules(const SBN_ConfTbl_t *ConfTbl)
{
    SBN_Status_t              Status;
    const SBN_Module_Entry_t *ModEntry;
    SBN_ModuleIdx_t           ModuleIdx;
    const SBN_IfOps_t        *ProtoOps;
    SBN_ProtocolModInfo_t    *LocalInfo;
    char                      SymbolName[CFE_MISSION_MAX_API_LEN];
    SBN_ProtocolOutlet_t      Outlet = { .PackMsg      = SBN_PackMsg,
                                         .UnpackMsg    = SBN_UnpackMsg,
                                         .Connected    = SBN_Connected,
                                         .Disconnected = SBN_Disconnected,
                                         .SendNetMsg   = SBN_SendNetMsg,
                                         .GetPeer      = SBN_GetPeer };

    /* load protocol modules */
    EVSSendDbg(SBN_TBL_EID, "Loading protocol modules...");
    ModEntry = ConfTbl->ProtocolModules;
    for (ModuleIdx = 0; ModuleIdx < SBN_MISSION_MAX_MOD_CNT && ModuleIdx < ConfTbl->ProtocolCnt; ++ModuleIdx)
    {
        LocalInfo = &SBN_AppData.ProtocolModInfo[ModuleIdx];
        CFE_SB_MessageStringGet(LocalInfo->ProtocolName,
                                ModEntry->Name,
                                "",
                                sizeof(LocalInfo->ProtocolName),
                                sizeof(ModEntry->Name));
        CFE_SB_MessageStringGet(SymbolName, ModEntry->LibSymbol, "", sizeof(SymbolName), sizeof(ModEntry->LibSymbol));

        Status = SBN_Module_GetProtocolIf(SymbolName, &ProtoOps);
        if (Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID,
                       "Failed to map protocol \'%s\' via symbol \'%s\' (at index %u)",
                       LocalInfo->ProtocolName,
                       SymbolName,
                       (unsigned int)ModuleIdx);
        }
        else
        {
            EVSSendInfo(SBN_TBL_EID, "initializing protocol module %s", LocalInfo->ProtocolName);
            Status = ProtoOps->InitModule(SBN_PROTOCOL_VERSION, ModEntry->BaseEID, &Outlet);
            if (Status != SBN_SUCCESS)
            {
                EVSSendErr(SBN_TBL_EID, "error in protocol init");
            }
            else
            {
                EVSSendInfo(SBN_TBL_EID, "protocol module %s initialized", LocalInfo->ProtocolName);
                LocalInfo->Intf = ProtoOps;
            }
        }
        ++ModEntry;

    } /* end for */
}

static void SBN_InitFilterModules(const SBN_ConfTbl_t *ConfTbl)
{
    SBN_Status_t                 Status;
    const SBN_Module_Entry_t    *ModEntry;
    SBN_ModuleIdx_t              ModuleIdx;
    const SBN_FilterInterface_t *FiltOps;
    SBN_FilterModInfo_t         *LocalInfo;
    char                         SymbolName[CFE_MISSION_MAX_API_LEN];

    /* load filter modules */
    EVSSendDbg(SBN_TBL_EID, "Loading filter modules...");
    ModEntry = ConfTbl->FilterModules;
    for (ModuleIdx = 0; ModuleIdx < SBN_MISSION_MAX_MOD_CNT && ModuleIdx < ConfTbl->FilterCnt; ++ModuleIdx)
    {
        LocalInfo = &SBN_AppData.FilterModInfo[ModuleIdx];
        CFE_SB_MessageStringGet(LocalInfo->FilterName,
                                ModEntry->Name,
                                "",
                                sizeof(LocalInfo->FilterName),
                                sizeof(ModEntry->Name));
        CFE_SB_MessageStringGet(SymbolName, ModEntry->LibSymbol, "", sizeof(SymbolName), sizeof(ModEntry->LibSymbol));

        Status = SBN_Module_GetFilterIf(SymbolName, &FiltOps);
        if (Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID,
                       "Failed to map filter \'%s\' via symbol \'%s\' (at index %u)",
                       LocalInfo->FilterName,
                       SymbolName,
                       (unsigned int)ModuleIdx);
        }
        else
        {
            EVSSendInfo(SBN_TBL_EID, "initializing filter module %s", LocalInfo->FilterName);
            Status = FiltOps->InitModule(SBN_FILTER_VERSION, ModEntry->BaseEID);
            if (Status != SBN_SUCCESS)
            {
                EVSSendErr(SBN_TBL_EID, "error in filter init");
            }
            else
            {
                EVSSendInfo(SBN_TBL_EID, "filter module %s initialized", LocalInfo->FilterName);
                LocalInfo->Intf = FiltOps;
            }
        }
        ++ModEntry;
    }
}

static const SBN_ProtocolModInfo_t *SBN_FindActiveProtocol(const char *ProtocolName)
{
    const SBN_ProtocolModInfo_t *ProtoEntry;
    SBN_ModuleIdx_t              ModuleIdx;

    ProtoEntry = SBN_AppData.ProtocolModInfo;
    ModuleIdx  = 0;

    while (true)
    {
        if (ModuleIdx >= SBN_MISSION_MAX_MOD_CNT)
        {
            ProtoEntry = NULL;
            break;
        }
        if (strcmp(ProtoEntry->ProtocolName, ProtocolName) == 0)
        {
            break;
        }
        ++ModuleIdx;
        ++ProtoEntry;
    }

    /* only return it if it was successfully set up */
    if (ProtoEntry != NULL && ProtoEntry->Intf == NULL)
    {
        ProtoEntry = NULL;
    }

    return ProtoEntry;
}

static const SBN_FilterModInfo_t *SBN_FindActiveFilter(const char *FilterName)
{
    const SBN_FilterModInfo_t *FilterEntry;
    SBN_ModuleIdx_t            ModuleIdx;

    FilterEntry = SBN_AppData.FilterModInfo;
    ModuleIdx   = 0;

    while (true)
    {
        if (ModuleIdx >= SBN_MISSION_MAX_MOD_CNT)
        {
            FilterEntry = NULL;
            break;
        }
        if (strcmp(FilterEntry->FilterName, FilterName) == 0)
        {
            break;
        }
        ++ModuleIdx;
        ++FilterEntry;
    }

    /* only return it if it was successfully set up */
    if (FilterEntry != NULL && FilterEntry->Intf == NULL)
    {
        FilterEntry = NULL;
    }

    return FilterEntry;
}

static void SBN_InitPeersFromConfig(const SBN_ConfTbl_t *ConfTbl)
{
    const SBN_Peer_Entry_t       *PeerConf;
    const SBN_ProtocolModInfo_t  *ProtoEntry;
    const SBN_FilterModInfo_t    *FilterEntry;
    const SBN_FilterInterface_t **FilterList;
    char                          FilterNameTemp[CFE_MISSION_MAX_API_LEN];
    SBN_PeerRecord_t             *PeerRec;
    SBN_NetRecord_t              *NetRec;
    SBN_PeerInterface_t          *Peer;
    SBN_NetInterface_t           *Net;
    SBN_NetIdx_t                  MaxNet;
    SBN_PeerIdx_t                 PeerIdx;
    uint8                         FilterIdx;

    union
    {
        char Name[CFE_MISSION_MAX_API_LEN];
        char Address[SBN_MISSION_ADDR_SZ];
    } LocalBuf;

    /* load nets and peers */
    for (PeerIdx = 0; PeerIdx < SBN_MISSION_MAX_PEER_CNT && PeerIdx < ConfTbl->PeerCnt; PeerIdx++)
    {
        PeerConf   = &ConfTbl->Peers[PeerIdx];
        FilterList = NULL;
        PeerRec    = NULL;
        NetRec     = NULL;
        Peer       = NULL;
        Net        = NULL;

        EVSSendInfo(SBN_TBL_EID, "configuring peer (SC=%d, CPU=%d)...", PeerConf->SpacecraftID, PeerConf->ProcessorID);

        CFE_SB_MessageStringGet(LocalBuf.Name,
                                PeerConf->ProtocolName,
                                "",
                                sizeof(LocalBuf),
                                sizeof(PeerConf->ProtocolName));
        ProtoEntry = SBN_FindActiveProtocol(LocalBuf.Name);

        if (ProtoEntry == NULL)
        {
            EVSSendCrit(SBN_TBL_EID,
                        "protocol %s is not available (skipping peer %u)",
                        LocalBuf.Name,
                        (unsigned int)PeerIdx);
        }
        else
        {
            /* note: the "NetNum" in the table was validated already by the validation routine */
            if (!CFE_RESOURCEID_TEST_DEFINED(SBN_AppData.NetIds[PeerConf->NetNum]))
            {
                SBN_RegisterNet(&SBN_AppData.NetIds[PeerConf->NetNum]);
            }
            NetRec = SBN_LocateNetRecordByID(SBN_AppData.NetIds[PeerConf->NetNum]);
            if (!SBN_NetRecordIsMatch(NetRec, SBN_AppData.NetIds[PeerConf->NetNum]))
            {
                EVSSendCrit(SBN_TBL_EID, "no net record found (skipping peer %u)", (unsigned int)PeerIdx);
                continue;
            }

            Net = SBN_Net_GetIntfData(NetRec);

            CFE_SB_MessageStringGet(LocalBuf.Address,
                                    (const char *)PeerConf->Address,
                                    "",
                                    sizeof(LocalBuf),
                                    sizeof(PeerConf->Address));

            /* Reset peer count since we're initializing the net */
            if (PeerConf->ProcessorID == CFE_PSP_GetProcessorId()
                && PeerConf->SpacecraftID == CFE_PSP_GetSpacecraftId())
            {
                EVSSendInfo(SBN_TBL_EID, "peer is this processor: loading net %u", (unsigned int)PeerConf->NetNum);

                Net->ProtocolIdx = (ProtoEntry - SBN_AppData.ProtocolModInfo);
                Net->IfOps       = ProtoEntry->Intf;
                Net->TaskFlags   = PeerConf->TaskFlags;
                Net->Configured  = true;

                ProtoEntry->Intf->LoadNet(Net, LocalBuf.Address);

                FilterList = Net->Filters;
            }
            else
            {
                EVSSendInfo(SBN_TBL_EID,
                            "peer is other processor: loading peer onto net %u",
                            (unsigned int)PeerConf->NetNum);

                /* Note that it was already validated that this cannot exceed the max peer count */
                SBN_RegisterPeer(&Net->PeerIds[Net->PeerCnt]);
                PeerRec = SBN_LocatePeerRecordByID(Net->PeerIds[Net->PeerCnt]);
                if (!SBN_PeerRecordIsMatch(PeerRec, Net->PeerIds[Net->PeerCnt]))
                {
                    EVSSendCrit(SBN_TBL_EID, "no peer record found (skipping peer %u)", (unsigned int)PeerIdx);
                    continue;
                }

                PeerRec->NetId = NetRec->NetId;

                Peer = SBN_Peer_GetIntfData(PeerRec);

                Peer->Net          = Net;
                Peer->ProcessorID  = PeerConf->ProcessorID;
                Peer->SpacecraftID = PeerConf->SpacecraftID;
                Peer->TaskFlags    = PeerConf->TaskFlags;
                ++Net->PeerCnt;

                ProtoEntry->Intf->LoadPeer(Peer, LocalBuf.Address);

                FilterList = Peer->Filters;
            }
        }

        /* Set up the filters for this peer */
        if (FilterList != NULL)
        {
            for (FilterIdx = 0; FilterIdx < SBN_MISSION_MAX_FILTERS_PER_PEER; ++FilterIdx)
            {
                CFE_SB_MessageStringGet(FilterNameTemp,
                                        PeerConf->Filters[FilterIdx],
                                        "",
                                        sizeof(FilterNameTemp),
                                        sizeof(PeerConf->Filters[FilterIdx]));
                if (FilterNameTemp[0] != 0)
                {
                    FilterEntry = SBN_FindActiveFilter(FilterNameTemp);
                    if (FilterEntry == NULL)
                    {
                        EVSSendInfo(SBN_TBL_EID,
                                    "cannot map filter %s at index %u peer %u",
                                    FilterNameTemp,
                                    (unsigned int)FilterIdx,
                                    (unsigned int)PeerIdx);
                    }
                    else
                    {
                        *FilterList = FilterEntry->Intf;
                        ++FilterList;
                    }
                }
            }
        }
    }

    /* Now find the MAX configured net number.  This is mainly for efficiency, to limit the looping at runtime. */
    MaxNet = SBN_PLATFORM_MAX_NETS;
    while (MaxNet != 0)
    {
        if (CFE_RESOURCEID_TEST_DEFINED(SBN_AppData.NetIds[MaxNet - 1]))
        {
            break;
        }
        --MaxNet;
    }

    SBN_AppData.NetCnt = MaxNet;
}

static SBN_Status_t SBN_LoadConf(void)
{
    void        *TableAddr;
    CFE_Status_t Status;

    /* wipe the runtime info structs as we are re-loading them */
    memset(SBN_AppData.ProtocolModInfo, 0, sizeof(SBN_AppData.ProtocolModInfo));
    memset(SBN_AppData.FilterModInfo, 0, sizeof(SBN_AppData.FilterModInfo));
    memset(SBN_AppData.NetIds, 0, sizeof(SBN_AppData.NetIds));
    memset(SBN_AppData.PeerRecords, 0, sizeof(SBN_AppData.PeerRecords));
    memset(SBN_AppData.NetRecords, 0, sizeof(SBN_AppData.NetRecords));

    SBN_AppData.NetCnt = 0;

    TableAddr = NULL;
    Status    = CFE_TBL_GetAddress(&TableAddr, SBN_AppData.ConfTblHandle);

    if (Status < CFE_SUCCESS || TableAddr == NULL)
    {
        EVSSendErr(SBN_TBL_EID, "unable to get conf table address");
        return SBN_ERROR;
    } /* end if */

    SBN_InitProtoModules(TableAddr);
    SBN_InitFilterModules(TableAddr);
    SBN_InitPeersFromConfig(TableAddr);

    /* address only needed at load time, release */
    if (CFE_TBL_ReleaseAddress(SBN_AppData.ConfTblHandle) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to release address of conf tbl");
        return SBN_ERROR;
    } /* end if */

    /* ...but we keep the handle so we can be notified of updates */
    return SBN_SUCCESS;
} /* end SBN_LoadConf() */

static SBN_Status_t SBN_UnloadConf(void)
{
    SBN_Status_t Status;

    EVSSendInfo(SBN_TBL_EID, "unloading configuration");

    Status = SBN_UnloadNets();
    if (Status != SBN_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to unload nets");
    }

    return Status;
} /* end SBN_UnloadConf() */

static SBN_Status_t SBN_UnloadPeer(SBN_PeerRecord_t *PeerRec)
{
    SBN_PeerInterface_t *Peer = SBN_Peer_GetIntfData(PeerRec);
    SBN_NetInterface_t  *Net  = Peer->Net;

    SBN_RemoveAllSubsFromPeer(PeerRec);
    SBN_Disconnected(Peer);

    if (Peer->TaskFlags & SBN_TASK_SEND)
    {
        if (CFE_RESOURCEID_TEST_DEFINED(Peer->SendTaskID))
        {
            if (CFE_ES_DeleteChildTask(Peer->SendTaskID) != CFE_SUCCESS)
            {
                EVSSendCrit(SBN_TBL_EID,
                            "unable to delete send task for peer %d:%d",
                            Peer->SpacecraftID,
                            Peer->ProcessorID);
            }
        }
    }

    if (Peer->TaskFlags & SBN_TASK_RECV)
    {
        if (CFE_RESOURCEID_TEST_DEFINED(Peer->RecvTaskID))
        {
            if (CFE_ES_DeleteChildTask(Peer->RecvTaskID) != CFE_SUCCESS)
            {
                EVSSendCrit(SBN_TBL_EID,
                            "unable to delete recv task for peer %d:%d",
                            Peer->SpacecraftID,
                            Peer->ProcessorID);
            }
        }
    }

    if (Net->IfOps && Net->IfOps->UnloadPeer)
    {
        Net->IfOps->UnloadPeer(Peer);
    }

    // Reset peer description
    Peer->ProcessorID  = 0;
    Peer->SpacecraftID = 0;
    Peer->Net          = NULL;
    Peer->TaskFlags    = 0;
    Peer->Pipe         = CFE_SB_INVALID_PIPE;
    Peer->FilterCnt    = 0;

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_DoUnloadNet(SBN_NetRecord_t *NetRec, void *Arg)
{
    uint32             *Errs = Arg;
    SBN_NetInterface_t *Net;
    SBN_Status_t        Status;
    SBN_PeerIdx_t       PeerIdx;
    SBN_PeerRecord_t   *PeerRec;

    Net = SBN_Net_GetIntfData(NetRec);

    Net->Configured = false;

    if (CFE_RESOURCEID_TEST_DEFINED(Net->RecvTaskID))
    {
        if (CFE_ES_DeleteChildTask(Net->RecvTaskID) != CFE_SUCCESS)
        {
            EVSSendCrit(SBN_TBL_EID, "unable to delete receive task %d", SBN_NetId_AsInt(NetRec->NetId));
            ++(*Errs);
        }

        Net->RecvTaskID = CFE_ES_TASKID_UNDEFINED;
    }

    for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        PeerRec = SBN_LocatePeerRecordByID(Net->PeerIds[PeerIdx]);
        if (SBN_PeerRecordIsMatch(PeerRec, Net->PeerIds[PeerIdx]))
        {
            SBN_UnloadPeer(PeerRec);

            SBN_PeerRecordSetFree(PeerRec);
        }

        Net->PeerIds[PeerIdx] = SBN_PEERID_UNDEFINED;
    }

    // Peers were cleared, reset the count
    Net->PeerCnt = 0;

    if (Net->IfOps && Net->IfOps->UnloadNet)
    {
        Status = Net->IfOps->UnloadNet(Net);
    }
    else
    {
        Status = SBN_SUCCESS;
    }
    if (Status != SBN_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to unload network %d", SBN_NetId_AsInt(NetRec->NetId));
        ++(*Errs);
    }
    else
    {
        EVSSendInfo(SBN_TBL_EID, "Terminated net: %d", SBN_NetId_AsInt(NetRec->NetId));
    }

    SBN_NetRecordSetFree(NetRec);

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_UnloadNets(void)
{
    uint32 Errs;

    Errs = 0;

    SBN_ForEachConfiguredNet(SBN_DoUnloadNet, &Errs);

    if (Errs != 0)
    {
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_LoadConfTbl(void)
{
    CFE_Status_t Status;

    Status = CFE_TBL_Register(&SBN_AppData.ConfTblHandle,
                              "ConfTbl",
                              sizeof(SBN_ConfTbl_t),
                              CFE_TBL_OPT_DEFAULT,
                              SBN_ValidateConfTbl);
    if (Status < CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to register conf tbl handle");
        return SBN_ERROR;
    } /* end if */

    Status = CFE_TBL_NotifyByMessage(SBN_AppData.ConfTblHandle, CFE_SB_ValueToMsgId(SBN_CMD_MID), SBN_TBL_CC, 0);
    if (Status < CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
        return SBN_ERROR;
    } /* end if */

    Status = CFE_TBL_Load(SBN_AppData.ConfTblHandle, CFE_TBL_SRC_FILE, SBN_PLATFORM_CONF_TBL_FILENAME);
    if (Status < CFE_SUCCESS)
    {
        /* This is not a fatal error, the table can be loaded later */
        EVSSendErr(SBN_TBL_EID, "unable to load conf tbl %s", SBN_PLATFORM_CONF_TBL_FILENAME);
    }

    return SBN_SUCCESS;
} /* end SBN_LoadConfTbl() */

static SBN_Status_t SBN_TeardownSubPipe(void)
{
    CFE_Status_t Status;

    if (CFE_RESOURCEID_TEST_DEFINED(SBN_AppData.SubPipe))
    {
        /* Delete pipe for subscribes and unsubscribes from SB */
        Status = CFE_SB_DeletePipe(SBN_AppData.SubPipe);
        if (Status != CFE_SUCCESS)
        {
            EVSSendErr(SBN_INIT_EID, "failed to delete subscription pipe (Status=%d)", (int)Status);
            return SBN_ERROR;
        }

        SBN_AppData.SubPipe = CFE_SB_INVALID_PIPE;
    }

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_SetupSubPipe(void)
{
    CFE_Status_t Status;

    /* Create pipe for subscribes and unsubscribes from SB */
    Status = CFE_SB_CreatePipe(&SBN_AppData.SubPipe, SBN_PLATFORM_SUB_PIPE_DEPTH, "SBNSubPipe");
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to create subscription pipe (Status=%d)", (int)Status);
        SBN_AppData.SubPipe = CFE_SB_INVALID_PIPE;
        return SBN_ERROR;
    } /* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID),
                                   SBN_AppData.SubPipe,
                                   SBN_PLATFORM_MAX_ALLSUBS_PKTS_ON_PIPE);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to allsubs (Status=%d)", (int)Status);
        return SBN_ERROR;
    } /* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID),
                                   SBN_AppData.SubPipe,
                                   SBN_PLATFORM_MAX_ONESUB_PKTS_ON_PIPE);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to sub (Status=%d)", (int)Status);
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

/**
 * @brief Initializes the housekeeping counters for a peer.
 *
 * @param[in] Peer The peer interface for which to reset housekeeping.
 */
static SBN_Status_t SBN_DoInitializePeerCounters(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    SBN_PeerInterface_t *Peer = SBN_Peer_GetIntfData(PeerRec);

    Peer->LastSend = OS_TIME_ZERO;
    Peer->LastRecv = OS_TIME_ZERO;

    memset(&Peer->Counts, 0, sizeof(Peer->Counts));

    return SBN_SUCCESS;
}

/**
 * @brief Initializes the housekeeping counters both at the app level
 *        and for each peer.
 */
void SBN_InitializeCounters(void)
{
    SBN_AppData.CmdCnt    = 0;
    SBN_AppData.CmdErrCnt = 0;

    /* Wipe the counters on every peer */
    SBN_ForEachConfiguredPeer(SBN_DoInitializePeerCounters, NULL);
}

/**
 * @brief Initializes the housekeeping counters for a peer.
 *
 * @param[in] Peer The peer interface for which to reset housekeeping.
 */
void SBN_InitializePeerCounters(SBN_PeerRecord_t *PeerRec)
{
    /* Wipe the counters on only this peer */
    SBN_DoInitializePeerCounters(PeerRec, NULL);
}

static SBN_Status_t SBN_Init(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not initialize SBN:";

    SBN_Status_t Status;

    /*
     * NOTE: Do not over-react if these fail. We do not need to abort
     * and kill the SBN app because of a config error.  Rather, the bad config
     * will be reported, and SBN can fall back to a passive or reduced operation
     * mode.  The configuration can then be fixed and reloaded.
     */

    /* Load the configuration from the table */
    Status = SBN_LoadConf();
    if (Status == SBN_SUCCESS)
    {
        Status = SBN_InitInterfaces();
        if (Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_INIT_EID, "%s unable to initialize interfaces", FAIL_PREFIX);
        }
    }

    const char *bit_order =
#ifdef SOFTWARE_BIG_BIT_ORDER
        "big-endian";
#else  /* !SOFTWARE_BIG_BIT_ORDER */
        "little-endian";
#endif /* SOFTWARE_BIG_BIT_ORDER */

    EVSSendInfo(SBN_INIT_EID,
                "initialized (ProcessorID=%d SpacecraftId=%d %s "
                "SBN_AppData.AppID=%d...",
                (int)CFE_PSP_GetProcessorId(),
                (int)CFE_PSP_GetSpacecraftId(),
                bit_order,
                (int)CFE_RESOURCEID_TO_ULONG(SBN_AppData.AppID));

    EVSSendInfo(SBN_INIT_EID, "...SBN_IDENT=%s CMD_MID=0x%04X)", SBN_IDENT, SBN_CMD_MID);

    EVSSendInfo(SBN_INIT_EID, "Re-sending SB subscription requests...");
    SBN_SendSubsRequests();

    EVSSendInfo(SBN_INIT_EID, "SBN initialized.");

    return Status;
}

static SBN_Status_t SBN_Cleanup(void)
{
    SBN_Status_t Status;

    Status = SBN_TeardownSubPipe();
    if (Status != SBN_SUCCESS)
    {
        /* A failure here does not mean we should everything else uncleaned */
        EVSSendErr(SBN_PEER_EID, "unable to tear down sub pipe");
    }

    Status = SBN_UnloadConf();
    if (Status != SBN_SUCCESS)
    {
        /* A failure here does not mean we should everything else uncleaned */
        EVSSendErr(SBN_PEER_EID, "unable to unload configuration");
    }

    // cleanup subs
    memset(SBN_AppData.Subs, 0, sizeof(SBN_AppData.Subs));

    SBN_AppData.SubCnt = 0;

    if (CFE_TBL_HANDLE_IS_VALID(SBN_AppData.ConfTblHandle))
    {
        /* "best effort" cleanup -- if this fails, nothing else to do */
        CFE_TBL_Unregister(SBN_AppData.ConfTblHandle);
        SBN_AppData.ConfTblHandle = CFE_TBL_BAD_TABLE_HANDLE;
    }

    return SBN_SUCCESS;
}

/** \brief SBN Main Routine */
void SBN_AppMain(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not start SBN:";
    CFE_ES_TaskInfo_t TaskInfo      = { 0 };
    int32             Status;
    uint32            RunStatus;
    CFE_ES_AppId_t    AppID;
    CFE_ES_TaskId_t   TskId;

    memset(&SBN_AppData, 0, sizeof(SBN_AppData));

    RunStatus = CFE_ES_RunStatus_APP_RUN;
    AppID     = CFE_ES_APPID_UNDEFINED;

    if (CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER) != CFE_SUCCESS)
    {
        return;
    }

    if (CFE_ES_GetAppID(&AppID) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_INIT_EID, "%s unable to get AppID", FAIL_PREFIX);
        return;
    }

    SBN_AppData.AppID = AppID;

    /* load my TaskName so I can ignore messages I send out to SB */
    CFE_ES_GetTaskID(&TskId);
    Status = CFE_ES_GetTaskInfo(&TaskInfo, TskId);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s SBN failed to get task info (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    strncpy(SBN_AppData.App_FullName, (const char *)TaskInfo.TaskName, OS_MAX_API_NAME - 1);
    SBN_AppData.App_FullName[OS_MAX_API_NAME - 1] = '\0';

    /** Create mutex for send tasks */
    Status = OS_MutSemCreate(&(SBN_AppData.SendMutex), "sbn_send_mutex", 0);
    if (Status != OS_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s error creating mutex for send tasks", FAIL_PREFIX);
        return;
    }

    /* Create pipe for HK requests and gnd commands */
    /* TODO: make configurable depth */
    Status = CFE_SB_CreatePipe(&SBN_AppData.CmdPipe, 20, "SBNCmdPipe");
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s failed to create command pipe (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SBN_CMD_MID), SBN_AppData.CmdPipe);
    if (Status == CFE_SUCCESS)
    {
        EVSSendInfo(SBN_INIT_EID, "SBN subscribed to command pipe SBN_CMD_MID 0x%04X", SBN_CMD_MID);
    }
    else
    {
        EVSSendErr(SBN_INIT_EID, "%s failed to subscribe to command pipe (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    /* Load the table */
    if (SBN_LoadConfTbl() != SBN_SUCCESS)
    {
        /* the SBN_LoadConfTbl() functions will generate events */
        EVSSendErr(SBN_INIT_EID, "%s failed to load configuration table", FAIL_PREFIX);
        return;
    }

    if (SBN_SetupSubPipe() != SBN_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s unable to set up subscription pipe", FAIL_PREFIX);
        return;
    }

    CFE_ES_WaitForStartupSync(10000);

    /* This sets up the nets and peers based on what is in the table, as a "best effort" */
    /* Notably - if this fails, then we will just proceed with watever nets/peers were valid,
     * or simply no nets or peers at all.  Thats OK.  SBN will not do anything useful without
     * any nets/peers, but it will sit idly until the table is reloaded with something valid.  */
    SBN_Init();

    /* Loop Forever */
    while (CFE_ES_RunLoop(&RunStatus))
    {
        SBN_WaitForWakeup(SBN_PLATFORM_MAIN_LOOP_DELAY);
    } /* end while */

    if (SBN_Cleanup() != SBN_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "ERROR: could not clean up SBN");
    }

    CFE_ES_ExitApp(RunStatus);
} /* end SBN_AppMain */

/**
 * Process a message from the network interface
 * @param[in] Net The net interface of the peer
 * @param[in] MsgType The type of the message (application data, SBN protocol)
 * @param[in] ProcessorID The ProcessorID to send this message to.
 * @param[in] SpacecraftID The SpacecraftID to send this message to.
 * @param[in] MsgSz The size of the message (in bytes).
 * @param[in] Msg The message contents.
 *
 * @return SBN_SUCCESS on successful processing, SBN_ERROR otherwise
 */
SBN_Status_t SBN_ProcessPeerMsg(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *D)
{
    static const char    FAIL_PREFIX[] = "ERROR: could not process peer message:";
    SBN_Status_t         SBN_Status;
    CFE_Status_t         CFE_Status;
    SBN_PeerInterface_t *Peer;
    uint8                Ver;
    SBN_ModuleIdx_t      FilterIdx;
    SBN_Filter_Ctx_t     Filter_Context;
    void                *Msg;

    if (!PeerRec)
    {
        EVSSendErr(SBN_PEERTASK_EID, "%s unknown peer (ProcessorID=%d)", FAIL_PREFIX, (int)D->ProcessorID);
        return SBN_ERROR;
    } /* end if */

    /* Check if message is protocol-specific (unkown to SBN core) */
    if (D->MsgType & SBN_MODULE_SPECIFIC_MESSAGE_ID_MASK)
    {
        EVSSendDbg(SBN_PEERTASK_EID, "SBN received module-specific message type: 0x%08x", (unsigned int)D->MsgType);
        return SBN_SUCCESS;
    }

    /* Shortcut to interface data */
    Peer = SBN_Peer_GetIntfData(PeerRec);
    Msg  = D->DataPtr;

    switch (D->MsgType)
    {
        case SBN_PROTO_MSG:
        {
            Ver = ((uint8 *)Msg)[0];
            if (Ver != SBN_PROTO_VER)
            {
                EVSSendErr(SBN_SB_EID,
                           "%s SBN protocol version mismatch with peer %d:%d, "
                           "my version=%d, peer version %d",
                           FAIL_PREFIX,
                           Peer->SpacecraftID,
                           (int)Peer->ProcessorID,
                           (int)SBN_PROTO_VER,
                           (int)Ver);
            }
            else
            {
                EVSSendInfo(SBN_SB_EID,
                            "SBN protocol version match with peer %d:%d",
                            (int)Peer->SpacecraftID,
                            (int)Peer->ProcessorID);
            } /* end if */
            break;
        } /* end case */
        case SBN_APP_MSG:
        {
            Filter_Context.MyProcessorID    = CFE_PSP_GetProcessorId();
            Filter_Context.MySpacecraftID   = CFE_PSP_GetSpacecraftId();
            Filter_Context.PeerProcessorID  = Peer->ProcessorID;
            Filter_Context.PeerSpacecraftID = Peer->SpacecraftID;

            for (FilterIdx = 0; FilterIdx < Peer->FilterCnt; FilterIdx++)
            {
                if (Peer->Filters[FilterIdx]->FilterRecv == NULL)
                {
                    continue;
                } /* end if */

                SBN_Status = (Peer->Filters[FilterIdx]->FilterRecv)(Msg, &Filter_Context);

                /* includes SBN_IF_EMPTY, for when filter recommends removing */
                if (SBN_Status != SBN_SUCCESS)
                {
                    return SBN_Status;
                } /* end if */
            } /* end for */

            CFE_Status = CFE_SB_TransmitMsg(Msg, false);

            if (CFE_Status != CFE_SUCCESS)
            {
                EVSSendErr(SBN_SB_EID,
                           "%s CFE_SB_PassMsg error (Status=%d MsgType=0x%x)",
                           FAIL_PREFIX,
                           (int)CFE_Status,
                           D->MsgType);
                return SBN_ERROR;
            } /* end if */
            break;
        } /* end case */
        case SBN_SUB_MSG:
            return SBN_ProcessSubsFromPeer(PeerRec, D);

        case SBN_UNSUB_MSG:
            return SBN_ProcessUnsubsFromPeer(PeerRec, D);

        case SBN_NO_MSG:
            return SBN_SUCCESS;
        default:
            /* Should I generate an event? Probably... */
            EVSSendErr(SBN_PEERTASK_EID, "%s unknown message type 0x%08x", FAIL_PREFIX, D->MsgType);
            return SBN_ERROR;
    } /* end switch */

    return SBN_SUCCESS;
} /* end SBN_ProcessPeerMsg */

SBN_PeerRecord_t *
SBN_GetPeerRecord(SBN_NetRecord_t *NetRec, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID)
{
    SBN_PeerIdx_t        PeerIdx;
    SBN_PeerRecord_t    *PeerRec;
    SBN_NetInterface_t  *Net;
    SBN_PeerInterface_t *Peer;

    Net = SBN_Net_GetIntfData(NetRec);

    for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        PeerRec = SBN_LocatePeerRecordInNet(NetRec, PeerIdx);
        Peer    = SBN_Peer_GetIntfData(PeerRec);

        if (Peer != NULL && Peer->ProcessorID == ProcessorID && Peer->SpacecraftID == SpacecraftID)
        {
            return PeerRec;
        }
    }

    return NULL;
}

/**
 * Find the PeerIndex for a given ProcessorID and net.
 * @param[in] Net The network interface to search.
 * @param[in] ProcessorID The ProcessorID of the peer being sought.
 * @param[in] SpacecraftID The SpacecraftI of the peer being sought.
 * @return The Peer interface pointer, or NULL if not found.
 */
SBN_PeerInterface_t *
SBN_GetPeer(SBN_NetInterface_t *Net, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID)
{
    /* Compatibility wrapper that works on intf level */
    return SBN_Peer_GetIntfData(SBN_GetPeerRecord(SBN_Net_GetPrivateData(Net), ProcessorID, SpacecraftID));
} /* end SBN_GetPeer */

/** \brief Reload configuration and re-initialize
 * This is only called after receiving a message from TBL service.
 */
SBN_Status_t SBN_ReloadConfTbl(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not reload SBN configuration:";

    SBN_Status_t Status;

    EVSSendInfo(SBN_TBL_EID, "re-initializing SBN with new configuration...");

    Status = SBN_UnloadConf();
    if (Status != SBN_SUCCESS)
    {
        /* A failure here does not mean we should everything else uncleaned */
        EVSSendErr(SBN_PEER_EID, "unable to unload configuration, status=%d", (int)Status);
    }

    /* This is intentional -- try to re-init regardless of how the cleanup went */
    Status = SBN_Init();
    if (Status != SBN_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "%s status=%d", FAIL_PREFIX, (int)Status);
    }
    else
    {
        EVSSendInfo(SBN_TBL_EID, "SBN re-initialized successfully.");
    }

    return Status;
} /* end SBN_ReloadConfTbl() */
