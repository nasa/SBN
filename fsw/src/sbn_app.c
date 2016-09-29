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
#include <string.h>

#include "cfe.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"
#include "sbn_version.h"
#include "sbn_app.h"
#include "sbn_netif.h"
#include "sbn_msgids.h"
#include "sbn_loader.h"
#include "sbn_cmds.h"
#include "sbn_subs.h"
#include "sbn_main_events.h"
#include "sbn_perfids.h"
#include "sbn_tables.h"
#include "cfe_sb_events.h" /* For event message IDs */
#include "cfe_sb_priv.h" /* For CFE_SB_SendMsgFull */
#include "cfe_es.h" /* PerfLog */

#ifndef SBN_TLM_MID
/* backwards compatability in case you're using a MID generator */
#define SBN_TLM_MID SBN_HK_TLM_MID
#endif /* SBN_TLM_MID */

/* Remap table from fields are sorted and unique, use a binary search. */
static CFE_SB_MsgId_t RemapMID(uint32 ProcessorId, CFE_SB_MsgId_t from)
{
    SBN_RemapTable_t *RemapTable = SBN.RemapTable;
    int start = 0, end = RemapTable->Entries - 1, midpoint = 0;

    while(end > start)
    {
        midpoint = (end + start) / 2;
        if(RemapTable->Entry[midpoint].ProcessorId == ProcessorId
            && RemapTable->Entry[midpoint].from == from)
        {
            break;
        }/* end if */

        if(RemapTable->Entry[midpoint].ProcessorId < ProcessorId
            || (RemapTable->Entry[midpoint].ProcessorId == ProcessorId
            && RemapTable->Entry[midpoint].from < from))
        {
            if(midpoint == start) /* degenerate case where end = start + 1 */
            {
                return 0;
            }
            else
            {
                start = midpoint;
            }/* end if */
        }
        else
        {
            end = midpoint;
        }/* end if */
    }

    if(end > start)
    {
        return RemapTable->Entry[midpoint].to;
    }/* end if */

    /* not found... */

    if(RemapTable->RemapDefaultFlag == SBN_REMAP_DEFAULT_SEND)
    {
        return from;
    }/* end if */

    return 0;
}/* end RemapMID */

/** \brief SBN global application data. */
SBN_App_t SBN;

/**
 * Iterate through all peers, examining the pipe to see if there are messages
 * I need to send to that peer.
 */
static void CheckPeerPipes(void)
{
    int PeerIdx = 0, ReceivedFlag = 0, iter = 0;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    CFE_SB_SenderId_t * lastSenderPtr = NULL;

    /* DEBUG_START(); chatty */

    /**
     * \note This processes one message per peer, then start again until no
     * peers have pending messages. At max only process SBN_MAX_MSG_PER_WAKEUP
     * per peer per wakeup otherwise I will starve other processing.
     */
    for(iter = 0; iter < SBN_MAX_MSG_PER_WAKEUP; iter++)
    {
        ReceivedFlag = 0;

        for(PeerIdx = 0; PeerIdx < SBN.Hk.PeerCount; PeerIdx++)
        {
            /* if peer data is not in use, go to next peer */
            if(SBN.Hk.PeerStatus[PeerIdx].State != SBN_HEARTBEATING)
            {
                continue;
            }

            if(CFE_SB_RcvMsg(&SBMsgPtr, SBN.Peers[PeerIdx].Pipe, CFE_SB_POLL)
                != CFE_SUCCESS)
            {
                continue;
            }/* end if */

            ReceivedFlag = 1;

            /* don't re-send what SBN sent */
            CFE_SB_GetLastSenderId(&lastSenderPtr, SBN.Peers[PeerIdx].Pipe);

            if(strncmp(SBN.App_FullName, lastSenderPtr->AppName,
                OS_MAX_API_NAME))
            {
                if(SBN.RemapTable)
                {
                    CFE_SB_MsgId_t mid =
                        RemapMID(SBN.Hk.PeerStatus[PeerIdx].ProcessorId,
                        CFE_SB_GetMsgId(SBMsgPtr));
                    if(!mid)
                    {
                        break; /* don't send message, filtered out */
                    }/* end if */
                    CFE_SB_SetMsgId(SBMsgPtr, mid);
                }/* end if */
                SBN_SendNetMsg(SBN_APP_MSG,
                    CFE_SB_GetTotalMsgLength(SBMsgPtr),
                    (SBN_Payload_t *)SBMsgPtr, PeerIdx);
            }/* end if */
        }/* end for */

        if(!ReceivedFlag)
        {
            break;
        }/* end if */
    } /* end for */
}/* end CheckPeerPipes */

