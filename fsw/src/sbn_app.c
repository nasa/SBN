/******************************************************************************
 ** \file sbn_app.c
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
 **      This file contains source code for the Software Bus Network
 **      Application.
 **
 ** Authors:   J. Wilmot/GSFC Code582
 **            R. McGraw/SSI
 **            C. Knight/ARC Code TI
 ******************************************************************************/

/*
 ** Include Files
 */
#include <fcntl.h>

#include "sbn_pack.h"
#include "sbn_app.h"
#include "cfe_sb_eventids.h" /* For event message IDs */
#include "cfe_es.h"        /* PerfLog */
#include "cfe_platform_cfg.h"
#include "cfe_msgids.h"
#include "cfe_version.h"

/** \brief SBN global application data, indexed by AppID. */
SBN_App_t SBN;

#include <string.h>
#include "sbn_app.h"

static SBN_Status_t UnloadNets(void);

static SBN_Status_t UnloadModules(void)
{
    SBN_ModuleIdx_t i = 0;

    for (i = 0; i < SBN_MAX_MOD_CNT; i++)
    {
        if (SBN.ProtocolModules[i] == 0)
        {
            continue; /* this module may have been loaded by ES, so continue in case there are any I loaded. */
        }             /* end if */

        if (OS_ModuleUnload(SBN.ProtocolModules[i]) != OS_SUCCESS)
        {
            EVSSendCrit(SBN_TBL_EID, "unable to unload protocol module ID %d", i);
            return SBN_ERROR;
        } /* end if */
    }     /* end for */

    for (i = 0; i < SBN_MAX_MOD_CNT; i++)
    {
        if (SBN.FilterModules[i] == 0)
        {
            continue; /* this module may have been loaded by ES, so continue in case there are any I loaded. */
        }             /* end if */

        if (OS_ModuleUnload(SBN.FilterModules[i]) != OS_SUCCESS)
        {
            EVSSendCrit(SBN_TBL_EID, "unable to unload filter module ID %d", i);
            return SBN_ERROR;
        } /* end if */
    }     /* end for */

    return SBN_SUCCESS;
} /* end UnloadModules() */

/**
 * Packs a CCSDS message with an SBN message header.
 *
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in network (big-endian) byte order.
 * \param SBNBuf[out] The buffer to pack into.
 * \param MsgSz[in] The size of the payload.
 * \param MsgType[in] The SBN message type.
 * \param ProcessorID[in] The ProcessorID of the sender (should be CFE_CPU_ID)
 * \param SpacecraftID[in] The SpacecraftID of the sender
 * \param Msg[in] The payload (CCSDS message or SBN sub/unsub.)
 */
void SBN_PackMsg(void *SBNBuf, SBN_MsgSz_t MsgSz, SBN_MsgType_t MsgType, CFE_ProcessorID_t ProcessorID,  CFE_ProcessorID_t SpacecraftID,void *Msg)
{
    Pack_t Pack;
    Pack_Init(&Pack, SBNBuf, MsgSz + SBN_PACKED_HDR_SZ, false);

    Pack_Int16(&Pack, MsgSz);
    Pack_UInt8(&Pack, MsgType);
    Pack_UInt32(&Pack, ProcessorID);
    Pack_UInt32(&Pack, SpacecraftID);

    if (!Msg || !MsgSz)
    {
        /* valid to have a NULL pointer/empty size payload */
        return;
    } /* end if */

    Pack_Data(&Pack, Msg, MsgSz);
} /* end SBN_PackMsg */

/**
 * Unpacks a CCSDS message with an SBN message header.
 *
 * \param SBNBuf[in] The buffer to unpack.
 * \param MsgTypePtr[out] The SBN message type.
 * \param MsgSzPtr[out] The payload size.
 * \param ProcessorID[out] The ProcessorID of the sender.
 * \param Msg[out] The payload (a CCSDS message, or SBN sub/unsub).
 * \return true if we were able to unpack the message.
 *
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in platform byte order.
 * \todo Use a type for SBNBuf.
 */
bool SBN_UnpackMsg(void *SBNBuf, SBN_MsgSz_t *MsgSzPtr, SBN_MsgType_t *MsgTypePtr,
                   CFE_ProcessorID_t *ProcessorIDPtr, CFE_SpacecraftID_t *SpacecraftIDPtr,
                   void *Msg)
{
    uint8  t = 0;
    Pack_t Pack;
    Pack_Init(&Pack, SBNBuf, SBN_MAX_PACKED_MSG_SZ, false);
    Unpack_Int16(&Pack, MsgSzPtr);
    Unpack_UInt8(&Pack, &t);
    *MsgTypePtr = t;
    Unpack_UInt32(&Pack, ProcessorIDPtr);
    Unpack_UInt32(&Pack, SpacecraftIDPtr);

    if (!*MsgSzPtr)
    {
        return true;
    } /* end if */

    if (*MsgSzPtr < 0 || *MsgSzPtr > CFE_MISSION_SB_MAX_SB_MSG_SIZE)
    {
        return false;
    } /* end if */

    Unpack_Data(&Pack, Msg, *MsgSzPtr);

    return true;
} /* end SBN_UnpackMsg */

/**
 * Called by a protocol module to signal that a peer has been connected.
 *
 * @param Peer[in] The peer that has been connected.
 * @return SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Connected(SBN_PeerInterface_t *Peer)
{
    static const char FAIL_PREFIX[] = "ERROR: could not disconnect peer:";
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    CFE_Status_t CFE_Status;

    if (Peer->Connected != 0)
    {
        EVSSendErr(SBN_PEER_EID, "%s peer %d:%d already connected", FAIL_PREFIX, Peer->SpacecraftID, (int)(Peer->ProcessorID));
        return SBN_ERROR;
    } /* end if */

    char PipeName[OS_MAX_API_NAME];

    /* create a pipe name string similar to SBN_0_Pipe */
    snprintf(PipeName, OS_MAX_API_NAME, "SBN_%d_%d_Pipe", (int)(Peer->ProcessorID), (int)(Peer->SpacecraftID));
    CFE_Status = CFE_SB_CreatePipe(&(Peer->Pipe), SBN_PEER_PIPE_DEPTH, PipeName);

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

    uint8 ProtocolVer = SBN_PROTO_VER;
    SBN_Status        = SBN_SendNetMsg(SBN_PROTO_MSG, sizeof(ProtocolVer), &ProtocolVer, Peer);
    if (SBN_Status != SBN_SUCCESS)
    {
        return SBN_Status;
    } /* end if */

    /* set this to current time so we don't think we've already timed out */
    OS_GetLocalTime(&Peer->LastRecv);

    SBN_Status = SBN_SendLocalSubsToPeer(Peer);

    Peer->Connected = 1;

    return SBN_Status;
} /* end SBN_Connected() */

