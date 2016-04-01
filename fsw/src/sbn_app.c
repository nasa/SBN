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
 **      This file contains source code for the Software Bus Network Application.
 **
 ** Authors:   J. Wilmot/GSFC Code582
 **            R. McGraw/SSI
 **
 ** $Log: sbn_app.c  $
 ** Revision 1.4 2010/10/05 15:24:12EDT jmdagost
 ** Cleaned up copyright symbol.
 ** Revision 1.3 2009/06/12 14:39:04EDT rmcgraw
 ** DCR82191:1 Changed OS_Mem function calls to CFE_PSP_Mem
 ** Revision 1.2 2008/04/08 08:07:09EDT ruperera
 ** Member moved from sbn_app.c in project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/cfs_sbn.pj to sbn_app.c in project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/fsw/src/project.pj.
 ** Revision 1.1 2008/04/08 07:07:09ACT ruperera
 ** Initial revision
 ** Member added to project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/cfs_sbn.pj
 ** Revision 1.1 2007/11/30 17:17:45EST rjmcgraw
 ** Initial revision
 ** Member added to project d:/mksdata/MKS-CFS-REPOSITORY/sbn/cfs_sbn.pj
 ** Revision 1.17 2007/05/16 11:37:58EDT rjmcgraw
 ** DCR3052:8 Fixed index range problem in SBN_StateNum2Str found by IV&V
 ** Revision 1.16 2007/04/19 15:52:43EDT rjmcgraw
 ** DCR3052:7 Moved and renamed CFE_SB_MAX_NETWORK_PEERS to this file
 ** Revision 1.14 2007/03/13 13:18:10EST rjmcgraw
 ** Replaced hardcoded 4 with SBN_ITEMS_PER_FILE_LINE
 ** Revision 1.13 2007/03/07 11:07:18EST rjmcgraw
 ** Removed references to channel
 ** Revision 1.12 2007/02/28 15:05:00EST rjmcgraw
 ** Removed unused #defines
 ** Revision 1.11 2007/02/27 09:37:40EST rjmcgraw
 ** Minor cleanup
 ** Revision 1.9 2007/02/23 14:53:35EST rjmcgraw
 ** Moved subscriptions from protocol port to data port
 ** Revision 1.8 2007/02/22 15:14:35EST rjmcgraw
 ** Changed PeerData file to exclude mac address
 ** Revision 1.6.1.1 2007/02/21 14:49:36EST rjmcgraw
 ** Debug events changed to OS_printfs
 **
 ******************************************************************************/

/*
 ** Include Files
 */
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>

#include "cfe.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"
#include "sbn_app.h"
#include "sbn_netif.h"
#include "sbn_msgids.h"
#include "sbn_loader.h"
#include "sbn_cmds.h"
#include "sbn_buf.h"
#include "sbn_subs.h"
#include "sbn_main_events.h"
#include "sbn_perfids.h"
#include "cfe_sb_events.h" /* For event message IDs */
#include "cfe_sb_priv.h" /* For CFE_SB_SendMsgFull */
#include "cfe_es.h" /* PerfLog */

/*
 **   Task Globals
 */

/** \brief SBN Main Routine */
void SBN_AppMain(void)
{
    int     Status = CFE_SUCCESS;
    uint32  RunStatus = CFE_ES_APP_RUN;

    Status = SBN_Init();
    if(Status != CFE_SUCCESS) RunStatus = CFE_ES_APP_ERROR;

    /* Loop Forever */
    while(CFE_ES_RunLoop(&RunStatus)) SBN_RcvMsg(SBN_MAIN_LOOP_DELAY);

    CFE_ES_ExitApp(RunStatus);
}/* end SBN_AppMain */