/**
 * SBN uses an inline protocol for maintaining the peer connections. Before a
 * peer is connected, an SBN_ANNOUNCE_MSG message is sent. (This is necessary
 * for connectionless protocols such as UDP.) Once the peer is connected,
 * a heartbeat is sent if no traffic is seen from that peer in the last
 * SBN_HEARTBEAT_SENDTIME seconds. If no traffic (heartbeat or data) is seen
 * in the last SBN_HEARTBEAT_TIMEOUT seconds, the peer is considered to be
 * lost/disconnected.
 */
static void RunProtocol(void)
{
    int         PeerIdx = 0;
    OS_time_t   current_time;

    /* DEBUG_START(); chatty */

    CFE_ES_PerfLogEntry(SBN_PERF_SEND_ID);

    for(PeerIdx = 0; PeerIdx < SBN.Hk.PeerCount; PeerIdx++)
    {
        OS_GetLocalTime(&current_time);

        if(SBN.Hk.PeerStatus[PeerIdx].State == SBN_ANNOUNCING)
        {
            if(current_time.seconds - SBN.Hk.PeerStatus[PeerIdx].LastSent.seconds
                    > SBN_ANNOUNCE_TIMEOUT)
            {
                SBN_Payload_t AnnMsgPayload;
                strncpy(AnnMsgPayload.AnnounceMsg, SBN_IDENT, SBN_IDENT_LEN);
                SBN_SendNetMsg(SBN_ANNOUNCE_MSG, SBN_IDENT_LEN,
                    &AnnMsgPayload, PeerIdx);
            }/* end if */
            continue;
        }/* end if */
        if(current_time.seconds - SBN.Hk.PeerStatus[PeerIdx].LastReceived.seconds
                > SBN_HEARTBEAT_TIMEOUT)
        {
            /* lost connection, reset */
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
                "peer %d lost connection", PeerIdx);
            SBN_RemoveAllSubsFromPeer(PeerIdx);
            SBN.Hk.PeerStatus[PeerIdx].State = SBN_ANNOUNCING;
            continue;
        }/* end if */
        if(current_time.seconds - SBN.Hk.PeerStatus[PeerIdx].LastSent.seconds
                > SBN_HEARTBEAT_SENDTIME)
        {
            SBN_SendNetMsg(SBN_HEARTBEAT_MSG, 0, NULL, PeerIdx);
	}/* end if */
    }/* end for */

    CFE_ES_PerfLogExit(SBN_PERF_SEND_ID);
}/* end RunProtocol */

/**
 * This function waits for the scheduler (SCH) to wake this code up, so that
 * nothing transpires until the cFE is fully operational.
 *
 * @param[in] iTimeOut The time to wait for the scheduler to notify this code.
 * @return CFE_SUCCESS on success, otherwise an error value.
 */
