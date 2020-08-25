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
#include "cfe_sb_events.h" /* For event message IDs */
#include "cfe_es.h" /* PerfLog */
#include "cfe_platform_cfg.h"
#include "cfe_msgids.h"
#include "cfe_version.h"

/** \brief SBN global application data, indexed by AppID. */
SBN_App_t SBN;

#include <string.h>
#include "sbn_app.h"

static SBN_Status_t UnloadModules(void)
{
    SBN_ModuleIdx_t i = 0;

    for(i = 0; i < SBN_MAX_MOD_CNT; i++)
    {
        if (SBN.ProtocolModules[i] == 0)
        {
            continue; /* this module may have been loaded by ES, so continue in case there are any I loaded. */
        }/* end if */

        if(OS_ModuleUnload(SBN.ProtocolModules[i]) != OS_SUCCESS)
        {
            EVSSendCrit(SBN_TBL_EID, "unable to unload protocol module ID %d", i);
            return SBN_ERROR;
        }/* end if */
    }/* end for */

    for(i = 0; i < SBN_MAX_MOD_CNT; i++)
    {
        if (SBN.FilterModules[i] == 0)
        {
            continue; /* this module may have been loaded by ES, so continue in case there are any I loaded. */
        }/* end if */

        if(OS_ModuleUnload(SBN.FilterModules[i]) != OS_SUCCESS)
        {
            EVSSendCrit(SBN_TBL_EID, "unable to unload filter module ID %d", i);
            return SBN_ERROR;
        }/* end if */
    }/* end for */

    return SBN_SUCCESS;
}/* end UnloadModules() */

/**
 * \brief Packs a CCSDS message with an SBN message header.
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in network (big-endian) byte order.
 * \param SBNBuf[out] The buffer to pack into.
 * \param MsgSz[in] The size of the payload.
 * \param MsgType[in] The SBN message type.
 * \param ProcessorID[in] The ProcessorID of the sender (should be CFE_CPU_ID)
 * \param Msg[in] The payload (CCSDS message or SBN sub/unsub.)
 */
void SBN_PackMsg(void *SBNBuf, SBN_MsgSz_t MsgSz, SBN_MsgType_t MsgType,
    CFE_ProcessorID_t ProcessorID, void *Msg)
{
    Pack_t Pack;
    Pack_Init(&Pack, SBNBuf, MsgSz + SBN_PACKED_HDR_SZ, false);

    Pack_Int16(&Pack, MsgSz);
    Pack_UInt8(&Pack, MsgType);
    Pack_UInt32(&Pack, ProcessorID);

    if(!Msg || !MsgSz)
    {
        /* valid to have a NULL pointer/empty size payload */
        return;
    }/* end if */

    Pack_Data(&Pack, Msg, MsgSz);
}/* end SBN_PackMsg */

/**
 * \brief Unpacks a CCSDS message with an SBN message header.
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
    CFE_ProcessorID_t *ProcessorIDPtr, void *Msg)
{
    uint8 t = 0;
    Pack_t Pack;
    Pack_Init(&Pack, SBNBuf, SBN_MAX_PACKED_MSG_SZ, false);
    Unpack_Int16(&Pack, MsgSzPtr);
    Unpack_UInt8(&Pack, &t);
    *MsgTypePtr = t;
    Unpack_UInt32(&Pack, ProcessorIDPtr);

    if(!*MsgSzPtr)
    {
        return true;
    }/* end if */

    if(*MsgSzPtr < 0 || *MsgSzPtr > CFE_MISSION_SB_MAX_SB_MSG_SIZE)
    {
        return false;
    }/* end if */

    Unpack_Data(&Pack, Msg, *MsgSzPtr);

    return true;
}/* end SBN_UnpackMsg */

/* Use a struct for all local variables in the task so we can specify exactly
 * how large of a stack we need for the task.
 */