/** \brief Initializes SBN */
int SBN_Init(void)
{
    int Status = CFE_SUCCESS;

    Status = CFE_ES_RegisterApp();
    if(Status != CFE_SUCCESS) return Status;

    Status = CFE_EVS_Register(NULL, 0, CFE_EVS_BINARY_FILTER);
    if(Status != CFE_SUCCESS) return Status;

    DEBUG_START();

    CFE_PSP_MemSet(&SBN, 0, sizeof(SBN));

    if(SBN_ReadModuleFile() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "SBN APP Will Terminate, Module File Not Found or Data Invalid!");
        return SBN_ERROR;
    }/* end if */

    if(SBN_GetPeerFileData() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "SBN APP Will Terminate, Peer File Not Found or Data Invalid!");
        return SBN_ERROR;
    }/* end if */

    if(SBN_InitProtocol() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "SBN APP Will Terminate, Error Creating Interfaces!");
        return SBN_ERROR;
    }/* end if */

    CFE_ES_GetAppID(&SBN.AppId);

    /* Init schedule pipe */
    SBN.usSchPipeDepth = SBN_SCH_PIPE_DEPTH;
    strncpy(SBN.cSchPipeName, "SBN_SCH_PIPE", OS_MAX_API_NAME-1);

    /* Subscribe to Wakeup messages */
    Status = CFE_SB_CreatePipe(&SBN.SchPipeId, SBN.usSchPipeDepth,
        SBN.cSchPipeName);
    if(Status == CFE_SUCCESS)
    {
        CFE_SB_Subscribe(SBN_WAKEUP_MID, SBN.SchPipeId);
    }
    else
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "SBN APP Failed to create SCH pipe (0x%08X)", Status);
        return SBN_ERROR;
    }/* end if */

    /* Create pipe for subscribes and unsubscribes from SB */
    CFE_SB_CreatePipe(&SBN.SubPipe, SBN_SUB_PIPE_DEPTH, "SBNSubPipe");
    CFE_SB_SubscribeLocal(CFE_SB_ALLSUBS_TLM_MID, SBN.SubPipe,
            SBN_MAX_ALLSUBS_PKTS_ON_PIPE);

    CFE_SB_SubscribeLocal(CFE_SB_ONESUB_TLM_MID, SBN.SubPipe,
            SBN_MAX_ONESUB_PKTS_ON_PIPE);

    /* Create pipe for HK requests and gnd commands */
    CFE_SB_CreatePipe(&SBN.CmdPipe,20,"SBNCmdPipe");
    CFE_SB_Subscribe(SBN_CMD_MID,SBN.CmdPipe);
    CFE_SB_Subscribe(SBN_SEND_HK_MID,SBN.CmdPipe);

    CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
        "SBN APP Initialized V1.1, AppId=%d", SBN.AppId);

    /* Initialize HK Message */
    CFE_SB_InitMsg(&SBN.HkPkt, SBN_HK_TLM_MID, sizeof(SBN_HkPacket_t), TRUE);
    SBN_InitializeCounters();

    /* Create event message pipe */
    CFE_SB_CreatePipe(&SBN.EventPipe, 100, "SBNEventPipe");

    /* Wait for event from SB saying it is initialized OR a response from SB
       to the above messages. SBN_TRUE means it needs to re-send subscription
       requests */
    if(SBN_WaitForSBStartup()) SBN_SendSubsRequests();

    return SBN_OK;
}/* end SBN_Init */

/**
 * Waits for either a response to the "get subscriptions" message from SB, OR
 * an event message that says SB has finished initializing. The latter message
 * means that SB was not started at the time SBN sent the "get subscriptions"
 * message, so that message will need to be sent again.
 * @return SBN_TRUE if message received was a initialization message and requests
 *                  need to be sent again, or
 * @return SBN_FALSE if message received was a response
 */