/**
 * Called by a protocol module to signal that a peer has been disconnected.
 *
 * @param Peer[in] The peer that has been disconnected.
 * @return SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Disconnected(SBN_PeerInterface_t *Peer)
{
    static const char FAIL_PREFIX[] = "ERROR: could not disconnect peer:";
    CFE_Status_t Status;

    if (Peer->Connected == 0)
    {
        EVSSendErr(SBN_PEER_EID, "%s already not connected to peer %d:%d", FAIL_PREFIX, Peer->SpacecraftID, (int)(Peer->ProcessorID));
        return SBN_ERROR;
    }

    Peer->Connected = 0; /**< mark as disconnected before deleting pipe */

    if((Status = CFE_SB_DeletePipe(Peer->Pipe)) != CFE_SUCCESS) {
      EVSSendErr(SBN_PEER_EID, "%s could not delete pipe when disconnecting peer %d:%d: 0x%08x", 
          FAIL_PREFIX,
          Peer->SpacecraftID,
          Peer->ProcessorID,
          Status);
    }
    Peer->Pipe = 0;

    Peer->SendCnt = 0;
    Peer->RecvCnt = 0;
    Peer->SendErrCnt = 0;
    Peer->RecvErrCnt = 0;

    Peer->SubCnt = 0; /* reset sub count, in case this is a reconnection */

    EVSSendInfo(SBN_PEER_EID, "Disconnected from peer %d:%d.", Peer->SpacecraftID, (int)(Peer->ProcessorID));

    return SBN_SUCCESS;
} /* end SBN_Disconnected() */

/* Use a struct for all local variables in the task so we can specify exactly
 * how large of a stack we need for the task.
 */

typedef struct
{
    SBN_Status_t         Status;
    OS_TaskID_t          RecvTaskID;
    SBN_PeerIdx_t        PeerIdx;
    SBN_NetIdx_t         NetIdx;
    SBN_PeerInterface_t *Peer;
    SBN_NetInterface_t * Net;
    CFE_ProcessorID_t    ProcessorID;
    CFE_SpacecraftID_t   SpacecraftID;
    SBN_MsgType_t        MsgType;
    SBN_MsgSz_t          MsgSz;
    uint8                Msg[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
} RecvPeerTaskData_t;

/**
 * \brief Receive task created for each direct peer-based connection.
 * Spanwed from PeerPoll()
 */
void SBN_RecvPeerTask(void)
{
    RecvPeerTaskData_t D;
    memset(&D, 0, sizeof(D));

    D.RecvTaskID = OS_TaskGetId();

    for (D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        if (!D.Net->Configured)
        {
            continue;
        }

        for (D.PeerIdx = 0; D.PeerIdx < D.Net->PeerCnt; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if (D.Peer->RecvTaskID == D.RecvTaskID)
            {
                break;
            } /* end if */
        }     /* end for */

        if (D.PeerIdx < D.Net->PeerCnt)
        {
            break;
        } /* end if */
    }     /* end for */

    if (D.NetIdx == SBN.NetCnt)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unable to connect task to peer struct");
        return;
    } /* end if */

    while (1)
    {
        D.Status = D.Net->IfOps->RecvFromPeer(D.Net, D.Peer, &D.MsgType, &D.MsgSz, &D.ProcessorID, &D.SpacecraftID, &D.Msg);

        if (D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }             /* end if */

        if (D.Status == SBN_SUCCESS)
        {
            OS_GetLocalTime(&D.Peer->LastRecv);

            D.Status = SBN_ProcessNetMsg(D.Net, D.MsgType, D.ProcessorID, D.SpacecraftID, D.MsgSz, &D.Msg);

            if (D.Status != SBN_SUCCESS)
            {
                D.Peer->RecvTaskID = 0;
                return;
            } /* end if */
        }
        else
        {
            EVSSendErr(SBN_PEER_EID, "recv error (%d)", D.Status);
            D.Peer->RecvErrCnt++;
            D.Peer->RecvTaskID = 0;
            return;
        } /* end if */
    }     /* end while */
} /* end SBN_RecvPeerTask() */

typedef struct RecvNetTaskData_s
{
    SBN_NetIdx_t         NetIdx;
    SBN_NetInterface_t * Net;
    SBN_PeerInterface_t *Peer;
    SBN_Status_t         Status;
    OS_TaskID_t          RecvTaskID;
    CFE_ProcessorID_t    ProcessorID;
    CFE_SpacecraftID_t   SpacecraftID;
    SBN_MsgType_t        MsgType;
    SBN_MsgSz_t          MsgSz;
    uint8                Msg[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
} RecvNetTaskData_t;

/**
 * \brief Receive task created for each net-based connection.
 * Spanwed from PeerPoll()
 */
void SBN_RecvNetTask(void)
{
    static const char FAIL_PREFIX_STARTUP[] = "ERROR: could not start SBN Receive Net Task:";
    static const char FAIL_PREFIX_RUNNING[] = "ERROR: during SBN Receive Net Task:";
    RecvNetTaskData_t D;
    memset(&D, 0, sizeof(D));

    D.RecvTaskID = OS_TaskGetId();

    for (D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        if (D.Net->RecvTaskID == D.RecvTaskID)
        {
            break;
        } /* end if */
    }     /* end for */

    if (D.NetIdx == SBN.NetCnt)
    {
        EVSSendErr(SBN_PEERTASK_EID, "%s unable to connect task to net struct", FAIL_PREFIX_STARTUP);
        return;
    } /* end if */

    while (1)
    {
        EVSSendDbg(SBN_PEERTASK_EID,"Try to receive from net...");
        D.Status = D.Net->IfOps->RecvFromNet(D.Net, &D.MsgType, &D.MsgSz, &D.ProcessorID, &D.SpacecraftID, &D.Msg);

        if (D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }             /* end if */

        if (D.Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_PEERTASK_EID, "%s RecvFromNet failed for net %d: status=0x%08X", FAIL_PREFIX_RUNNING, D.NetIdx, D.Status);
            break;
        } /* end if */

        D.Peer = SBN_GetPeer(D.Net, D.ProcessorID, D.SpacecraftID);
        if (!D.Peer)
        {
            EVSSendErr(SBN_PEERTASK_EID, "unknown peer (ProcessorID=%d)", (int)D.ProcessorID);
            break;
        } /* end if */

        OS_GetLocalTime(&D.Peer->LastRecv);

        D.Status = SBN_ProcessNetMsg(D.Net, D.MsgType, D.ProcessorID, D.SpacecraftID, D.MsgSz, &D.Msg);

        if (D.Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_PEERTASK_EID, "SBN_ProcessNetMsg failed: 0x%08X", D.Status);
            break;
        } /* end if */
    }     /* end while */

    /* Unset the task id so that it can be created if necessary */
    D.Net->RecvTaskID = 0;
} /* end SBN_RecvNetTask() */

/**
 * Checks all interfaces for messages from peers.
 * Receive messages from the specified peer, injecting them onto the local
 * software bus.
 */