typedef struct
{
    SBN_Status_t Status;
    OS_TaskID_t RecvTaskID;
    SBN_PeerIdx_t PeerIdx;
    SBN_NetIdx_t NetIdx;
    SBN_PeerInterface_t *Peer;
    SBN_NetInterface_t *Net;
    CFE_ProcessorID_t ProcessorID;
    SBN_MsgType_t MsgType;
    SBN_MsgSz_t MsgSz;
    uint8 Msg[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
} RecvPeerTaskData_t;

void SBN_RecvPeerTask(void)
{
    RecvPeerTaskData_t D;
    memset(&D, 0, sizeof(D));
    if(CFE_ES_RegisterChildTask() != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unable to register child task");
        return;
    }/* end if */

    D.RecvTaskID = OS_TaskGetId();

    for(D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        if(!D.Net->Configured)
        {
            continue;
        }

        for(D.PeerIdx = 0; D.PeerIdx < D.Net->PeerCnt; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if(D.Peer->RecvTaskID == D.RecvTaskID)
            {
                break;
            }/* end if */
        }/* end for */

        if(D.PeerIdx < D.Net->PeerCnt)
        {
            break;
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.NetCnt)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unable to connect task to peer struct");
        return;
    }/* end if */

    while(1)
    {
        D.Status = D.Net->IfOps->RecvFromPeer(D.Net, D.Peer,
            &D.MsgType, &D.MsgSz, &D.ProcessorID, &D.Msg);

        if(D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }/* end if */

        if(D.Status == SBN_SUCCESS)
        {
            OS_GetLocalTime(&D.Peer->LastRecv);

            D.Status = SBN_ProcessNetMsg(D.Net, D.MsgType, D.ProcessorID, D.MsgSz, &D.Msg);

            if (D.Status != SBN_SUCCESS)
            {
                D.Peer->RecvTaskID = 0;
                return;
            }/* end if */
        }
        else
        {
            EVSSendErr(SBN_PEER_EID, "recv error (%d)", D.Status);
            D.Peer->RecvErrCnt++;
            D.Peer->RecvTaskID = 0;
            return;
        }/* end if */
    }/* end while */
}/* end SBN_RecvPeerTask() */

typedef struct
{
    SBN_NetIdx_t NetIdx;
    SBN_NetInterface_t *Net;
    SBN_PeerInterface_t *Peer;
    SBN_Status_t Status;
    OS_TaskID_t RecvTaskID;
    CFE_ProcessorID_t ProcessorID;
    SBN_MsgType_t MsgType;
    SBN_MsgSz_t MsgSz;
    uint8 Msg[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
} RecvNetTaskData_t;

void SBN_RecvNetTask(void)
{
    RecvNetTaskData_t D;
    memset(&D, 0, sizeof(D));
    if(CFE_ES_RegisterChildTask() != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unable to register child task");
        return;
    }/* end if */

    D.RecvTaskID = OS_TaskGetId();

    for(D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        if(D.Net->RecvTaskID == D.RecvTaskID)
        {
            break;
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.NetCnt)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unable to connect task to net struct");
        return;
    }/* end if */

    while(1)
    {
        SBN_Status_t Status = SBN_SUCCESS;

        Status = D.Net->IfOps->RecvFromNet(D.Net, &D.MsgType,
            &D.MsgSz, &D.ProcessorID, &D.Msg);

        if(Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }/* end if */

        if(Status != SBN_SUCCESS)
        {
            return;
        }/* end if */

        D.Peer = SBN_GetPeer(D.Net, D.ProcessorID);
        if(!D.Peer)
        {
            EVSSendErr(SBN_PEERTASK_EID, "unknown peer (ProcessorID=%d)", D.ProcessorID);
            return;
        }/* end if */

        OS_GetLocalTime(&D.Peer->LastRecv);

        D.Status = SBN_ProcessNetMsg(D.Net, D.MsgType, D.ProcessorID, D.MsgSz, &D.Msg);

        if (D.Status != SBN_SUCCESS)
        {
            return;
        }/* end if */
    }/* end while */
}/* end SBN_RecvNetTask() */

/**
 * Checks all interfaces for messages from peers.
 * Receive messages from the specified peer, injecting them onto the local
 * software bus.
 */
SBN_Status_t SBN_RecvNetMsgs(void)
{
    SBN_Status_t SBN_Status = 0;
    uint8 Msg[CFE_MISSION_SB_MAX_SB_MSG_SIZE];

    SBN_NetIdx_t NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        SBN_MsgType_t MsgType;
        SBN_MsgSz_t MsgSz;
        CFE_ProcessorID_t ProcessorID;

        if(Net->TaskFlags & SBN_TASK_RECV)
        {
            continue; /* separate task handles receiving from a net */
        }/* end if */

        if(Net->IfOps->RecvFromNet)
        {
            int MsgCnt = 0;
            // TODO: make configurable
            for(MsgCnt = 0; MsgCnt < 100; MsgCnt++) /* read at most 100 messages from the net */
            {
                memset(Msg, 0, sizeof(Msg));

                SBN_Status = Net->IfOps->RecvFromNet(Net, &MsgType, &MsgSz, &ProcessorID, Msg);

                if(SBN_Status == SBN_IF_EMPTY)
                {
                    break; /* no (more) messages for this net, continue to next net */
                }/* end if */

                /* for UDP, the message received may not be from the peer
                 * expected.
                 */
                SBN_PeerInterface_t *Peer = SBN_GetPeer(Net, ProcessorID);

                if (!Peer)
                {
                    EVSSendInfo(SBN_PEERTASK_EID, "unknown peer (ProcessorID=%d)", ProcessorID);
                    /* may be a misconfiguration on my part...? continue processing msgs... */
                    continue;
                }/* end if */

                OS_GetLocalTime(&Peer->LastRecv);
                SBN_ProcessNetMsg(Net, MsgType, ProcessorID, MsgSz, Msg); /* ignore errors */
            }/* end for */
        }
        else if(Net->IfOps->RecvFromPeer)
        {
            SBN_PeerIdx_t PeerIdx = 0;
            for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

                int MsgCnt = 0;
                // TODO: make configurable
                for(MsgCnt = 0; MsgCnt < 100; MsgCnt++) /* read at most 100 messages from peer */
                {
                    CFE_ProcessorID_t ProcessorID = 0;
                    SBN_MsgType_t MsgType = 0;
                    SBN_MsgSz_t MsgSz = 0;

                    memset(Msg, 0, sizeof(Msg));

                    SBN_Status = Net->IfOps->RecvFromPeer(Net, Peer,
                        &MsgType, &MsgSz, &ProcessorID, Msg);

                    if(SBN_Status == SBN_IF_EMPTY)
                    {
                        break; /* no (more) messages for this peer, continue to next peer */
                    }/* end if */

                    OS_GetLocalTime(&Peer->LastRecv);

                    SBN_Status = SBN_ProcessNetMsg(Net, MsgType, ProcessorID, MsgSz, Msg);

                    if(SBN_Status != SBN_SUCCESS)
                    {
                        break; /* continue to next peer */
                    }/* end if */
                }/* end for */
            }/* end for */
        }
        else
        {
            EVSSendErr(SBN_PEER_EID, "neither RecvFromPeer nor RecvFromNet defined for net #%d", NetIdx);

            /* meanwhile, continue to next net... */
        }/* end if */
    }/* end for */

    return SBN_SUCCESS;
}/* end SBN_RecvNetMsgs */