int SBN_WaitForSBStartup()
{
    CFE_EVS_Packet_t    *EvsPacket = NULL;
    CFE_SB_MsgPtr_t     SBMsgPtr = 0;
    uint8               counter = 0;

    DEBUG_START();

    /* Subscribe to event messages temporarily to be notified when SB is done
     * initializing
     */
    CFE_SB_Subscribe(CFE_EVS_EVENT_MSG_MID, SBN.EventPipe);

    while(1)
    {
        /* Check for subscription message from SB */
        if(SBN_CheckSubscriptionPipe())
        {
            /* SBN does not need to re-send request messages to SB */
            return SBN_TRUE;
        }
        else if(counter % 100 == 0)
        {
            /* Send subscription request messages again. This may cause the SB
             * to respond to duplicate requests but that should be okay
             */
            SBN_SendSubsRequests();
        }/* end if */

        /* Check for event message from SB */
        if(CFE_SB_RcvMsg(&SBMsgPtr, SBN.EventPipe, 100) == CFE_SUCCESS)
        {
            if(CFE_SB_GetMsgId(SBMsgPtr) == CFE_EVS_EVENT_MSG_MID)
            {
                EvsPacket = (CFE_EVS_Packet_t *)SBMsgPtr;

                /* If it's an event message from SB, make sure it's the init
                 * message
                 */
#ifdef SBN_PAYLOAD
                if(strcmp(EvsPacket->Payload.PacketID.AppName, "CFE_SB") == 0)
                {
                    if(EvsPacket->Payload.PacketID.EventID == CFE_SB_INIT_EID)
                    {
                        /* Unsubscribe from event messages */
                        CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID,
                            SBN.EventPipe);

                        /* SBN needs to re-send request messages */
                        return SBN_TRUE;
                    }/* end if */
                }/* end if */
#else /* !SBN_PAYLOAD */
                if(strcmp(EvsPacket->PacketID.AppName, "CFE_SB") == 0)
                {
                    if(EvsPacket->PacketID.EventID == CFE_SB_INIT_EID)
                    {
                        /* Unsubscribe from event messages */
                        CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID,
                            SBN.EventPipe);

                        /* SBN needs to re-send request messages */
                        return SBN_TRUE;
                    }/* end if */
                }/* end if */
#endif /* SBN_PAYLOAD */
            }/* end if */
        }/* end if */

        counter++;
    }/* end while */

    return SBN_TRUE; /* This code should never be reached */
}/* end SBN_WaitForSBStartup */

/*============================================================================
** \parName: SBN_RcvMsg
**
** Purpose: To receive and process messages for TA4 application
**
** Arguments:
**    iTimeOut - amount of time to wait for a scheduler message before the
**               RcvMsg call times out and returns.  This supports usage of
**               this app in systems not using a local scheduler.  The main
**               sbn loop will still execute at
**               1 / (SBN_MAIN_LOOP_DELAY / 1000) Hz, SBN_MAIN_LOOP_DELAY is in
**               milliseconds without a wakeup call from a scheduler.
**
** Returns:
**    int32 Status - error Status
**
** Routines Called:
**    CFE_SB_RcvMsg
**    CFE_SB_GetMsgId
**    CFE_EVS_SendEvent
**    CFE_ES_PerfLogEntry
**    CFE_ES_PerfLogExit
**    SBN_RunProtocol
**    SBN_CheckForNetAppMsgs
**    SBN_CheckSubscriptionPipe
**    SBN_CheckPeerPipes
**    SBN_CheckCmdPipe
**
** Called By:
**    SBN_Main
**
** Global Inputs/Reads:
**    SBN.SchPipeId
**
** Global Outputs/Writes:
**    SBN.uiRunStatus
**
** Limitations, Assumptions, External Events, and Notes:
**    1. List assumptions that are made that apply to this function.
**    2. List the external source(s) and event(s) that can cause this function
**       to execute.
**    3. List known limitations that apply to this function.
**    4. If there are no assumptions, external events, or notes then enter NONE.
**       Do not omit the section.
**
** Algorithm:
**    Psuedo-code or description of basic algorithm
**
** Author(s):  Steve Duran
**
** History:  Date Written  2013-06-04
**           Unit Tested   yyyy-mm-dd
**=========================================================================*/
int32 SBN_RcvMsg(int32 iTimeOut)
{
    int32           Status = CFE_SUCCESS;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    CFE_SB_MsgId_t  MsgId;

    /* TODO: To include performance logging for this application,
        uncomment CFE_ES_PerfLogEntry() and CFE_ES_PerfLogExit() lines */

    /* DEBUG_START(); chatty */

    /* Wait for WakeUp messages from scheduler */
    Status = CFE_SB_RcvMsg(&SBMsgPtr, SBN.SchPipeId, iTimeOut);

    /* Performance Log Entry stamp */
    CFE_ES_PerfLogEntry(SBN_PERF_RECV_ID);

    /* success or timeout is ok to proceed through main loop */
    if(Status == CFE_SUCCESS)
    {
        MsgId = CFE_SB_GetMsgId(SBMsgPtr); /* note MsgId is platform-endian */
        switch(MsgId)
        {
            case SBN_WAKEUP_MID:
                /* cyclic processing at sch wakeup rate */
                SBN_RunProtocol();
                SBN_CheckForNetAppMsgs();
                SBN_CheckSubscriptionPipe();
                SBN_CheckPeerPipes();
                SBN_CheckCmdPipe();
                break;

            /* TODO:  Add code here to handle other commands, if needed */

            default:
                CFE_EVS_SendEvent(SBN_MSG_EID, CFE_EVS_ERROR,
                    "SBN - Recvd invalid SCH msgId (0x%04X)", MsgId);
        }/* end switch */
    }
    else if(Status == CFE_SB_TIME_OUT)
    {
        /* For sbn, we still want to perform cyclic processing
        ** if the RcvMsg time out
        ** cyclic processing at timeout rate
        */
        SBN_RunProtocol();
        SBN_CheckForNetAppMsgs();
        SBN_CheckSubscriptionPipe();
        SBN_CheckPeerPipes();
        SBN_CheckCmdPipe();
    }
    else if(Status == CFE_SB_NO_MESSAGE)
    {
        /* If there's no incoming message, you can do something here,
         * or do nothing
         */
        ;
    }
    else
    {
        /* This is an example of exiting on an error.
         * Note that a SB read error is not always going to result in an app
         * quitting.
         */
        CFE_EVS_SendEvent(SBN_PIPE_EID, CFE_EVS_ERROR,
            "SBN: SB pipe read error (0x%08X), app will exit", Status);
        /* sbn does not have a run Status var:
         * SBN.uiRunStatus= CFE_ES_APP_ERROR;
         */
    }/* end if */

    /* Performance Log Exit stamp */
    CFE_ES_PerfLogExit(SBN_PERF_RECV_ID);

    return Status;
}/* end SBN_RcvMsg */