SBN_Status_t SBN_RecvNetMsgs(void)
{
    SBN_Status_t SBN_Status = 0;

    SBN_NetIdx_t NetIdx = 0;
    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        SBN_MsgType_t       MsgType;
        SBN_MsgSz_t         MsgSz;
        CFE_ProcessorID_t   ProcessorID;
        CFE_SpacecraftID_t  SpacecraftID;

        if (Net->TaskFlags & SBN_TASK_RECV)
        {
            continue; /* separate task handles receiving from a net */
        }             /* end if */

        if (Net->IfOps->RecvFromNet)
        {
            int MsgCnt = 0;
            // TODO: make configurable
            for (MsgCnt = 0; MsgCnt < 100; MsgCnt++) /* read at most 100 messages from the net */
            {
                /*memset(SBN.MsgBuffer, 0, sizeof(SBN.MsgBuffer));*/

                SBN_Status = Net->IfOps->RecvFromNet(Net, &MsgType, &MsgSz, &ProcessorID, &SpacecraftID, SBN.MsgBuffer);

                if (SBN_Status == SBN_IF_EMPTY)
                {
                    break; /* no (more) messages for this net, continue to next net */
                }          /* end if */

                /* for UDP, the message received may not be from the peer
                 * expected.
                 */
                SBN_PeerInterface_t *Peer = SBN_GetPeer(Net, ProcessorID, SpacecraftID);

                if (!Peer)
                {
                    EVSSendInfo(SBN_PEERTASK_EID, "unknown peer (ProcessorID=%d)", (int)ProcessorID);
                    /* may be a misconfiguration on my part...? continue processing msgs... */
                    continue;
                } /* end if */

                OS_GetLocalTime(&Peer->LastRecv);
                SBN_ProcessNetMsg(Net, MsgType, ProcessorID, SpacecraftID, MsgSz, SBN.MsgBuffer); /* ignore errors */
            }                                                             /* end for */
        }
        else if (Net->IfOps->RecvFromPeer)
        {
            SBN_PeerIdx_t PeerIdx = 0;
            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

                int MsgCnt = 0;
                // TODO: make configurable
                for (MsgCnt = 0; MsgCnt < 100; MsgCnt++) /* read at most 100 messages from peer */
                {
                    CFE_ProcessorID_t ProcessorID = 0;
                    CFE_SpacecraftID_t SpacecraftID = 0;
                    SBN_MsgType_t     MsgType     = 0;
                    SBN_MsgSz_t       MsgSz       = 0;

                    memset(SBN.MsgBuffer, 0, sizeof(SBN.MsgBuffer));

                    SBN_Status = Net->IfOps->RecvFromPeer(Net, Peer, &MsgType, &MsgSz, &ProcessorID, &SpacecraftID, SBN.MsgBuffer);

                    if (SBN_Status == SBN_IF_EMPTY)
                    {
                        break; /* no (more) messages for this peer, continue to next peer */
                    }          /* end if */

                    OS_GetLocalTime(&Peer->LastRecv);

                    SBN_Status = SBN_ProcessNetMsg(Net, MsgType, ProcessorID, SpacecraftID, MsgSz, SBN.MsgBuffer);

                    if (SBN_Status != SBN_SUCCESS)
                    {
                        break; /* continue to next peer */
                    }          /* end if */
                }              /* end for */
            }                  /* end for */
        }
        else
        {
            EVSSendErr(SBN_PEER_EID, "neither RecvFromPeer nor RecvFromNet defined for net #%d", (int)NetIdx);

            /* meanwhile, continue to next net... */
        } /* end if */
    }     /* end for */

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
    SBN_NetInterface_t *Net        = Peer->Net;
    SBN_Status_t        SBN_Status = SBN_SUCCESS;

    if (Peer->SendTaskID)
    {
        if (OS_MutSemTake(SBN.SendMutex) != OS_SUCCESS)
        {
            EVSSendErr(SBN_PEER_EID, "unable to take send mutex");
            return SBN_ERROR;
        } /* end if */
    }     /* end if */

    SBN_Status = Net->IfOps->Send(Peer, MsgType, MsgSz, Msg);

    if (SBN_Status != SBN_SUCCESS)
    {
        Peer->SendErrCnt++;
    } else {
        Peer->SendCnt++;
    } /* end if */

    /* for clients that need a poll or heartbeat, update time even when failing */
    OS_GetLocalTime(&Peer->LastSend);

    if (Peer->SendTaskID)
    {
        if (OS_MutSemGive(SBN.SendMutex) != OS_SUCCESS)
        {
            EVSSendErr(SBN_PEER_EID, "unable to give send mutex");
            return SBN_ERROR;
        } /* end if */
    }     /* end if */

    return SBN_Status;
} /* end SBN_SendNetMsg */

typedef struct
{
    SBN_Status_t         Status;
    SBN_NetIdx_t         NetIdx;
    SBN_PeerIdx_t        PeerIdx;
    OS_TaskID_t          SendTaskID;
    CFE_MSG_Message_t   *MsgPtr;
    CFE_SB_MsgId_t       MsgID;
    SBN_NetInterface_t  *Net;
    SBN_PeerInterface_t *Peer;
} SendTaskData_t;

/**
 * \brief When a peer is connected, a task is created to listen to the relevant
 * pipe for messages to send to that peer.
 */
void SBN_SendTask(void)
{
    SendTaskData_t   D;
    SBN_Filter_Ctx_t Filter_Context;
    CFE_MSG_Size_t   MsgSz = 0;

    Filter_Context.MyProcessorID  = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID = CFE_PSP_GetSpacecraftId();

    memset(&D, 0, sizeof(D));

    D.SendTaskID = OS_TaskGetId();

    for (D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        for (D.PeerIdx = 0; D.PeerIdx < D.Net->PeerCnt; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if (D.Peer->SendTaskID == D.SendTaskID)
            {
                break;
            } /* end if */
        }     /* end for */

        if (D.PeerIdx < D.Net->PeerCnt)
        {
            break; /* found a ringer */
        }          /* end if */
    }              /* end for */

    if (D.NetIdx == SBN.NetCnt)
    {
        EVSSendErr(SBN_PEER_EID, "error connecting send task");
        return;
    } /* end if */

    while (1)
    {
        SBN_ModuleIdx_t FilterIdx = 0;

        if (!D.Peer->Connected)
        {
            OS_TaskDelay(SBN_MAIN_LOOP_DELAY);
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
        }     /* end for */

        if (FilterIdx < D.Peer->FilterCnt)
        {
            /* one of the above filters suggested rejecting this message */
            continue;
        } /* end if */

        if(CFE_MSG_GetSize(D.MsgPtr, &MsgSz) != CFE_SUCCESS)
        {
            continue;
        } /* end if */

        D.Status = SBN_SendNetMsg(SBN_APP_MSG, MsgSz, D.MsgPtr, D.Peer);

        if (D.Status == SBN_ERROR)
        {
            break;
        } /* end if */
    }     /* end while */

    /* mark peer as not having a task so that sending will create a new one */
    D.Peer->SendTaskID = 0;
} /* end SBN_SendTask() */

/**
 * Iterate through all peers, examining the pipe to see if there are messages
 * I need to send to that peer.
 */