/**
 * Sends a message to a peer using the module's SendNetMsg.
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
    SBN_NetInterface_t *Net = Peer->Net;
    SBN_Status_t SBN_Status = SBN_SUCCESS;

    if(Peer->SendTaskID)
    {
        if(OS_MutSemTake(SBN.SendMutex) != OS_SUCCESS)
        {
            EVSSendErr(SBN_PEER_EID, "unable to take mutex");
            return SBN_ERROR;
        }/* end if */
    }/* end if */

    SBN_Status = Net->IfOps->Send(Peer, MsgType, MsgSz, Msg);

    if(SBN_Status != SBN_SUCCESS)
    {
        Peer->SendErrCnt++;

        return SBN_Status;
    }/* end if */

    /* for clients that need a poll or heartbeat, update time even when failing */
    OS_GetLocalTime(&Peer->LastSend);

    Peer->SendCnt++;

    if(Peer->SendTaskID)
    {
        if(OS_MutSemGive(SBN.SendMutex) != OS_SUCCESS)
        {
            EVSSendErr(SBN_PEER_EID, "unable to give mutex");
            return SBN_ERROR;
        }/* end if */
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_SendNetMsg */

typedef struct
{
    SBN_Status_t Status;
    SBN_NetIdx_t NetIdx;
    SBN_PeerIdx_t PeerIdx;
    OS_TaskID_t SendTaskID;
    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_MsgId_t MsgID;
    SBN_NetInterface_t *Net;
    SBN_PeerInterface_t *Peer;
} SendTaskData_t;

/**
 * \brief When a peer is connected, a task is created to listen to the relevant
 * pipe for messages to send to that peer.
 */
void SBN_SendTask(void)
{
    SendTaskData_t D;
    SBN_Filter_Ctx_t Filter_Context;

    Filter_Context.MyProcessorID = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID = CFE_PSP_GetSpacecraftId();

    memset(&D, 0, sizeof(D));

    if(CFE_ES_RegisterChildTask() != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unable to register child task");
        return;
    }/* end if */

    D.SendTaskID = OS_TaskGetId();

    for(D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        for(D.PeerIdx = 0; D.PeerIdx < D.Net->PeerCnt; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if(D.Peer->SendTaskID == D.SendTaskID)
            {
                break;
            }/* end if */
        }/* end for */

        if(D.PeerIdx < D.Net->PeerCnt)
        {
            break; /* found a ringer */
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.NetCnt)
    {
        EVSSendErr(SBN_PEER_EID, "error connecting send task");
        return;
    }/* end if */

    while(1)
    {
        SBN_ModuleIdx_t FilterIdx = 0;

        if(!D.Peer->Connected)
        {
            OS_TaskDelay(SBN_MAIN_LOOP_DELAY);
            continue;
        }/* end if */

        if(CFE_SB_RcvMsg(&D.SBMsgPtr, D.Peer->Pipe, CFE_SB_PEND_FOREVER)
            != CFE_SUCCESS)
        {
            break;
        }/* end if */

        Filter_Context.PeerProcessorID = D.Peer->ProcessorID;
        Filter_Context.PeerSpacecraftID = D.Peer->SpacecraftID;

        for(FilterIdx = 0; FilterIdx < D.Peer->FilterCnt; FilterIdx++)
        {
            if(D.Peer->Filters[FilterIdx]->FilterSend == NULL)
            {
                continue;
            }/* end if */
            
            D.Status = (D.Peer->Filters[FilterIdx]->FilterSend)(D.SBMsgPtr, &Filter_Context);
            if (D.Status == SBN_IF_EMPTY) /* filter requests not sending this msg, see below for loop */
            {
                break;
            }/* end if */

            if(D.Status != SBN_SUCCESS)
            {
                /* mark peer as not having a task so that sending will create a new one */
                D.Peer->SendTaskID = 0;
                return;
            }/* end if */
        }/* end for */

        if(FilterIdx < D.Peer->FilterCnt)
        {
            /* one of the above filters suggested rejecting this message */
            continue;
        }/* end if */

        D.Status = SBN_SendNetMsg(SBN_APP_MSG, CFE_SB_GetTotalMsgLength(D.SBMsgPtr), D.SBMsgPtr, D.Peer);

        if (D.Status == SBN_ERROR)
        {
            /* mark peer as not having a task so that sending will create a new one */
            D.Peer->SendTaskID = 0;
            return;
        }/* end if */
    }/* end while */
}/* end SBN_SendTask() */

/**
 * Iterate through all peers, examining the pipe to see if there are messages
 * I need to send to that peer.
 */
static SBN_Status_t CheckPeerPipes(void)
{
    CFE_Status_t CFE_Status;
    int ReceivedFlag = 0, iter = 0;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    SBN_Filter_Ctx_t Filter_Context;

    Filter_Context.MyProcessorID = CFE_PSP_GetProcessorId();
    Filter_Context.MySpacecraftID = CFE_PSP_GetSpacecraftId();

    /**
     * \note This processes one message per peer, then start again until no
     * peers have pending messages. At max only process SBN_MAX_MSG_PER_WAKEUP
     * per peer per wakeup otherwise I will starve other processing.
     */
    for(iter = 0; iter < SBN_MAX_MSG_PER_WAKEUP; iter++)
    {
        ReceivedFlag = 0;

        SBN_NetIdx_t NetIdx = 0;
        for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
        {
            SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

            SBN_PeerIdx_t PeerIdx = 0;
            for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                SBN_ModuleIdx_t FilterIdx = 0;
                SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

                if(Peer->Connected == 0)
                {
                    continue;
                }/* end if */

                if(Peer->TaskFlags & SBN_TASK_SEND)
                {
                    if (!Peer->SendTaskID)
                    {
                        /* TODO: logic/controls to prevent hammering? */
                        char SendTaskName[32];

                        snprintf(SendTaskName, 32, "sendT_%d_%d", NetIdx,
                            Peer->ProcessorID);
                        CFE_Status = CFE_ES_CreateChildTask(&(Peer->SendTaskID),
                            SendTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_SendTask, NULL,
                            CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(SendTaskData_t), 0, 0);

                        if(CFE_Status != CFE_SUCCESS)
                        {
                            EVSSendErr(SBN_PEER_EID, "error creating send task for %d", Peer->ProcessorID);
                            return SBN_ERROR;
                        }/* end if */
                    }/* end if */

                    continue;
                }/* end if */

                /* if peer data is not in use, go to next peer */
                if(CFE_SB_RcvMsg(&SBMsgPtr, Peer->Pipe, CFE_SB_POLL) != CFE_SUCCESS)
                {
                    continue;
                }/* end if */

                ReceivedFlag = 1;

                Filter_Context.PeerProcessorID = Peer->ProcessorID;
                Filter_Context.PeerSpacecraftID = Peer->SpacecraftID;

                for(FilterIdx = 0; FilterIdx < Peer->FilterCnt; FilterIdx++)
                {
                    SBN_Status_t SBN_Status;
                    
                    if(Peer->Filters[FilterIdx]->FilterSend == NULL)
                    {
                        continue;
                    }/* end if */
                    
                    SBN_Status = (Peer->Filters[FilterIdx]->FilterSend)(SBMsgPtr, &Filter_Context);

                    if (SBN_Status == SBN_IF_EMPTY) /* filter requests not sending this msg, see below for loop */
                    {
                        break;
                    }/* end if */

                    if(SBN_Status != SBN_SUCCESS)
                    {
                        /* something fatal happened, exit */
                        return SBN_Status;
                    }/* end if */
                }/* end for */

                if(FilterIdx < Peer->FilterCnt)
                {
                    /* one of the above filters suggested rejecting this message */
                    continue;
                }/* end if */

                SBN_SendNetMsg(SBN_APP_MSG,
                    CFE_SB_GetTotalMsgLength(SBMsgPtr),
                    SBMsgPtr, Peer);
            }/* end for */
        }/* end for */

        if(!ReceivedFlag)
        {
            break;
        }/* end if */
    } /* end for */
    return SBN_SUCCESS;
}/* end CheckPeerPipes */

/**
 * Iterate through all peers, calling the poll interface if no messages have
 * been sent in the last SBN_POLL_TIME seconds.
 */
static SBN_Status_t PeerPoll(void)
{
    CFE_Status_t CFE_Status;
    SBN_NetIdx_t NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

        if(Net->IfOps->RecvFromNet && Net->TaskFlags & SBN_TASK_RECV)
        {
            if(!Net->RecvTaskID)
            {
                /* TODO: add logic/controls to prevent hammering */
                char RecvTaskName[32];
                snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_recvs_%d", NetIdx);
                CFE_Status = CFE_ES_CreateChildTask(&(Net->RecvTaskID),
                    RecvTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_RecvNetTask,
                    NULL, CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(RecvNetTaskData_t),
                    0, 0);

                if(CFE_Status != CFE_SUCCESS)
                {
                    EVSSendErr(SBN_PEER_EID, "error creating task for net %d", NetIdx);
                    return SBN_ERROR;
                }/* end if */
            }/* end if */
        }
        else
        {
            SBN_PeerIdx_t PeerIdx = 0;
            for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

                if(Net->IfOps->RecvFromPeer && Peer->TaskFlags & SBN_TASK_RECV)
                {
                    if(!Peer->RecvTaskID)
                    {
                        /* TODO: add logic/controls to prevent hammering */
                        char RecvTaskName[32];
                        snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_recv_%d", PeerIdx);
                        CFE_Status = CFE_ES_CreateChildTask(&(Peer->RecvTaskID),
                            RecvTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SBN_RecvPeerTask,
                            NULL,
                            CFE_PLATFORM_ES_DEFAULT_STACK_SIZE + 2 * sizeof(RecvPeerTaskData_t),
                            0, 0);
                        /* TODO: more accurate stack size required */

                        if(CFE_Status != CFE_SUCCESS)
                        {
                            EVSSendErr(SBN_PEER_EID, "error creating task for %d", Peer->ProcessorID);
                            return SBN_ERROR;
                        }/* end if */
                    }/* end if */
                }
                else
                {
                    Net->IfOps->PollPeer(Peer);
                }/* end if */
            }/* end for */
        }/* end if */
    }/* end for */

    return SBN_SUCCESS;
}/* end PeerPoll */