int SBN_InitProtocol(void)
{
    DEBUG_START();

    SBN_InitPeerVariables();
    SBN_InitPeerInterface();
    SBN_VerifyPeerInterfaces();
    SBN_VerifyHostInterfaces();
    return SBN_OK;
}/* end SBN_InitProtocol */

void SBN_InitPeerVariables(void)
{
    DEBUG_START();

    int     PeerIdx = 0, j = 0;

    for(PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {
        SBN.Peer[PeerIdx].InUse = SBN_NOT_IN_USE;
        SBN.Peer[PeerIdx].SubCnt = 0;
        for(j = 0; j < SBN_MAX_SUBS_PER_PEER; j++)
        {
            SBN.Peer[PeerIdx].Sub[j].InUseCtr = 0;
        }/* end for */

        SBN.LocalSubCnt = 0;
    }/* end for */
}/* end SBN_InitPeerVariables */

int SBN_CreatePipe4Peer(int PeerIdx)
{
    int32   Status = 0;
    char    PipeName[OS_MAX_API_NAME];

    DEBUG_START();

    /* create a pipe name string similar to SBN_CPU2_Pipe */
    sprintf(PipeName, "SBN_%s_Pipe", SBN.Peer[PeerIdx].Name);
    Status = CFE_SB_CreatePipe(&SBN.Peer[PeerIdx].Pipe, SBN_PEER_PIPE_DEPTH,
            PipeName);

    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "%s:failed to create pipe %s", CFE_CPU_NAME, PipeName);

        return Status;
    }/* end if */

    strncpy(SBN.Peer[PeerIdx].PipeName, PipeName, OS_MAX_API_NAME);

    CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
        "%s: pipe created %s", CFE_CPU_NAME, PipeName);

    return SBN_OK;
}/* end SBN_CreatePipe4Peer */