static int32 WaitForWakeup(int32 iTimeOut)
{
    int32           Status = CFE_SUCCESS;
    CFE_SB_MsgPtr_t Msg = 0;

    /* DEBUG_START(); chatty */

    /* Wait for WakeUp messages from scheduler */
    Status = CFE_SB_RcvMsg(&Msg, SBN.CmdPipe, iTimeOut);

    switch(Status)
    {
        case CFE_SB_NO_MESSAGE:
        case CFE_SB_TIME_OUT:
            Status = CFE_SUCCESS;
            break;
        case CFE_SUCCESS:
            SBN_HandleCommand(Msg);
            break;
        default:
            return Status;
    }/* end switch */

    /* For sbn, we still want to perform cyclic processing
    ** if the WaitForWakeup time out
    ** cyclic processing at timeout rate
    */
    CFE_ES_PerfLogEntry(SBN_PERF_RECV_ID);

    RunProtocol();
    SBN_RecvNetMsgs();
    SBN_CheckSubscriptionPipe();
    CheckPeerPipes();

    if(Status == CFE_SB_NO_MESSAGE) Status = CFE_SUCCESS;

    CFE_ES_PerfLogExit(SBN_PERF_RECV_ID);

    return Status;
}/* end WaitForWakeup */

/**
 * Waits for either a response to the "get subscriptions" message from SB, OR
 * an event message that says SB has finished initializing. The latter message
 * means that SB was not started at the time SBN sent the "get subscriptions"
 * message, so that message will need to be sent again.
 * @return TRUE if message received was a initialization message and
 *      requests need to be sent again, or
 * @return FALSE if message received was a response
 */
static int WaitForSBStartup(void)
{
    CFE_EVS_Packet_t *EvsPacket = NULL;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    uint8 counter = 0;
    CFE_SB_PipeId_t EventPipe = 0;
    uint32 Status = CFE_SUCCESS;

    DEBUG_START();

    /* Create event message pipe */
    Status = CFE_SB_CreatePipe(&EventPipe, 100, "SBNEventPipe");
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to create event pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    /* Subscribe to event messages temporarily to be notified when SB is done
     * initializing
     */
    Status = CFE_SB_Subscribe(CFE_EVS_EVENT_MSG_MID, EventPipe);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to event pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    while(1)
    {
        /* Check for subscription message from SB */
        if(SBN_CheckSubscriptionPipe())
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
            if(CFE_SB_GetMsgId(SBMsgPtr) == CFE_EVS_EVENT_MSG_MID)
            {
                EvsPacket = (CFE_EVS_Packet_t *)SBMsgPtr;

                /* If it's an event message from SB, make sure it's the init
                 * message
                 */
#ifdef SBN_PAYLOAD
                if(strcmp(EvsPacket->Payload.PacketID.AppName, "CFE_SB") == 0
                    && EvsPacket->Payload.PacketID.EventID == CFE_SB_INIT_EID)
                {
                    break;
                }/* end if */
#else /* !SBN_PAYLOAD */
                if(strcmp(EvsPacket->PacketID.AppName, "CFE_SB") == 0
                    && EvsPacket->PacketID.EventID == CFE_SB_INIT_EID)
                {
                    break;
                }/* end if */
#endif /* SBN_PAYLOAD */
            }/* end if */
        }/* end if */

        counter++;
    }/* end while */

    /* Unsubscribe from event messages */
    CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID, EventPipe);

    CFE_SB_DeletePipe(EventPipe);

    /* SBN needs to re-send request messages */
    return TRUE;
}/* end WaitForSBStartup */