/**
 * Loops through all hosts and peers, initializing all.
 *
 * @return SBN_SUCCESS if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
static SBN_Status_t InitInterfaces(void)
{
    if(SBN.NetCnt < 1)
    {
        EVSSendErr(SBN_PEER_EID, "no networks configured");

        return SBN_ERROR;
    }/* end if */

    SBN_NetIdx_t NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

        if(!Net->Configured)
        {
            EVSSendErr(SBN_PEER_EID, "network #%d not configured", NetIdx);

            return SBN_ERROR;
        }/* end if */

        Net->IfOps->InitNet(Net);

        SBN_PeerIdx_t PeerIdx = 0;
        for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            Net->IfOps->InitPeer(Peer);
        }/* end for */
    }/* end for */

    EVSSendInfo(SBN_INIT_EID, "configured, %d nets", SBN.NetCnt);

    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        EVSSendInfo(SBN_INIT_EID, "net %d has %d peers", NetIdx, SBN.Nets[NetIdx].PeerCnt);
    }/* end for */

    return SBN_SUCCESS;
}/* end InitInterfaces */

/**
 * This function waits for the scheduler (SCH) to wake this code up, so that
 * nothing transpires until the cFE is fully operational.
 *
 * @param[in] iTimeOut The time to wait for the scheduler to notify this code.
 * @return CFE_SUCCESS on success, otherwise an error value.
 */
static SBN_Status_t WaitForWakeup(int32 iTimeOut)
{
    CFE_Status_t CFE_Status = CFE_SUCCESS;
    CFE_SB_MsgPtr_t Msg = 0;

    /* Wait for WakeUp messages from scheduler */
    CFE_Status = CFE_SB_RcvMsg(&Msg, SBN.CmdPipe, iTimeOut);

    switch(CFE_Status)
    {
        case CFE_SB_NO_MESSAGE:
        case CFE_SB_TIME_OUT:
            break;
        case CFE_SUCCESS:
            SBN_HandleCommand(Msg);
            break;
        default:
            return SBN_ERROR;
    }/* end switch */

    /* For sbn, we still want to perform cyclic processing
    ** if the WaitForWakeup time out
    ** cyclic processing at timeout rate
    */
    CFE_ES_PerfLogEntry(SBN_PERF_RECV_ID);

    SBN_RecvNetMsgs();

    SBN_CheckSubscriptionPipe();

    CheckPeerPipes();

    PeerPoll();

    CFE_ES_PerfLogExit(SBN_PERF_RECV_ID);

    return SBN_SUCCESS;
}/* end WaitForWakeup */