static SBN_Status_t CheckPeerPipes(void)
{
    CFE_Status_t       CFE_Status;
    int                ReceivedFlag = 0, iter = 0;
    CFE_MSG_Message_t *MsgPtr = NULL;
    CFE_MSG_Size_t     MsgSz = 0;
    SBN_Filter_Ctx_t   Filter_Context;

    Filter_Context.MyProcessorID  = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID = CFE_PSP_GetSpacecraftId();

    /**
     * \note This processes one message per peer, then start again until no
     * peers have pending messages. At max only process SBN_MAX_MSG_PER_WAKEUP
     * per peer per wakeup otherwise I will starve other processing.
     */
    for (iter = 0; iter < SBN_MAX_MSG_PER_WAKEUP; iter++)
    {
        ReceivedFlag = 0;

        SBN_NetIdx_t NetIdx = 0;
        for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
        {
            SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

            SBN_PeerIdx_t PeerIdx = 0;
            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                SBN_ModuleIdx_t      FilterIdx = 0;
                SBN_PeerInterface_t *Peer      = &Net->Peers[PeerIdx];

                // Poll peer here to detect disconnections and to reconnect
                if(Net->IfOps->PollPeer(Peer) != SBN_SUCCESS) {
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
                    if (!Peer->SendTaskID)
                    {
                        /* TODO: logic/controls to prevent hammering? */
                        char SendTaskName[32];

                        snprintf(SendTaskName, 32, "sendT_%d_%d_%d", (int)NetIdx, (int)(Peer->ProcessorID), (int)(Peer->SpacecraftID));
                        CFE_Status = CFE_ES_CreateChildTask(
                            &(Peer->SendTaskID), SendTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_SendTask, NULL,
                            CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(SendTaskData_t), 0, 0);

                        if (CFE_Status != CFE_SUCCESS)
                        {
                            EVSSendErr(SBN_PEER_EID, "error creating send task for peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
                            return SBN_ERROR;
                        } /* end if */
                    }     /* end if */

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
                    SBN_Status_t SBN_Status;

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
                }     /* end for */

                if (FilterIdx < Peer->FilterCnt)
                {
                    /* one of the above filters suggested rejecting this message */
                    continue;
                } /* end if */

                if (CFE_MSG_GetSize(MsgPtr, &MsgSz) != CFE_SUCCESS)
                {
                    continue;
                } /* end if */

                SBN_SendNetMsg(SBN_APP_MSG, MsgSz, MsgPtr, Peer);
            } /* end for */
        }     /* end for */

        if (!ReceivedFlag)
        {
            break;
        } /* end if */
    }     /* end for */
    return SBN_SUCCESS;
} /* end CheckPeerPipes */

/**
 * Iterate through all nets and create receive tasks if they do not yet exist.
 */
static SBN_Status_t PeerPoll(void)
{
    CFE_Status_t CFE_Status;
    SBN_NetIdx_t NetIdx = 0;
    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

        if (Net->IfOps->RecvFromNet && Net->TaskFlags & SBN_TASK_RECV)
        {
            if (!Net->RecvTaskID)
            {
                EVSSendInfo(SBN_PEER_EID, "Creating recv task for net %d", (int)NetIdx);

                /* TODO: add logic/controls to prevent hammering */
                char RecvTaskName[32];
                snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_rs_%d", (int)NetIdx);
                CFE_Status = CFE_ES_CreateChildTask(
                    &(Net->RecvTaskID), RecvTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_RecvNetTask, NULL,
                    CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(RecvNetTaskData_t), 0, 0);

                if (CFE_Status != CFE_SUCCESS)
                {
                    EVSSendErr(SBN_PEER_EID, "error creating task for net %d",(int)NetIdx);
                    return SBN_ERROR;
                } /* end if */
            }     /* end if */
        }
        else
        {
            SBN_PeerIdx_t PeerIdx = 0;
            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

                if (Net->IfOps->RecvFromPeer && Peer->TaskFlags & SBN_TASK_RECV)
                {
                    if (!Peer->RecvTaskID)
                    {
                        /* TODO: add logic/controls to prevent hammering */
                        char RecvTaskName[32];
                        snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_recv_%d", (int)PeerIdx);
                        CFE_Status = CFE_ES_CreateChildTask(
                            &(Peer->RecvTaskID), RecvTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_RecvPeerTask, NULL,
                            CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(RecvPeerTaskData_t), 0, 0);
                        /* TODO: more accurate stack size required */

                        if (CFE_Status != CFE_SUCCESS)
                        {
                            EVSSendErr(SBN_PEER_EID, "error creating task for %d", (int)(Peer->ProcessorID));
                            return SBN_ERROR;
                        } /* end if */
                    }     /* end if */
                }
                else
                {
                    Net->IfOps->PollPeer(Peer);
                } /* end if */
            }     /* end for */
        }         /* end if */
    }             /* end for */

    return SBN_SUCCESS;
} /* end PeerPoll */

/**
 * Loops through all hosts and peers, initializing all.
 *
 * @return SBN_SUCCESS if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
static SBN_Status_t InitInterfaces(void)
{
    if (SBN.NetCnt < 1)
    {
        EVSSendErr(SBN_PEER_EID, "no networks configured");

        return SBN_ERROR;
    } /* end if */

    SBN_NetIdx_t NetIdx = 0;
    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        EVSSendInfo(SBN_PEER_EID, "initializing net: %d", (int)NetIdx);
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

        if (!Net->Configured)
        {
            EVSSendErr(SBN_PEER_EID, "network #%d not configured", NetIdx);

            return SBN_ERROR;
        } /* end if */

        Net->IfOps->InitNet(Net);

        SBN_PeerIdx_t PeerIdx = 0;
        EVSSendInfo(SBN_PEER_EID, "Net %d has %d peers", NetIdx, Net->PeerCnt);
        for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            EVSSendInfo(SBN_PEER_EID, "initializing net: %d peer: %d: sc: %d cpu: %d", (int)NetIdx, (int)PeerIdx, Peer->SpacecraftID, Peer->ProcessorID);

            Net->IfOps->InitPeer(Peer);
        } /* end for */
    }     /* end for */

    EVSSendInfo(SBN_INIT_EID, "configured, %d nets", SBN.NetCnt);

    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        EVSSendInfo(SBN_INIT_EID, "net %d has %d peers", NetIdx, SBN.Nets[NetIdx].PeerCnt);
    } /* end for */

    return SBN_SUCCESS;
} /* end InitInterfaces */

/**
 * This function waits for the scheduler (SCH) to wake this code up, so that
 * nothing transpires until the cFE is fully operational.
 *
 * @param[in] iTimeOut The time to wait for the scheduler to notify this code.
 * @return CFE_SUCCESS on success, otherwise an error value.
 */