/** \brief Initializes SBN */
static int Init(void)
{
    int Status = CFE_SUCCESS;
    uint32 TskId = 0;

    Status = CFE_ES_RegisterApp();
    if(Status != CFE_SUCCESS) return Status;

    Status = CFE_EVS_Register(NULL, 0, CFE_EVS_BINARY_FILTER);
    if(Status != CFE_SUCCESS) return Status;

    DEBUG_START();

    CFE_PSP_MemSet(&SBN, 0, sizeof(SBN));
    CFE_SB_InitMsg(&SBN.Hk, SBN_TLM_MID, sizeof(SBN_HkPacket_t), TRUE);

    /* load the App_FullName so I can ignore messages I send out to SB */
    TskId = OS_TaskGetId();
    CFE_SB_GetAppTskName(TskId, SBN.App_FullName);

    if(SBN_ReadModuleFile() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "module file not found or data invalid");
        return SBN_ERROR;
    }/* end if */

    if(SBN_GetPeerFileData() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "peer file not found or data invalid");
        return SBN_ERROR;
    }/* end if */

    SBN_InitInterfaces();

    CFE_ES_GetAppID(&SBN.AppId);

    /* Create pipe for subscribes and unsubscribes from SB */
    Status = CFE_SB_CreatePipe(&SBN.SubPipe, SBN_SUB_PIPE_DEPTH, "SBNSubPipe");
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to create subscription pipe (Status=%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ALLSUBS_TLM_MID, SBN.SubPipe,
        SBN_MAX_ALLSUBS_PKTS_ON_PIPE);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to allsubs (Status=%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ONESUB_TLM_MID, SBN.SubPipe,
        SBN_MAX_ONESUB_PKTS_ON_PIPE);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to sub (Status=%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    /* Create pipe for HK requests and gnd commands */
    Status = CFE_SB_CreatePipe(&SBN.CmdPipe, 20, "SBNCmdPipe");
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to create command pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    Status = CFE_SB_Subscribe(SBN_CMD_MID, SBN.CmdPipe);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to command pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    Status = SBN_LoadTables(&SBN.TableHandle);
    if (Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "SBN failed to load SBN.RemapTable (%d)", Status);
        return SBN_ERROR;
    }/* end if */

    CFE_TBL_GetAddress((void **)&SBN.RemapTable, SBN.TableHandle);

    CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
        "initialized (CFE_CPU_NAME='%s' ProcessorId=%d SpacecraftId=%d %s "
        "SBN.AppId=%d...",
        CFE_CPU_NAME, CFE_PSP_GetProcessorId(), CFE_PSP_GetSpacecraftId(),
#ifdef SOFTWARE_BIG_BIT_ORDER
        "big-endian",
#else /* !SOFTWARE_BIG_BIT_ORDER */
        "little-endian",
#endif /* SOFTWARE_BIG_BIT_ORDER */
        (int)SBN.AppId);
    CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
        "...SBN_IDENT=%s SBN_DEBUG_MSGS=%s CMD_MID=0x%04X)",
        SBN_IDENT,
#ifdef SBN_DEBUG_MSGS
        "TRUE",
#else /* !SBN_DEBUG_MSGS */
        "FALSE",
#endif /* SBN_DEBUG_MSGS */
        SBN_CMD_MID
        );

    SBN_InitializeCounters();

    /* Wait for event from SB saying it is initialized OR a response from SB
       to the above messages. TRUE means it needs to re-send subscription
       requests */
    if(WaitForSBStartup()) SBN_SendSubsRequests();

    return SBN_SUCCESS;
}/* end Init */

/** \brief SBN Main Routine */
void SBN_AppMain(void)
{
    int     Status = CFE_SUCCESS;
    uint32  RunStatus = CFE_ES_APP_RUN;

    Status = Init();
    if(Status != CFE_SUCCESS) RunStatus = CFE_ES_APP_ERROR;

    /* Loop Forever */
    while(CFE_ES_RunLoop(&RunStatus)) WaitForWakeup(SBN_MAIN_LOOP_DELAY);

    CFE_ES_ExitApp(RunStatus);
}/* end SBN_AppMain */

/**
 * Creates a local pipe to receive messages from the software bus that this
 * application will send to the peer.
 *
 * @param[in] PeerIdx The index of the peer.
 * @return SBN_SUCCESS on success, otherwise error code.
 */
int SBN_CreatePipe4Peer(SBN_PeerInterface_t *PeerInterface)
{
    int32   Status = 0;
    char    PipeName[OS_MAX_API_NAME];

    DEBUG_START();

    /* create a pipe name string similar to SBN_CPU2_Pipe */
    snprintf(PipeName, OS_MAX_API_NAME, "SBN_%s_Pipe",
        PeerInterface->Status->Name);
    Status = CFE_SB_CreatePipe(&PeerInterface->Pipe, SBN_PEER_PIPE_DEPTH,
            PipeName);

    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "failed to create pipe '%s'", PipeName);

        return Status;
    }/* end if */

    CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
        "pipe created '%s'", PipeName);

    return SBN_SUCCESS;
}/* end SBN_CreatePipe4Peer */