/**
 * Waits for either a response to the "get subscriptions" message from SB, OR
 * an event message that says SB has finished initializing. The latter message
 * means that SB was not started at the time SBN sent the "get subscriptions"
 * message, so that message will need to be sent again.
 *
 * @return SBN_SUCCESS or SBN_ERROR
 */
static SBN_Status_t WaitForSBStartup(void)
{
    CFE_EVS_LongEventTlm_t *EvsTlm = NULL;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    uint8 counter = 0;
    CFE_SB_PipeId_t EventPipe = 0;
    CFE_Status_t Status = SBN_SUCCESS;

    /* Create event message pipe */
    Status = CFE_SB_CreatePipe(&EventPipe, 20, "SBNEventPipe");
    if(Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to create event pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    /* Subscribe to event messages temporarily to be notified when SB is done
     * initializing
     */
    Status = CFE_SB_Subscribe(CFE_EVS_LONG_EVENT_MSG_MID, EventPipe);
    if(Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to event pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    while(1)
    {
        /* Check for subscription message from SB */
        if(SBN_CheckSubscriptionPipe() == SBN_SUCCESS)
        {
            /* SBN does not need to re-send request messages to SB */
            break;
        }
        else if(counter % 100 == 0)
        {
            /* Send subscription request messages again. This may cause the SB
             * to respond to duplicate requests but that should be okay
             */
            SBN_SendSubsRequests();
        }/* end if */

        /* Check for event message from SB */
        if(CFE_SB_RcvMsg(&SBMsgPtr, EventPipe, 100) == CFE_SUCCESS)
        {
            if(CFE_SB_GetMsgId(SBMsgPtr) == CFE_EVS_LONG_EVENT_MSG_MID)
            {
                EvsTlm = (CFE_EVS_LongEventTlm_t *)SBMsgPtr;

                /* If it's an event message from SB, make sure it's the init
                 * message
                 */
                if(strcmp(EvsTlm->Payload.PacketID.AppName, "CFE_SB") == 0
                    && EvsTlm->Payload.PacketID.EventID == CFE_SB_INIT_EID)
                {
                    break;
                }/* end if */
            }/* end if */
        }/* end if */

        counter++;
    }/* end while */

    /* Unsubscribe from event messages */
    if(CFE_SB_Unsubscribe(CFE_EVS_LONG_EVENT_MSG_MID, EventPipe) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "unable to unsubscribe from event messages");
        return SBN_ERROR;
    }/* end if */

    if(CFE_SB_DeletePipe(EventPipe) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "unable to delete event pipe");
        return SBN_ERROR;
    }/* end if */

    /* SBN needs to re-send request messages */
    return SBN_SUCCESS;
}/* end WaitForSBStartup */

static cpuaddr LoadConf_Module(SBN_Module_Entry_t *e, CFE_ES_ModuleID_t *ModuleIDPtr)
{
    cpuaddr StructAddr;

    EVSSendInfo(SBN_TBL_EID, "linking symbol (%s)", e->LibSymbol);
    if(OS_SymbolLookup(&StructAddr, e->LibSymbol) != OS_SUCCESS) /* try loading it if it's not already loaded */
    {
        if(e->LibFileName[0] == '\0')
        {
            EVSSendErr(SBN_TBL_EID, "invalid module (Name=%s)", e->Name);
            return 0;
        }

        EVSSendInfo(SBN_TBL_EID, "loading module (Name=%s, File=%s)", e->Name, e->LibFileName);
        if(OS_ModuleLoad(ModuleIDPtr, e->Name, e->LibFileName) != OS_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "invalid module file (Name=%s LibFileName=%s)", e->Name,
                e->LibFileName);
            return 0;
        }/* end if */

        EVSSendInfo(SBN_TBL_EID, "trying symbol again (%s)", e->LibSymbol);
        if(OS_SymbolLookup(&StructAddr, e->LibSymbol) != OS_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "invalid symbol (Name=%s LibSymbol=%s)", e->Name,
                e->LibSymbol);
            return 0;
        }
    }/* end if */

    return StructAddr;
}/* end LoadConf_Module */

/**
 * Load the filters from the table.
 * @param FilterModules[in] - The filter module entries in the table.
 * @param FilterModuleCnt[in] - The number of entries in FilterModules.
 * @param ModuleNames[in] - The array of filters this peer/net wishes to use.
 * @param FilterFns[out] - The function pointers for the filters requested.
 * @return The number of entries in FilterFns.
 */
static SBN_ModuleIdx_t LoadConf_Filters(SBN_Module_Entry_t *FilterModules,
    SBN_ModuleIdx_t FilterModuleCnt, SBN_FilterInterface_t **ConfFilters,
    char ModuleNames[SBN_MAX_FILTERS_PER_PEER][SBN_MAX_MOD_NAME_LEN],
    SBN_FilterInterface_t **Filters)
{
    int i = 0;
    SBN_ModuleIdx_t FilterCnt = 0;

    memset(FilterModules, 0, sizeof(*FilterModules) * FilterCnt);

    for (i = 0; *ModuleNames[i] && i < SBN_MAX_FILTERS_PER_PEER; i++)
    {
        SBN_ModuleIdx_t FilterIdx = 0;
        for (FilterIdx = 0; FilterIdx < FilterModuleCnt; FilterIdx++)
        {
            if(strcmp(ModuleNames[i], FilterModules[FilterIdx].Name) == 0)
            {
                break;
            }/* end if */
        }/* end for */

        if(FilterIdx == FilterModuleCnt)
        {
            EVSSendErr(SBN_TBL_EID, "Invalid filter name: %s", ModuleNames[i]);
            continue;
        }/* end if */

        Filters[FilterCnt++] = ConfFilters[FilterIdx];
    }/* end for */

    return FilterCnt;
}/* end LoadConf_Filters() */