void SBN_RunProtocol(void)
{
    int         PeerIdx = 0;
    OS_time_t   current_time;
    SBN_Hdr_t   msg;

    CFE_PSP_MemSet(&msg, 0, sizeof(msg));

    /* DEBUG_START(); chatty */

    CFE_ES_PerfLogEntry(SBN_PERF_SEND_ID);

    /* The protocol is run for each peer in use. In Linux, the UDP/IP message
     * jitter/latency is enough to have 2 HB and 2 HB ack messages pending
     * With a low SBN_TIMEOUT_CYCLES value, this simple protocol can deal with
     * that for up to two consecutive cycles */

    /* The protocol engine assumes that messages have been cleared prior to the
     * first run. SBN init function does this */
    for(PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {
        /* if peer data is not in use, go to next peer */
        if(SBN.Peer[PeerIdx].InUse != SBN_IN_USE) continue;

        OS_GetLocalTime(&current_time);

        if(SBN.Peer[PeerIdx].State == SBN_ANNOUNCING)
        {
            if(current_time.seconds - SBN.Peer[PeerIdx].last_sent.seconds
                    > SBN_ANNOUNCE_TIMEOUT)
            {
                msg.Type = SBN_ANNOUNCE_MSG;
                msg.MsgSize = sizeof(msg);
                SBN_SendNetMsgNoBuf((SBN_NetPkt_t *)&msg, PeerIdx);
            }/* end if */
            return;
        }/* end if */
        if(current_time.seconds - SBN.Peer[PeerIdx].last_received.seconds
                > SBN_HEARTBEAT_TIMEOUT)
        {
            /* lost connection, reset */
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
                "peer %d lost connection, resetting\n", PeerIdx);
            SBN_RemoveAllSubsFromPeer(PeerIdx);
            SBN.Peer[PeerIdx].State = SBN_ANNOUNCING;

        }/* end if */
        if(current_time.seconds - SBN.Peer[PeerIdx].last_sent.seconds
                > SBN_HEARTBEAT_SENDTIME)
        {
            msg.Type = SBN_HEARTBEAT_MSG;
            msg.MsgSize = sizeof(msg);
            SBN_SendNetMsgNoBuf((SBN_NetPkt_t *)&msg, PeerIdx);
	}/* end if */
    }/* end for */

    CFE_ES_PerfLogExit(SBN_PERF_SEND_ID);
}/* end SBN_RunProtocol */

static uint16 CheckMsgSize(CFE_SB_MsgPtr_t SBMsgPtr, int PeerIdx)
{
    uint16              AppMsgSize = 0, MaxAppMsgSize = 0;

    DEBUG_START();

    /* Find size of app msg without SBN hdr */
    AppMsgSize = CFE_SB_GetTotalMsgLength(SBMsgPtr);
    MaxAppMsgSize = (SBN_MAX_MSG_SIZE - sizeof(SBN_Hdr_t));

    if(AppMsgSize >= MaxAppMsgSize)
    {
        CFE_EVS_SendEvent(SBN_MSG_EID, CFE_EVS_INFORMATION,
            "%s:AppMsg 0x%04X, sz=%d destined for"
            " %s truncated to %d(max sz)",
            CFE_CPU_NAME, CFE_SB_GetMsgId(SBMsgPtr),
            AppMsgSize, SBN.Peer[PeerIdx].Name, MaxAppMsgSize);

        AppMsgSize = MaxAppMsgSize;
    }/* end if */

    return AppMsgSize;
}/* end CheckMsgSize */

void SBN_CheckPipe(int PeerIdx, int32 * priority_remaining)
{
    CFE_SB_MsgPtr_t     SBMsgPtr = 0;
    uint16              AppMsgSize = 0;
    CFE_SB_SenderId_t * lastSenderPtr = NULL;
    SBN_NetPkt_t        msg;

    CFE_PSP_MemSet(&msg, 0, sizeof(msg));

    /* DEBUG_START(); chatty */

    if(SBN.Peer[PeerIdx].State != SBN_HEARTBEATING ||
        CFE_SB_RcvMsg(&SBMsgPtr, SBN.Peer[PeerIdx].Pipe, CFE_SB_POLL)
            != CFE_SUCCESS)
    {
        return;
    }/* end if */

    /* don't re-send what SBN sent */
    CFE_SB_GetLastSenderId(&lastSenderPtr, SBN.Peer[PeerIdx].Pipe);
    if(!strncmp(lastSenderPtr->AppName, "SBN", OS_MAX_API_NAME))
    {
        return;
    }/* end if */

    /* copy message from SB buffer to network data msg buffer */
    AppMsgSize = CheckMsgSize(SBMsgPtr, PeerIdx);
    CFE_PSP_MemCpy(msg.Data, SBMsgPtr, AppMsgSize);

    msg.Hdr.MsgSize = AppMsgSize + sizeof(SBN_Hdr_t);
    msg.Hdr.Type = SBN_APP_MSG;
    SBN_SendNetMsg(&msg, PeerIdx);
    (*priority_remaining)++;
}/* end SBN_CheckPipe */