static SBN_Status_t WaitForWakeup(int32 iTimeOut)
{
    CFE_Status_t       CFE_Status = CFE_SUCCESS;
    SBN_Status_t       SBN_Status = SBN_SUCCESS;
    CFE_MSG_Message_t *MsgPtr    = 0;

    /* Wait for WakeUp messages from scheduler */
    CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&MsgPtr, SBN.CmdPipe, iTimeOut);

    switch (CFE_Status)
    {
        case CFE_SB_NO_MESSAGE:
        case CFE_SB_TIME_OUT:
            break;
        case CFE_SUCCESS:
            SBN_HandleCommand(MsgPtr);
            break;
        default:
            return SBN_ERROR;
    } /* end switch */

    /* For sbn, we still want to perform cyclic processing
    ** if the WaitForWakeup time out
    ** cyclic processing at timeout rate
    */
    CFE_ES_PerfLogEntry(SBN_PERF_RECV_ID);

    SBN_RecvNetMsgs();

    SBN_Status = SBN_CheckSubscriptionPipe();
    switch(SBN_Status) {
      case SBN_IF_EMPTY:
        break;
      case SBN_ERROR:
        EVSSendErr(SBN_PEER_EID, "SBN_CheckSubscriptionPipe failed.");
        break;
      default:
        /* no error */
        break;
    };

    CheckPeerPipes();

    PeerPoll();

    CFE_ES_PerfLogExit(SBN_PERF_RECV_ID);

    return SBN_SUCCESS;
} /* end WaitForWakeup */

/**
 * Load Protocol or Filter Module from the table
 *
 * Cleaned up by UnloadModules()
 */
static cpuaddr LoadConf_Module(SBN_Module_Entry_t *e, CFE_ES_ModuleID_t *ModuleIDPtr)
{
    cpuaddr StructAddr;

    EVSSendInfo(SBN_TBL_EID, "checking if module (%s) already loded", e->Name);
    if (OS_SymbolLookup(&StructAddr, e->LibSymbol) != OS_SUCCESS) /* try loading it if it's not already loaded */
    {
        EVSSendInfo(SBN_TBL_EID, "symbol not yet loaded (%s)", e->LibSymbol);
        if (e->LibFileName[0] == '\0')
        {
            EVSSendErr(SBN_TBL_EID, "invalid module (Name=%s)", e->Name);
            return 0;
        }

        EVSSendInfo(SBN_TBL_EID, "loading module (Name=%s, File=%s)", e->Name, e->LibFileName);
        if (OS_ModuleLoad(ModuleIDPtr, e->Name, e->LibFileName, OS_MODULE_FLAG_GLOBAL_SYMBOLS) != OS_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "invalid module file (Name=%s LibFileName=%s)", e->Name, e->LibFileName);
            return 0;
        } /* end if */

        EVSSendInfo(SBN_TBL_EID, "validating symbol load (%s)", e->LibSymbol);
        if (OS_SymbolLookup(&StructAddr, e->LibSymbol) != OS_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "invalid symbol (Name=%s LibSymbol=%s)", e->Name, e->LibSymbol);
            return 0;
        }
    } else {
      EVSSendInfo(SBN_TBL_EID, "symbol already loaded (%s)", e->LibSymbol);
    } /* end if */

    return StructAddr;
} /* end LoadConf_Module */

/**
 * Load the filters from the table.
 * Cleaned up by UnloadModules()
 *
 * @param FilterModules[in] - The filter module entries in the table.
 * @param FilterModuleCnt[in] - The number of entries in FilterModules.
 * @param ModuleNames[in] - The array of filters this peer/net wishes to use.
 * @param FilterFns[out] - The function pointers for the filters requested.
 * @return The number of entries in FilterFns.
 */
static SBN_ModuleIdx_t LoadConf_Filters(SBN_Module_Entry_t *FilterModules, SBN_ModuleIdx_t FilterModuleCnt,
                                        SBN_FilterInterface_t **ConfFilters,
                                        char ModuleNames[SBN_MAX_FILTERS_PER_PEER][SBN_MAX_MOD_NAME_LEN],
                                        SBN_FilterInterface_t **Filters)
{
    int             i         = 0;
    SBN_ModuleIdx_t FilterCnt = 0;

    memset(FilterModules, 0, sizeof(*FilterModules) * FilterCnt);

    for (i = 0; *ModuleNames[i] && i < SBN_MAX_FILTERS_PER_PEER; i++)
    {
        SBN_ModuleIdx_t FilterIdx = 0;
        for (FilterIdx = 0; FilterIdx < FilterModuleCnt; FilterIdx++)
        {
            if (strcmp(ModuleNames[i], FilterModules[FilterIdx].Name) == 0)
            {
                break;
            } /* end if */
        }     /* end for */

        if (FilterIdx == FilterModuleCnt)
        {
            EVSSendErr(SBN_TBL_EID, "Invalid filter name: %s", ModuleNames[i]);
            continue;
        } /* end if */

        Filters[FilterCnt++] = ConfFilters[FilterIdx];
    } /* end for */

    return FilterCnt;
} /* end LoadConf_Filters() */