static SBN_Status_t LoadConf(void)
{
    SBN_ConfTbl_t *TblPtr = NULL;
    SBN_ModuleIdx_t ModuleIdx = 0;
    SBN_PeerIdx_t PeerIdx = 0;
    SBN_FilterInterface_t *Filters[SBN_MAX_MOD_CNT];

    memset(Filters, 0, sizeof(Filters));

    if(CFE_TBL_GetAddress((void **)&TblPtr, SBN.ConfTblHandle) != CFE_TBL_INFO_UPDATED)
    {
        EVSSendErr(SBN_TBL_EID, "unable to get conf table address");
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return SBN_ERROR;
    }/* end if */

    /* load protocol modules */
    for(ModuleIdx = 0; ModuleIdx < TblPtr->ProtocolCnt; ModuleIdx++)
    {
        CFE_ES_ModuleID_t ModuleID = 0;

        SBN_IfOps_t *Ops = (SBN_IfOps_t *)LoadConf_Module(
            &TblPtr->ProtocolModules[ModuleIdx], &ModuleID);

        if (Ops == NULL)
        {
            /* LoadConf_Module already generated an event */
            return SBN_ERROR;
        }/* end if */

        EVSSendInfo(SBN_TBL_EID, "calling init fn");
        if(Ops->InitModule(SBN_PROTOCOL_VERSION, TblPtr->ProtocolModules[ModuleIdx].BaseEID) != CFE_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "error in protocol init");
            return SBN_ERROR;
        }/* end if */

        SBN.IfOps[ModuleIdx] = Ops;
        SBN.ProtocolModules[ModuleIdx] = ModuleID;
    }/* end for */

    /* load filter modules */
    for(ModuleIdx = 0; ModuleIdx < TblPtr->FilterCnt; ModuleIdx++)
    {
        CFE_ES_ModuleID_t ModuleID = 0;

        Filters[ModuleIdx] = (SBN_FilterInterface_t *)LoadConf_Module(
            &TblPtr->FilterModules[ModuleIdx], &ModuleID);

        if(Filters[ModuleIdx] == NULL)
        {
            /* LoadConf_Module already generated an event */
            return SBN_ERROR;
        }/* end if */

        if(Filters[ModuleIdx]->InitModule(SBN_FILTER_VERSION, TblPtr->FilterModules[ModuleIdx].BaseEID) != CFE_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID, "error in filter init");
            return SBN_ERROR;
        }/* end if */

        SBN.FilterModules[ModuleIdx] = ModuleID;
    }/* end for */

    /* load nets and peers */
    for(PeerIdx = 0; PeerIdx < TblPtr->PeerCnt; PeerIdx++)
    {
        SBN_Peer_Entry_t *e = &TblPtr->Peers[PeerIdx];

        for(ModuleIdx = 0; ModuleIdx < TblPtr->ProtocolCnt; ModuleIdx++)
        {
            if(strcmp(TblPtr->ProtocolModules[ModuleIdx].Name, e->ProtocolName) == 0)
            {
                break;
            }
        }
        
        if(ModuleIdx == TblPtr->ProtocolCnt)
        {
            EVSSendCrit(SBN_TBL_EID, "invalid module name %s", e->ProtocolName);
            return SBN_ERROR;
        }/* end if */

        if(e->NetNum < 0 || e->NetNum >= SBN_MAX_NETS)
        {
            EVSSendCrit(SBN_TBL_EID, "too many networks");
            return SBN_ERROR;
        }/* end if */

        if(e->NetNum + 1 > SBN.NetCnt)
        {
            SBN.NetCnt = e->NetNum + 1;
        }/* end if */

        SBN_NetInterface_t *Net = &SBN.Nets[e->NetNum];
        if(e->ProcessorID == CFE_PSP_GetProcessorId() && e->SpacecraftID == CFE_PSP_GetSpacecraftId())
        {
            Net->Configured = true;
            Net->ProtocolIdx = ModuleIdx;
            Net->IfOps = SBN.IfOps[ModuleIdx];
            Net->IfOps->LoadNet(Net, (const char *)e->Address);

            Net->FilterCnt = LoadConf_Filters(TblPtr->FilterModules, TblPtr->FilterCnt,
                Filters, e->Filters, Net->Filters);

            Net->TaskFlags = e->TaskFlags;
        }
        else
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[Net->PeerCnt++];
            memset(Peer, 0, sizeof(*Peer));
            Peer->Net = Net;
            Peer->ProcessorID = e->ProcessorID;
            Peer->SpacecraftID = e->SpacecraftID;

            Peer->FilterCnt = LoadConf_Filters(TblPtr->FilterModules, TblPtr->FilterCnt,
                Filters, e->Filters, Peer->Filters);

            SBN.IfOps[ModuleIdx]->LoadPeer(Peer, (const char *)e->Address);

            Peer->TaskFlags = e->TaskFlags;
        }/* end if */
    }/* end for */

    /* address only needed at load time, release */
    if(CFE_TBL_ReleaseAddress(SBN.ConfTblHandle) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to release address of conf tbl");
        return SBN_ERROR;
    }/* end if */

    /* ...but we keep the handle so we can be notified of updates */
    return SBN_SUCCESS;
}/* end LoadConf() */

static uint32 UnloadConf(void)
{
    uint32 Status;

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        if((Status = Net->IfOps->UnloadNet(Net)) != SBN_SUCCESS)
        {
            EVSSendCrit(SBN_TBL_EID, "unable to unload network %d", NetIdx);
            return Status;
        }/* end if */
    }/* end for */

    return UnloadModules();
}/* end UnloadConf() */