/**
 * Sends a message to a peer.
 * @param[in] MsgType The type of the message (application data, SBN protocol)
 * @param[in] CpuId The CpuId to send this message to.
 * @param[in] MsgSize The size of the message (in bytes).
 * @param[in] Msg The message contents.
 */
void SBN_ProcessNetMsg(SBN_MsgType_t MsgType, SBN_CpuId_t CpuId,
    SBN_MsgSize_t MsgSize, void *Msg)
{
    int PeerIdx = 0, Status = 0;

    DEBUG_START();

    PeerIdx = SBN_GetPeerIndex(CpuId);

    if(PeerIdx == SBN_ERROR)
    {
        return;
    }/* end if */

    if(MsgType == SBN_ANNOUNCE_MSG)
    {
        if(MsgSize < 1)
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
                "peer running old (unversioned) code!");
        }
        else
        {
            if(strncmp(SBN_IDENT, (char *)Msg, SBN_IDENT_LEN))
            {
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
                    "peer #%d version mismatch (me=%s, him=%s)",
                    PeerIdx, SBN_IDENT, (char *)Msg);
            }
            else
            {
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
                    "peer #%d running same version of SBN (%s)",
                    PeerIdx, SBN_IDENT);
            }/* end if */
        }/* end if */
    }/* end if */

    if(SBN.Hk.PeerStatus[PeerIdx].State == SBN_ANNOUNCING
        || MsgType == SBN_ANNOUNCE_MSG)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
            "peer #%d alive", PeerIdx);
        SBN.Hk.PeerStatus[PeerIdx].State = SBN_HEARTBEATING;
        SBN_SendLocalSubsToPeer(PeerIdx);
    }/* end if */

    switch(MsgType)
    {
        case SBN_ANNOUNCE_MSG:
        case SBN_HEARTBEAT_MSG:
            break;

        case SBN_APP_MSG:
            Status = CFE_SB_SendMsgFull(Msg,
                CFE_SB_DO_NOT_INCREMENT, CFE_SB_SEND_ONECOPY);

            if(Status != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(SBN_SB_EID, CFE_EVS_ERROR,
                    "CFE_SB_SendMsg error (Status=%d MsgType=0x%x)",
                    Status, MsgType);
            }/* end if */
            break;

        case SBN_SUBSCRIBE_MSG:
        {
            SBN_ProcessSubFromPeer(PeerIdx, Msg);
            break;
        }

        case SBN_UN_SUBSCRIBE_MSG:
        {
            SBN_ProcessUnsubFromPeer(PeerIdx, Msg);
            break;
        }

        default:
            /* make sure of termination */
            CFE_EVS_SendEvent(SBN_MSG_EID, CFE_EVS_ERROR,
                "unknown message type (MsgType=0x%x)", MsgType);
            break;
    }/* end switch */
}/* end SBN_ProcessNetMsg */

/**
 * Find the PeerIndex for a given CpuId.
 * @param[in] ProcessorId The CpuId of the peer being sought.
 * @return The Peer index (index into the SBN.Peer array, or SBN_ERROR.
 */
int SBN_GetPeerIndex(uint32 ProcessorId)
{
    int PeerIdx = 0;

    /* DEBUG_START(); chatty */

    for(PeerIdx = 0; PeerIdx < SBN.Hk.PeerCount; PeerIdx++)
    {
        if(SBN.Hk.PeerStatus[PeerIdx].ProcessorId == ProcessorId)
        {
            return PeerIdx;
        }/* end if */
    }/* end for */

    return SBN_ERROR;
}/* end SBN_GetPeerIndex */