static SBN_Status_t LoadConf(void)
{
    SBN_ConfTbl_t *        TblPtr    = NULL;
    SBN_ModuleIdx_t        ModuleIdx = 0;
    SBN_PeerIdx_t          PeerIdx   = 0;
    SBN_FilterInterface_t *Filters[SBN_MAX_MOD_CNT];
    SBN_ProtocolOutlet_t   Outlet = {.PackMsg = SBN_PackMsg, .UnpackMsg = SBN_UnpackMsg, .Connected = SBN_Connected, .Disconnected = SBN_Disconnected, .SendNetMsg = SBN_SendNetMsg, .GetPeer = SBN_GetPeer};

    memset(Filters, 0, sizeof(Filters));

    if (CFE_TBL_GetAddress((void **)&TblPtr, SBN.ConfTblHandle) != CFE_TBL_INFO_UPDATED)
    {
        EVSSendErr(SBN_TBL_EID, "unable to get conf table address");
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return SBN_ERROR;
    } /* end if */

    /* load protocol modules */
    EVSSendDbg(SBN_TBL_EID, "Loading protocol modules...");
    for (ModuleIdx = 0; ModuleIdx < TblPtr->ProtocolCnt; ModuleIdx++)
    {
        CFE_ES_ModuleID_t ModuleID = 0;

        SBN_IfOps_t *Ops = (SBN_IfOps_t *)LoadConf_Module(&TblPtr->ProtocolModules[ModuleIdx], &ModuleID);

        if (Ops == NULL)
        {
            /* LoadConf_Module already generated an event */
            return SBN_ERROR;
        } /* end if */

        EVSSendInfo(SBN_TBL_EID, "initializing protocol module");
        if (Ops->InitModule(SBN_PROTOCOL_VERSION, TblPtr->ProtocolModules[ModuleIdx].BaseEID, &Outlet) != CFE_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "error in protocol init");
            return SBN_ERROR;
        } /* end if */
        EVSSendInfo(SBN_TBL_EID, "protocol module initialized");

        SBN.IfOps[ModuleIdx]           = Ops;
        SBN.ProtocolModules[ModuleIdx] = ModuleID;
    } /* end for */

    /* load filter modules */
    EVSSendDbg(SBN_TBL_EID, "Loading filter modules...");
    for (ModuleIdx = 0; ModuleIdx < TblPtr->FilterCnt; ModuleIdx++)
    {
        CFE_ES_ModuleID_t ModuleID = 0;

        Filters[ModuleIdx] = (SBN_FilterInterface_t *)LoadConf_Module(&TblPtr->FilterModules[ModuleIdx], &ModuleID);

        if (Filters[ModuleIdx] == NULL)
        {
            /* LoadConf_Module already generated an event */
            return SBN_ERROR;
        } /* end if */

        EVSSendInfo(SBN_TBL_EID, "initializing filter module");
        if (Filters[ModuleIdx]->InitModule(SBN_FILTER_VERSION, TblPtr->FilterModules[ModuleIdx].BaseEID) != CFE_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "error in filter init");
            return SBN_ERROR;
        } /* end if */
        EVSSendInfo(SBN_TBL_EID, "filter module initialized");

        SBN.FilterModules[ModuleIdx] = ModuleID;
    } /* end for */

    /* load nets and peers */
    for (PeerIdx = 0; PeerIdx < TblPtr->PeerCnt; PeerIdx++)
    {
        SBN_Peer_Entry_t *e = &TblPtr->Peers[PeerIdx];

        EVSSendInfo(SBN_TBL_EID, "configuring peer (SC=%d, CPU=%d)...", e->SpacecraftID, e->ProcessorID);

        for (ModuleIdx = 0; ModuleIdx < TblPtr->ProtocolCnt; ModuleIdx++)
        {
            if (strcmp(TblPtr->ProtocolModules[ModuleIdx].Name, e->ProtocolName) == 0)
            {
                break;
            }
        }

        if (ModuleIdx == TblPtr->ProtocolCnt)
        {
            EVSSendCrit(SBN_TBL_EID, "invalid module name %s", e->ProtocolName);
            return SBN_ERROR;
        } /* end if */

        if (e->NetNum < 0 || e->NetNum >= SBN_MAX_NETS)
        {
            EVSSendCrit(SBN_TBL_EID, "network index too large (%d>%d)", e->NetNum, SBN_MAX_NETS);
            return SBN_ERROR;
        } /* end if */

        /* Net initialization */
        if (e->NetNum + 1 > SBN.NetCnt)
        {
            EVSSendInfo(SBN_TBL_EID, "found new highest net id: %d", e->NetNum);
            SBN.NetCnt = e->NetNum + 1;
            SBN.Nets[e->NetNum].PeerCnt = 0;
            EVSSendInfo(SBN_TBL_EID, "increasing net count to %d", SBN.NetCnt);
        } /* end if */

        SBN_NetInterface_t *Net = &SBN.Nets[e->NetNum];
        /* Reset peer count since we're initializing the net */
        if (e->ProcessorID == CFE_PSP_GetProcessorId() && e->SpacecraftID == CFE_PSP_GetSpacecraftId())
        {
            EVSSendInfo(SBN_TBL_EID, "peer is this processor: loading net %d", e->NetNum);
            Net->Configured  = true;
            Net->ProtocolIdx = ModuleIdx;
            Net->IfOps       = SBN.IfOps[ModuleIdx];
            Net->IfOps->LoadNet(Net, (const char *)e->Address);

            Net->FilterCnt =
                LoadConf_Filters(TblPtr->FilterModules, TblPtr->FilterCnt, Filters, e->Filters, Net->Filters);

            Net->TaskFlags = e->TaskFlags;
        }
        else
        {
            EVSSendInfo(SBN_TBL_EID, "peer is other processor: loading peer onto net %d", e->NetNum);
            SBN_PeerInterface_t *Peer = &Net->Peers[Net->PeerCnt++];
            memset(Peer, 0, sizeof(*Peer));
            Peer->Net          = Net;
            Peer->ProcessorID  = e->ProcessorID;
            Peer->SpacecraftID = e->SpacecraftID;

            Peer->FilterCnt =
                LoadConf_Filters(TblPtr->FilterModules, TblPtr->FilterCnt, Filters, e->Filters, Peer->Filters);

            SBN.IfOps[ModuleIdx]->LoadPeer(Peer, (const char *)e->Address);

            Peer->TaskFlags = e->TaskFlags;
        } /* end if */
    }     /* end for */

    /* address only needed at load time, release */
    if (CFE_TBL_ReleaseAddress(SBN.ConfTblHandle) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to release address of conf tbl");
        return SBN_ERROR;
    } /* end if */

    /* ...but we keep the handle so we can be notified of updates */
    return SBN_SUCCESS;
} /* end LoadConf() */

static uint32 UnloadConf(void)
{
    uint32 Status;

    EVSSendInfo(SBN_TBL_EID, "unloading configuration");

    if((Status = UnloadNets()) != SBN_SUCCESS) {
      EVSSendCrit(SBN_TBL_EID, "unable to unload nets");
      return Status;
    }

    if((Status = UnloadModules()) != SBN_SUCCESS) {
      EVSSendCrit(SBN_TBL_EID, "unable to unload modules");
      return Status;
    }

    return SBN_SUCCESS;
} /* end UnloadConf() */