static uint32 LoadConfTbl(void)
{
    int32 Status = CFE_SUCCESS;

    if((Status = CFE_TBL_Register(&SBN.ConfTblHandle, "SBN_ConfTbl",
        sizeof(SBN_ConfTbl_t), CFE_TBL_OPT_DEFAULT, NULL))
        != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to register conf tbl handle");
        return Status;
    }/* end if */

    if((Status = CFE_TBL_Load(SBN.ConfTblHandle, CFE_TBL_SRC_FILE,
            SBN_CONF_TBL_FILENAME))
        != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to load conf tbl %s", SBN_CONF_TBL_FILENAME);
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return Status;
    }/* end if */

    if((Status = CFE_TBL_Manage(SBN.ConfTblHandle)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to manage conf tbl");
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return Status;
    }/* end if */

    if((Status = CFE_TBL_NotifyByMessage(SBN.ConfTblHandle, SBN_CMD_MID,
        SBN_TBL_CC, 0)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
        CFE_TBL_Unregister(SBN.ConfTblHandle);
        return Status;
    }/* end if */

    return LoadConf();
}/* end LoadConfTbl() */

/** \brief SBN Main Routine */
void SBN_AppMain(void)
{
    CFE_ES_TaskInfo_t TaskInfo;
    uint32  Status = CFE_SUCCESS;
    uint32  RunStatus = CFE_ES_RunStatus_APP_RUN,
            AppID = 0;

    if(CFE_ES_RegisterApp() != CFE_SUCCESS) return;

    if(CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER != CFE_SUCCESS)) return;

    if(CFE_ES_GetAppID(&AppID) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_INIT_EID, "unable to get AppID");
        return;
    }

    SBN.AppID = AppID;

    /* load my TaskName so I can ignore messages I send out to SB */
    uint32 TskId = OS_TaskGetId();
    if ((Status = CFE_ES_GetTaskInfo(&TaskInfo, TskId)) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "SBN failed to get task info (%d)", Status);
        return;
    }/* end if */

    strncpy(SBN.App_FullName, (const char *)TaskInfo.TaskName, OS_MAX_API_NAME - 1);

    CFE_ES_WaitForStartupSync(10000);

    if (LoadConfTbl() != SBN_SUCCESS)
    {
        /* the LoadConfTbl() functions will generate events */
        return;
    }/* end if */

    /** Create mutex for send tasks */
    Status = OS_MutSemCreate(&(SBN.SendMutex), "sbn_send_mutex", 0);

    if(Status != OS_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "error creating mutex for send tasks");
        return;
    }

    if(InitInterfaces() == SBN_ERROR)
    {
        EVSSendErr(SBN_INIT_EID, "unable to initialize interfaces");
        return;
    }/* end if */

    /* Create pipe for subscribes and unsubscribes from SB */
    Status = CFE_SB_CreatePipe(&SBN.SubPipe, SBN_SUB_PIPE_DEPTH, "SBNSubPipe");
    if(Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to create subscription pipe (Status=%d)", (int)Status);
        return;
    }/* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ALLSUBS_TLM_MID, SBN.SubPipe,
        SBN_MAX_ALLSUBS_PKTS_ON_PIPE);
    if(Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to allsubs (Status=%d)", (int)Status);
        return;
    }/* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ONESUB_TLM_MID, SBN.SubPipe,
        SBN_MAX_ONESUB_PKTS_ON_PIPE);
    if(Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to sub (Status=%d)", (int)Status);
        return;
    }/* end if */

    /* Create pipe for HK requests and gnd commands */
    /* TODO: make configurable depth */
    Status = CFE_SB_CreatePipe(&SBN.CmdPipe, 20, "SBNCmdPipe");
    if(Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to create command pipe (%d)", (int)Status);
        return;
    }/* end if */

    Status = CFE_SB_Subscribe(SBN_CMD_MID, SBN.CmdPipe);
    if(Status == CFE_SUCCESS)
    {
        EVSSendInfo(SBN_INIT_EID, "Subscribed to command MID 0x%04X", SBN_CMD_MID);
    }
    else
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to command pipe (%d)", (int)Status);
        return;
    }/* end if */

    const char *bit_order = 
#ifdef SOFTWARE_BIG_BIT_ORDER
        "big-endian";
#else /* !SOFTWARE_BIG_BIT_ORDER */
        "little-endian";
#endif /* SOFTWARE_BIG_BIT_ORDER */

    EVSSendInfo(SBN_INIT_EID, "initialized (CFE_PLATFORM_CPU_NAME='%s' ProcessorID=%d SpacecraftId=%d %s "
        "SBN.AppID=%d...",
        CFE_PLATFORM_CPU_NAME, CFE_PSP_GetProcessorId(), CFE_PSP_GetSpacecraftId(), bit_order, (int)SBN.AppID);
    EVSSendInfo(SBN_INIT_EID, "...SBN_IDENT=%s CMD_MID=0x%04X)", SBN_IDENT, SBN_CMD_MID);

    SBN_InitializeCounters();

    /* Wait for event from SB saying it is initialized OR a response from SB
       to the above messages. true means it needs to re-send subscription
       requests */
    Status = WaitForSBStartup();
    if(Status == SBN_SUCCESS)
    {
        SBN_SendSubsRequests();
    }
    else
    {
        RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }/* end if */


    /* Loop Forever */
    while(CFE_ES_RunLoop(&RunStatus))
    {
        WaitForWakeup(SBN_MAIN_LOOP_DELAY);
    }/* end while */

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        Net->IfOps->UnloadNet(Net);
    }/* end for */

    UnloadModules();

    CFE_ES_ExitApp(RunStatus);
}/* end SBN_AppMain */

/**
 * Sends a message to a peer.
 * @param[in] MsgType The type of the message (application data, SBN protocol)
 * @param[in] ProcessorID The ProcessorID to send this message to.
 * @param[in] MsgSz The size of the message (in bytes).
 * @param[in] Msg The message contents.
 *
 * @return SBN_SUCCESS on successful processing, SBN_ERROR otherwise
 */