void SBN_CheckPeerPipes(void)
{
    int     PeerIdx = 0;
    int32   priority = 0;
    int32   priority_remaining = 0;

    /* DEBUG_START(); chatty */

    while(priority < SBN_MAX_PEER_PRIORITY)
    {
        priority_remaining = 0;

        for(PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
        {
            /* if peer data is not in use, go to next peer */
            if(SBN.Peer[PeerIdx].State != SBN_HEARTBEATING)
                continue;

            if((int32)SBN_GetPeerQoSPriority(&SBN.Peer[PeerIdx]) != priority)
                continue;

            SBN_CheckPipe(PeerIdx, &priority_remaining);
        }/* end for */
        if(!priority_remaining) priority++;
    }/* end while */
}/* end SBN_CheckPeerPipes */

void SBN_ProcessNetAppMsg(SBN_NetPkt_t *msg)
{
    int                 PeerIdx = 0,
                        Status = 0;

    DEBUG_START();

    PeerIdx = SBN_GetPeerIndex(msg->Hdr.CPU_ID);

    if(PeerIdx == SBN_ERROR) return;

    if(SBN.Peer[PeerIdx].State == SBN_ANNOUNCING
        || msg->Hdr.Type == SBN_ANNOUNCE_MSG)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
            "Peer #%d alive, resetting", PeerIdx);
        SBN.Peer[PeerIdx].State = SBN_HEARTBEATING;
        SBN_ResetPeerMsgCounts(PeerIdx);
        SBN_SendLocalSubsToPeer(PeerIdx);
    }/* end if */

    switch(msg->Hdr.Type)
    {
        case SBN_ANNOUNCE_MSG:
        case SBN_ANNOUNCE_ACK_MSG:
        case SBN_HEARTBEAT_MSG:
        case SBN_HEARTBEAT_ACK_MSG:
            break;

        case SBN_APP_MSG:
            DEBUG_MSG("Injecting %d", msg->Hdr.SequenceCount);

            Status = CFE_SB_SendMsgFull((CFE_SB_MsgPtr_t)msg->Data,
                CFE_SB_DO_NOT_INCREMENT, CFE_SB_SEND_ONECOPY);

            if(Status != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(SBN_SB_EID, CFE_EVS_ERROR,
                    "%s:CFE_SB_SendMsg err %d. type 0x%x",
                    CFE_CPU_NAME, Status,
                    msg->Hdr.Type);
            }/* end if */
            break;

        case SBN_SUBSCRIBE_MSG:
        {
            SBN_NetSub_t *sub = (SBN_NetSub_t *)msg;
            SBN_ProcessSubFromPeer(PeerIdx, sub->MsgId, sub->Qos);
            break;
        }

        case SBN_UN_SUBSCRIBE_MSG:
        {
            SBN_NetSub_t *sub = (SBN_NetSub_t *)msg;
            SBN_ProcessUnsubFromPeer(PeerIdx, sub->MsgId);
            break;
        }

        default:
            /* make sure of termination */
            CFE_EVS_SendEvent(SBN_MSG_EID, CFE_EVS_ERROR,
                "%s:Unknown Msg Type 0x%x", CFE_CPU_NAME,
                msg->Hdr.Type);
            break;
    }/* end switch */
}/* end SBN_ProcessNetAppMsg */

int32 SBN_CheckCmdPipe(void)
{
    CFE_SB_MsgPtr_t     SBMsgPtr = 0;
    int                 Status = 0;

    /* DEBUG_START(); */

    /* Command and HK requests pipe */
    while(Status == CFE_SUCCESS)
    {
        Status = CFE_SB_RcvMsg(&SBMsgPtr, SBN.CmdPipe, CFE_SB_POLL);

        if(Status == CFE_SUCCESS) SBN_AppPipe(SBMsgPtr);
    }/* end while */

    if(Status == CFE_SB_NO_MESSAGE) Status = CFE_SUCCESS;

    return Status;
}/* end SBN_CheckCmdPipe */

int SBN_GetPeerIndex(uint32 ProcessorId)
{
    int     PeerIdx = 0;

    /* DEBUG_START(); chatty */

    for(PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {
        if(SBN.Peer[PeerIdx].InUse == SBN_NOT_IN_USE) continue;

        if(SBN.Peer[PeerIdx].ProcessorId == ProcessorId) return PeerIdx;
    }/* end for */

    return SBN_ERROR;
}/* end SBN_GetPeerIndex */

void SBN_ResetPeerMsgCounts(uint32 PeerIdx)
{
    DEBUG_START();

    SBN.Peer[PeerIdx].SentCount = 0;
    SBN.Peer[PeerIdx].RcvdCount = 0;
    SBN.Peer[PeerIdx].MissCount = 0;
}/* end SBN_ResetPeerMsgCounts */