static SBN_Status_t UnloadPeer(SBN_PeerInterface_t *Peer)
{
  SBN_RemoveAllSubsFromPeer(Peer);

  SBN_Disconnected(Peer);

  if (Peer->TaskFlags & SBN_TASK_SEND)
  {
    if (Peer->SendTaskID)
    {
      if(CFE_ES_DeleteChildTask(Peer->SendTaskID) != CFE_SUCCESS) {
        EVSSendCrit(SBN_TBL_EID, "unable to delete send task for peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
        return SBN_ERROR;
      }
    }
  }

  if (Peer->TaskFlags & SBN_TASK_RECV)
  {
    if (Peer->RecvTaskID)
    {
      if(CFE_ES_DeleteChildTask(Peer->RecvTaskID) != CFE_SUCCESS) {
        EVSSendCrit(SBN_TBL_EID, "unable to delete recv task for peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
      }
    }
  }

  // Reset peer description
  Peer->ProcessorID = 0;
  Peer->SpacecraftID = 0;
  Peer->Net = NULL;
  Peer->TaskFlags = 0;
  Peer->Pipe = 0;
  Peer->FilterCnt = 0;

  return SBN_SUCCESS;
}

static SBN_Status_t UnloadNets(void)
{
    uint32 Status;

    int NetIdx = 0;
    for (NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        Net->Configured = false;

        if(Net->RecvTaskID != 0) {
          if(CFE_ES_DeleteChildTask(Net->RecvTaskID) != CFE_SUCCESS) {
            EVSSendCrit(SBN_TBL_EID, "unable to delete receive task %d", NetIdx);
          }

          Net->RecvTaskID = 0;
        }

        // UnloadNet assumes Peers are still valid
        if ((Status = Net->IfOps->UnloadNet(Net)) != SBN_SUCCESS)
        {
            EVSSendCrit(SBN_TBL_EID, "unable to unload network %d", NetIdx);
            return Status;
        } /* end if */
        else
        {
            EVSSendInfo(SBN_TBL_EID, "Terminated net: %d", NetIdx);
        }

        SBN_PeerIdx_t PeerIdx = 0;
        for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
          SBN_PeerInterface_t *Peer      = &Net->Peers[PeerIdx];

          UnloadPeer(Peer);
        }

        // Peers were cleared, reset the count
        Net->PeerCnt = 0;
    }/* end for */

    SBN.NetCnt = 0;

    return SBN_SUCCESS;
}

static uint32 LoadConfTbl(void)
{
    int32 Status = CFE_SUCCESS;

    if ((Status = CFE_TBL_Register(&SBN.ConfTblHandle, "SBN_ConfTbl", sizeof(SBN_ConfTbl_t), CFE_TBL_OPT_DEFAULT,
                                   NULL)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to register conf tbl handle");
        return Status;
    } /* end if */

    if ((Status = CFE_TBL_Load(SBN.ConfTblHandle, CFE_TBL_SRC_FILE, SBN_CONF_TBL_FILENAME)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to load conf tbl %s", SBN_CONF_TBL_FILENAME);
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return Status;
    } /* end if */

    if ((Status = CFE_TBL_Manage(SBN.ConfTblHandle)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to manage conf tbl");
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return Status;
    } /* end if */

    if ((Status = CFE_TBL_NotifyByMessage(SBN.ConfTblHandle, CFE_SB_ValueToMsgId(SBN_CMD_MID), SBN_TBL_CC, 0)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return Status;
    } /* end if */

    return SBN_SUCCESS;
} /* end LoadConfTbl() */

static SBN_Status_t TeardownSubPipe(void)
{
    CFE_Status_t Status;

    /* Delete pipe for subscribes and unsubscribes from SB */
    Status = CFE_SB_DeletePipe(SBN.SubPipe);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to delete subscription pipe (Status=%d)", (int)Status);
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
}

static SBN_Status_t SetupSubPipe(void)
{
    CFE_Status_t Status;

    /* Create pipe for subscribes and unsubscribes from SB */
    Status = CFE_SB_CreatePipe(&SBN.SubPipe, SBN_SUB_PIPE_DEPTH, "SBNSubPipe");
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to create subscription pipe (Status=%d)", (int)Status);
        return SBN_ERROR;
    } /* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID), SBN.SubPipe, SBN_MAX_ALLSUBS_PKTS_ON_PIPE);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to allsubs (Status=%d)", (int)Status);
        return SBN_ERROR;
    } /* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID), SBN.SubPipe, SBN_MAX_ONESUB_PKTS_ON_PIPE);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to sub (Status=%d)", (int)Status);
        return SBN_ERROR;
    } /* end if */


    return SBN_SUCCESS;
}

static SBN_Status_t Init(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not initialize SBN:";

    /* Load the configuration from the table */
    if(LoadConf() != SBN_SUCCESS)
    {
        return SBN_ERROR;
    }

    if (InitInterfaces() == SBN_ERROR)
    {
        EVSSendErr(SBN_INIT_EID, "%s unable to initialize interfaces", FAIL_PREFIX);
        return SBN_ERROR;
    } /* end if */

    if(SetupSubPipe() != SBN_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s unable to set up subscription pipe", FAIL_PREFIX);
        return SBN_ERROR;
    }

    const char *bit_order =
#ifdef SOFTWARE_BIG_BIT_ORDER
        "big-endian";
#else  /* !SOFTWARE_BIG_BIT_ORDER */
        "little-endian";
#endif /* SOFTWARE_BIG_BIT_ORDER */

    EVSSendInfo(SBN_INIT_EID,
                "initialized (ProcessorID=%d SpacecraftId=%d %s "
                "SBN.AppID=%d...",
                (int)CFE_PSP_GetProcessorId(), (int)CFE_PSP_GetSpacecraftId(), bit_order, (int)SBN.AppID);

    EVSSendInfo(SBN_INIT_EID, "...SBN_IDENT=%s CMD_MID=0x%04X)", SBN_IDENT, SBN_CMD_MID);

    SBN_InitializeCounters();

    /* Wait for event from SB saying it is initialized OR a response from SB
       to the above messages. true means it needs to re-send subscription
       requests */

    while(CFE_ES_WaitForSystemState(CFE_ES_SystemState_OPERATIONAL, 1000) == CFE_ES_OPERATION_TIMED_OUT) /* do nothing but keep waiting */;

    EVSSendInfo(SBN_INIT_EID, "Re-sending SB subscription requests...");
    SBN_SendSubsRequests();

    EVSSendInfo(SBN_INIT_EID, "SBN initialized.");

    return SBN_SUCCESS;
}

static SBN_Status_t Cleanup(void)
{
    SBN_Status_t Status;

    if ((Status = TeardownSubPipe()) != SBN_SUCCESS) {
        EVSSendErr(SBN_PEER_EID, "unable to tear down sub pipe");
        return Status;
    }

    if ((Status = UnloadConf()) != SBN_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "unable to unload configuration");
        return Status;
    } /* end if */

    // cleanup subs
    memset(SBN.Subs, 0, sizeof(SBN.Subs));

    SBN.SubCnt = 0;

    if (CFE_TBL_Update(SBN.ConfTblHandle) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "unable to update table");
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
}

/** \brief SBN Main Routine */
void SBN_AppMain(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not start SBN:";
    CFE_ES_TaskInfo_t TaskInfo;
    uint32            Status    = CFE_SUCCESS;
    uint32            RunStatus = CFE_ES_RunStatus_APP_RUN, AppID = 0;

    if (CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER != CFE_SUCCESS))
        return;

    if (CFE_ES_GetAppID(&AppID) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_INIT_EID, "%s unable to get AppID", FAIL_PREFIX);
        return;
    }

    SBN.AppID = AppID;

    /* load my TaskName so I can ignore messages I send out to SB */
    uint32 TskId = OS_TaskGetId();
    if ((Status = CFE_ES_GetTaskInfo(&TaskInfo, TskId)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s SBN failed to get task info (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    strncpy(SBN.App_FullName, (const char *)TaskInfo.TaskName, OS_MAX_API_NAME - 1);
    SBN.App_FullName[OS_MAX_API_NAME - 1] = '\0';

    /** Create mutex for send tasks */
    Status = OS_MutSemCreate(&(SBN.SendMutex), "sbn_send_mutex", 0);

    if (Status != OS_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s error creating mutex for send tasks", FAIL_PREFIX);
        return;
    }

    /** Create mutex for coordinating live reconfiguration **/
    Status = OS_MutSemCreate(&(SBN.ConfMutex), "sbn_conf_mutex", 0);
    if (Status != OS_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s error creating mutex for configuiration", FAIL_PREFIX);
        return;
    }

    /* Create pipe for HK requests and gnd commands */
    /* TODO: make configurable depth */
    Status = CFE_SB_CreatePipe(&SBN.CmdPipe, 20, "SBNCmdPipe");
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s failed to create command pipe (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SBN_CMD_MID), SBN.CmdPipe);
    if (Status == CFE_SUCCESS)
    {
        EVSSendInfo(SBN_INIT_EID, "SBN subscribed to command pipe SBN_CMD_MID 0x%04X", SBN_CMD_MID);
    }
    else
    {
        EVSSendErr(SBN_INIT_EID, "%s failed to subscribe to command pipe (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    CFE_ES_WaitForStartupSync(10000);

    /* Load the table */
    if (LoadConfTbl() != SBN_SUCCESS)
    {
        /* the LoadConfTbl() functions will generate events */
        EVSSendErr(SBN_INIT_EID, "%s failed to load configuration table", FAIL_PREFIX);
        return;
    } /* end if */

    if(Init() != SBN_SUCCESS) {
        RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /* Loop Forever */
    while (CFE_ES_RunLoop(&RunStatus))
    {
        if (OS_MutSemTake(SBN.ConfMutex) != OS_SUCCESS)
        {
            EVSSendErr(SBN_PEER_EID, "ERROR: SBN run loop unable to take configuration mutex");
            break;
        } /* end if */

        WaitForWakeup(SBN_MAIN_LOOP_DELAY);

        if (OS_MutSemGive(SBN.ConfMutex) != OS_SUCCESS) {
            EVSSendErr(SBN_PEER_EID, "ERROR: SBN run loop unable to give configuration mutex");
            break;
        }
    } /* end while */

    if(Cleanup() != SBN_SUCCESS) {
      EVSSendErr(SBN_INIT_EID, "ERROR: could not clean up SBN");
    }

    CFE_ES_ExitApp(RunStatus);
} /* end SBN_AppMain */

/**
 * Sends a message to a peer.
 * @param[in] MsgType The type of the message (application data, SBN protocol)
 * @param[in] ProcessorID The ProcessorID to send this message to.
 * @param[in] MsgSz The size of the message (in bytes).
 * @param[in] Msg The message contents.
 *
 * @return SBN_SUCCESS on successful processing, SBN_ERROR otherwise
 */
SBN_Status_t SBN_ProcessNetMsg(SBN_NetInterface_t *Net, SBN_MsgType_t MsgType, CFE_ProcessorID_t ProcessorID,
                               CFE_SpacecraftID_t SpacecraftID,
                               SBN_MsgSz_t MsgSize, void *Msg)
{
    static const char FAIL_PREFIX[] = "ERROR: could not process peer message:";
    SBN_Status_t         SBN_Status = SBN_SUCCESS;
    CFE_Status_t         CFE_Status = CFE_SUCCESS;
    SBN_PeerInterface_t *Peer       = SBN_GetPeer(Net, ProcessorID, SpacecraftID);

    if (!Peer)
    {
        EVSSendErr(SBN_PEERTASK_EID, "%s unknown peer (ProcessorID=%d)", FAIL_PREFIX, (int)ProcessorID);
        return SBN_ERROR;
    } /* end if */

    /* Check if message is protocol-specific (unkown to SBN core) */
    if(MsgType & SBN_MODULE_SPECIFIC_MESSAGE_ID_MASK) {
      EVSSendDbg(SBN_PEERTASK_EID, "SBN received module-specific message type: 0x%08x", MsgType);
      return SBN_SUCCESS;
    }

    switch (MsgType)
    {
        case SBN_PROTO_MSG:
        {
            uint8 Ver = ((uint8 *)Msg)[0];
            if (Ver != SBN_PROTO_VER)
            {
                EVSSendErr(SBN_SB_EID,
                           "%s SBN protocol version mismatch with peer %d:%d, "
                           "my version=%d, peer version %d",
                           FAIL_PREFIX,
                           Peer->SpacecraftID,
                           (int)Peer->ProcessorID, (int)SBN_PROTO_VER, (int)Ver);
            }
            else
            {
                EVSSendInfo(SBN_SB_EID, "SBN protocol version match with peer %d:%d", (int)Peer->SpacecraftID, (int)Peer->ProcessorID);
            } /* end if */
            break;
        } /* end case */
        case SBN_APP_MSG:
        {
            SBN_ModuleIdx_t  FilterIdx = 0;
            SBN_Filter_Ctx_t Filter_Context;

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
            }     /* end for */

            CFE_Status = CFE_SB_TransmitMsg(Msg, false);

            if (CFE_Status != CFE_SUCCESS)
            {
                EVSSendErr(SBN_SB_EID, "%s CFE_SB_PassMsg error (Status=%d MsgType=0x%x)", FAIL_PREFIX, (int)CFE_Status, MsgType);
                return SBN_ERROR;
            } /* end if */
            break;
        } /* end case */
        case SBN_SUB_MSG:
            return SBN_ProcessSubsFromPeer(Peer, Msg);

        case SBN_UNSUB_MSG:
            return SBN_ProcessUnsubsFromPeer(Peer, Msg);

        case SBN_NO_MSG:
            return SBN_SUCCESS;
        default:
            /* Should I generate an event? Probably... */
            EVSSendErr(SBN_PEERTASK_EID, "%s unknown message type 0x%08x", FAIL_PREFIX, MsgType);
            return SBN_ERROR;
    } /* end switch */

    return SBN_SUCCESS;
} /* end SBN_ProcessNetMsg */

/**
 * Find the PeerIndex for a given ProcessorID and net.
 * @param Net[in] The network interface to search.
 * @param ProcessorID[in] The ProcessorID of the peer being sought.
 * @param SpacecraftID[in] The SpacecraftI of the peer being sought.
 * @return The Peer interface pointer, or NULL if not found.
 */
SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID)
{
    SBN_PeerIdx_t PeerIdx = 0;

    for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        if (Net->Peers[PeerIdx].ProcessorID == ProcessorID && Net->Peers[PeerIdx].SpacecraftID == SpacecraftID)
        {
            return &Net->Peers[PeerIdx];
        } /* end if */
    }     /* end for */

    return NULL;
} /* end SBN_GetPeer */


/** \brief Reload configuration and re-initialize
 * This is only called after receiving a message from TBL service.
 */
SBN_Status_t SBN_ReloadConfTbl(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not reload SBN configuration:";

    SBN_Status_t Status;

    EVSSendInfo(SBN_TBL_EID, "re-initializing SBN with new configuration...");

    if (OS_MutSemTake(SBN.ConfMutex) != OS_SUCCESS)
    {
      EVSSendErr(SBN_PEER_EID, "%s could not take configuration mutex", FAIL_PREFIX);
      return SBN_ERROR;
    } /* end if */

    Status = Cleanup();

    if(Status != SBN_SUCCESS) {
      EVSSendErr(SBN_PEER_EID,  "%s could not clean up SBN", FAIL_PREFIX);
    } else {
      Status = Init();

      EVSSendInfo(SBN_TBL_EID, "SBN re-initialized.");
    }

    if (OS_MutSemGive(SBN.ConfMutex) != OS_SUCCESS)
    {
      EVSSendErr(SBN_PEER_EID, "%s could not give configuration mutex", FAIL_PREFIX);
      return SBN_ERROR;
    } /* end if */

    return Status;
} /* end SBN_ReloadConfTbl() */