SBN_Status_t SBN_ProcessNetMsg(SBN_NetInterface_t *Net, SBN_MsgType_t MsgType,
    CFE_ProcessorID_t ProcessorID, SBN_MsgSz_t MsgSize, void *Msg)
{
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    CFE_Status_t CFE_Status = CFE_SUCCESS;
    SBN_PeerInterface_t *Peer = SBN_GetPeer(Net, ProcessorID);

    if(!Peer)
    {
        EVSSendErr(SBN_PEERTASK_EID, "unknown peer (ProcessorID=%d)", ProcessorID);
        return SBN_ERROR;
    }/* end if */

    switch(MsgType)
    {
    	case SBN_PROTO_MSG:
        {
            uint8 Ver = ((uint8 *)Msg)[0];
            if(Ver != SBN_PROTO_VER)
            {
                EVSSendErr(SBN_SB_EID, "SBN protocol version mismatch with ProcessorID %d, "
                        "my version=%d, peer version %d",
                    (int)Peer->ProcessorID, (int)SBN_PROTO_VER, (int)Ver);
            }
            else
            {
                EVSSendInfo(SBN_SB_EID, "SBN protocol version match with ProcessorID %d", (int)Peer->ProcessorID);
            }/* end if */
            break;
        }/* end case */
        case SBN_APP_MSG:
        {
            SBN_ModuleIdx_t FilterIdx = 0;
            SBN_Filter_Ctx_t Filter_Context;

            Filter_Context.MyProcessorID = CFE_PSP_GetProcessorId();
            Filter_Context.MySpacecraftID = CFE_PSP_GetSpacecraftId();
            Filter_Context.PeerProcessorID = Peer->ProcessorID;
            Filter_Context.PeerSpacecraftID = Peer->SpacecraftID;

            for(FilterIdx = 0; FilterIdx < Peer->FilterCnt; FilterIdx++)
            {
                if(Peer->Filters[FilterIdx]->FilterRecv == NULL)
                {
                    continue;
                }/* end if */

                SBN_Status = (Peer->Filters[FilterIdx]->FilterRecv)(Msg, &Filter_Context);

                /* includes SBN_IF_EMPTY, for when filter recommends removing */
                if(SBN_Status != SBN_SUCCESS)
                {
                    return SBN_Status;
                }/* end if */
            }/* end for */

            CFE_Status = CFE_SB_PassMsg(Msg);

            if(CFE_Status != CFE_SUCCESS)
            {
                EVSSendErr(SBN_SB_EID, "CFE_SB_PassMsg error (Status=%d MsgType=0x%x)", CFE_Status, MsgType);
                return SBN_ERROR;
            }/* end if */
            break;
        }/* end case */
        case SBN_SUB_MSG:
            return SBN_ProcessSubsFromPeer(Peer, Msg);

        case SBN_UNSUB_MSG:
            return SBN_ProcessUnsubsFromPeer(Peer, Msg);

        case SBN_NO_MSG:
            return SBN_SUCCESS;
        default:
            /* Should I generate an event? Probably... */
            return SBN_ERROR;
            break;
    }/* end switch */

    return SBN_SUCCESS;
}/* end SBN_ProcessNetMsg */

/**
 * Find the PeerIndex for a given ProcessorID and net.
 * @param[in] Net The network interface to search.
 * @param[in] ProcessorID The ProcessorID of the peer being sought.
 * @return The Peer interface pointer, or NULL if not found.
 */
SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID)
{
    SBN_PeerIdx_t PeerIdx = 0;

    for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        if(Net->Peers[PeerIdx].ProcessorID == ProcessorID)
        {
            return &Net->Peers[PeerIdx];
        }/* end if */
    }/* end for */

    return NULL;
}/* end SBN_GetPeer */

SBN_Status_t SBN_Connected(SBN_PeerInterface_t *Peer)
{
    SBN_Status_t SBN_Status = SBN_SUCCESS;
    CFE_Status_t CFE_Status;

    if (Peer->Connected != 0)
    {
        EVSSendErr(SBN_PEER_EID, "CPU %d already connected", Peer->ProcessorID);
        return SBN_ERROR;
    }/* end if */

    char PipeName[OS_MAX_API_NAME];

    /* create a pipe name string similar to SBN_0_Pipe */
    snprintf(PipeName, OS_MAX_API_NAME, "SBN_%d_Pipe", Peer->ProcessorID);
    CFE_Status = CFE_SB_CreatePipe(&(Peer->Pipe), SBN_PEER_PIPE_DEPTH, PipeName);

    if(CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "failed to create pipe '%s'", PipeName);

        return SBN_ERROR;
    }/* end if */

    EVSSendInfo(SBN_PEER_EID, "pipe created '%s'", PipeName);

    CFE_Status = CFE_SB_SetPipeOpts(Peer->Pipe, CFE_SB_PIPEOPTS_IGNOREMINE);
    if(CFE_Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "failed to set pipe options '%s'", PipeName);

        return SBN_ERROR;
    }/* end if */

    EVSSendInfo(SBN_PEER_EID, "CPU %d connected", Peer->ProcessorID);

    uint8 ProtocolVer = SBN_PROTO_VER;
    SBN_Status = SBN_SendNetMsg(SBN_PROTO_MSG, sizeof(ProtocolVer), &ProtocolVer, Peer);
    if (SBN_Status != SBN_SUCCESS)
    {
        return SBN_Status;
    }/* end if */

    /* set this to current time so we don't think we've already timed out */
    OS_GetLocalTime(&Peer->LastRecv);

    SBN_Status = SBN_SendLocalSubsToPeer(Peer);

    Peer->Connected = 1;

    return SBN_Status;
} /* end SBN_Connected() */

SBN_Status_t SBN_Disconnected(SBN_PeerInterface_t *Peer)
{
    if(Peer->Connected == 0)
    {
        EVSSendErr(SBN_PEER_EID, "CPU %d not connected", Peer->ProcessorID);

        return SBN_ERROR;
    }

    Peer->Connected = 0; /**< mark as disconnected before deleting pipe */

    CFE_SB_DeletePipe(Peer->Pipe); /* ignore returned errors */
    Peer->Pipe = 0;

    Peer->SubCnt = 0; /* reset sub count, in case this is a reconnection */

    EVSSendInfo(SBN_PEER_EID, "CPU %d disconnected", Peer->ProcessorID);

    return SBN_SUCCESS;
}/* end SBN_Disconnected() */

SBN_Status_t SBN_ReloadConfTbl(void)
{
    SBN_Status_t Status;

    if((Status = UnloadConf()) != SBN_SUCCESS)
    {
        return Status;
    }/* end if */

    if(CFE_TBL_Update(SBN.ConfTblHandle) != CFE_SUCCESS)
    {
        return SBN_ERROR;
    }/* end if */

    return LoadConf();
}/* end SBN_ReloadConfTbl() */
