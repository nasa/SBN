#undef SBN_PAYLOAD
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
#include "cfe_sb_events.h" /* For event message IDs */
#include "cfe_sb_priv.h" /* For CFE_SB_SendMsgFull */

/*
 **   Task Globals
 */

/** \brief SBN Main Routine */
void SBN_AppMain(void) {
    int Status = CFE_SUCCESS;
    uint32 RunStatus = CFE_ES_APP_RUN;

    Status = SBN_Init();
    if(Status != CFE_SUCCESS) {
        RunStatus = CFE_ES_APP_ERROR;
    }

    /* Loop Forever */
    while (CFE_ES_RunLoop(&RunStatus)) {
        SBN_RcvMsg(SBN_MAIN_LOOP_DELAY);
    }

    CFE_ES_ExitApp(RunStatus);
}/* end SBN_AppMain */

/** \brief Initializes SBN */
int SBN_Init(void) {
    int iStatus = CFE_SUCCESS;

    iStatus = CFE_ES_RegisterApp();
    if(iStatus != CFE_SUCCESS) {
        return iStatus;
    }

    iStatus = CFE_EVS_Register(NULL, 0, CFE_EVS_BINARY_FILTER);
    if(iStatus != CFE_SUCCESS) {
        return iStatus;
    }

    if (SBN_ReadModuleFile() == SBN_ERROR) {
        CFE_EVS_SendEvent(SBN_FILE_ERR_EID, CFE_EVS_ERROR,
            "SBN APP Will Terminate, Module File Not Found or Data Invalid!");
        return SBN_ERROR;
    }

    if (SBN_GetPeerFileData() == SBN_ERROR) {
        CFE_EVS_SendEvent(SBN_FILE_ERR_EID, CFE_EVS_ERROR,
                "SBN APP Will Terminate, Peer File Not Found or Data Invalid!");
        return SBN_ERROR;
    }

    if (SBN_InitProtocol() == SBN_ERROR) {
        CFE_EVS_SendEvent(SBN_PROTO_INIT_ERR_EID, CFE_EVS_ERROR,
                "SBN APP Will Terminate, Error Creating Interfaces!");
        return SBN_ERROR;
    }

    CFE_ES_GetAppID(&SBN.AppId);

    /* Init schedule pipe */
    SBN.usSchPipeDepth = SBN_SCH_PIPE_DEPTH;
    memset((void*)SBN.cSchPipeName, '\0', sizeof(SBN.cSchPipeName));
    strncpy(SBN.cSchPipeName, "SBN_SCH_PIPE", OS_MAX_API_NAME-1);

    /* Subscribe to Wakeup messages */
    iStatus = CFE_SB_CreatePipe(&SBN.SchPipeId, SBN.usSchPipeDepth,
            SBN.cSchPipeName);
    if (iStatus == CFE_SUCCESS) {
        CFE_SB_Subscribe(SBN_WAKEUP_MID, SBN.SchPipeId);
    } else {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
                "SBN APP Failed to create SCH pipe (0x%08X)", iStatus);
        return SBN_ERROR;
    }

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

    SBN.DebugOn = SBN_FALSE;

    CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
            "SBN APP Initialized V1.1, AppId=%d", SBN.AppId);


    /* Initialize HK Message */
    CFE_SB_InitMsg(&SBN.HkPkt, SBN_HK_TLM_MID, sizeof(SBN_HkPacket_t), TRUE);
    SBN_InitializeCounters();

    /* Create event message pipe */
    CFE_SB_CreatePipe(&SBN.EventPipe, 20, "SBNEventPipe");

    /* Wait for event from SB saying it is initialized OR a response from SB
       to the above messages. SBN_TRUE means it needs to re-send subscription
       requests */
    if (SBN_WaitForSBStartup()) {
        SBN_SendSubsRequests();
    }

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
int SBN_WaitForSBStartup() {
    CFE_EVS_Packet_t *EvsPacket = NULL;
    CFE_SB_MsgPtr_t SBMsgPtr;
    uint8 MsgFound = SBN_FALSE;
    uint8 counter = 0;

    /* Subscribe to event messages temporarily to be notified when SB is done initializing */
    CFE_SB_Subscribe(CFE_EVS_EVENT_MSG_MID, SBN.EventPipe);

    while (!MsgFound)
    {
        /* Check for subscription message from SB */
        if (SBN_CheckSubscriptionPipe()) {
            /* Unsubscribe from event messages */
            CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID, SBN.EventPipe);
            MsgFound = SBN_TRUE;
            return SBN_FALSE; /* SBN does not need to re-send request messages to SB */
        }
        else if (counter % 100 == 0) {
            /* Send subscription request messages again. This may cause the SB
               to respond to duplicate requests but that should be okay */
            SBN_SendSubsRequests();
        }

        /* Check for event message from SB */
        if (CFE_SB_RcvMsg(&SBMsgPtr, SBN.EventPipe, 100) == CFE_SUCCESS) {
            if (CFE_SB_GetMsgId(SBMsgPtr) == CFE_EVS_EVENT_MSG_MID) {
                EvsPacket = (CFE_EVS_Packet_t *)SBMsgPtr;

                /* If it's an event message from SB, make sure it's the init message */
#ifdef SBN_PAYLOAD
                if ( strcmp(EvsPacket->Payload.PacketID.AppName, "CFE_SB") == 0 ) {
                    if (EvsPacket->Payload.PacketID.EventID == CFE_SB_INIT_EID) {
                        /* Unsubscribe from event messages */
                        CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID, SBN.EventPipe);
                        MsgFound = SBN_TRUE;
                        return SBN_TRUE; /* SBN needs to re-send request messages */
                    }
                }
#else /* !SBN_PAYLOAD */
                if ( strcmp(EvsPacket->PacketID.AppName, "CFE_SB") == 0 ) {
                    if (EvsPacket->PacketID.EventID == CFE_SB_INIT_EID) {
                        /* Unsubscribe from event messages */
                        CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID, SBN.EventPipe);
                        MsgFound = SBN_TRUE;
                        return SBN_TRUE; /* SBN needs to re-send request messages */
                    }
                }
#endif /* SBN_PAYLOAD */
            }
        }

        counter++;
    }

    return SBN_TRUE; /* This code should never be reached */
}

/*=====================================================================================
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
**    int32 iStatus - error status
**
** Routines Called:
**    CFE_SB_RcvMsg
**    CFE_SB_GetMsgId
**    CFE_EVS_SendEvent
**    CFE_ES_PerfLogEntry
**    CFE_ES_PerfLogExit
**    SBN_SendWakeUpDebugMsg
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
**    2. List the external source(s) and event(s) that can cause this function to execute.
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
**=====================================================================================*/
int32 SBN_RcvMsg(int32 iTimeOut) {
  int32           iStatus=CFE_SUCCESS;
  CFE_SB_Msg_t   *MsgPtr=NULL;
  CFE_SB_MsgId_t  MsgId;

  /* TODO: To include performance logging for this application,
           uncomment CFE_ES_PerfLogEntry() and CFE_ES_PerfLogExit() lines */

  /* Wait for WakeUp messages from scheduler */
  iStatus = CFE_SB_RcvMsg(&MsgPtr, SBN.SchPipeId, iTimeOut);

  /* Performance Log Entry stamp */
  /* CFE_ES_PerfLogEntry(SBN_MAIN_TASK_PERF_ID); */

  /* success or timeout is ok to proceed through main loop */
  if (iStatus == CFE_SUCCESS) {
    MsgId = CFE_SB_GetMsgId(MsgPtr); /* note MsgId is platform-endian */
    switch (MsgId) {
      case SBN_WAKEUP_MID:
        /* cyclic processing at sch wakeup rate */
        SBN_SendWakeUpDebugMsg();
        SBN_RunProtocol();
        SBN_CheckForNetAppMsgs();
        SBN_CheckSubscriptionPipe();
        SBN_CheckPeerPipes();
        SBN_CheckCmdPipe();
        break;

      /* TODO:  Add code here to handle other commands, if needed */

      default:
        CFE_EVS_SendEvent(SBN_MSGID_ERR_EID, CFE_EVS_ERROR,
                          "SBN - Recvd invalid SCH msgId (0x%04X)", MsgId);
    }
  }
  else if (iStatus == CFE_SB_TIME_OUT) {
    /* CFE_ES_WriteToSysLog("SBN TIMEOUT: MID: 0x%X\n", SBN_WAKEUP_MID); */

    /* For sbn, we still want to perform cyclic processing
     * if the RcvMsg time out
     * cyclic processing at timeout rate */
    SBN_SendWakeUpDebugMsg();
    SBN_RunProtocol();
    SBN_CheckForNetAppMsgs();
    SBN_CheckSubscriptionPipe();
    SBN_CheckPeerPipes();
    SBN_CheckCmdPipe();
  }
  else if (iStatus == CFE_SB_NO_MESSAGE) {
    /* If there's no incoming message, you can do something here, or do nothing */
  }
  else {
    /* This is an example of exiting on an error.
    ** Note that a SB read error is not always going to result in an app quitting.
    */
    CFE_EVS_SendEvent(SBN_PIPE_ERR_EID, CFE_EVS_ERROR,
                     "SBN: SB pipe read error (0x%08X), app will exit", iStatus);
    /* sbn does not have a run status var: SBN.uiRunStatus= CFE_ES_APP_ERROR; */
  }

  /* Performance Log Exit stamp */
  /* CFS_ES_PerfLogExit(SBN_MAIN_TASK_PERF_ID); */

  return iStatus;
}/* end SBN_RcvMsg */

int SBN_InitProtocol(void) {
    SBN_InitPeerVariables();
    SBN_InitPeerInterface();
    SBN_VerifyPeerInterfaces();
    SBN_VerifyHostInterfaces();
    return SBN_OK;
}/* end SBN_InitProtocol */

void SBN_InitPeerVariables(void) {
    int PeerIdx = 0, j = 0;

    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++) {
        SBN.Peer[PeerIdx].InUse = SBN_NOT_IN_USE;
        SBN.Peer[PeerIdx].SubCnt = 0;
        for (j = 0; j < SBN_MAX_SUBS_PER_PEER; j++) {
            SBN.Peer[PeerIdx].Sub[j].InUseCtr = 0;
        }/* end for */

        SBN.LocalSubCnt = 0;
    }/* end for */

}/* end SBN_InitPeerVariables */

int32 SBN_GetPeerFileData(void) {
    static char SBN_PeerData[SBN_PEER_FILE_LINE_SIZE];
    int BuffLen = 0; /* Length of the current buffer */
    int PeerFile = 0;
    char c = '\0';
    int FileOpened = FALSE;
    int LineNum = 0;

    /* First check for the file in RAM */
    PeerFile = OS_open(SBN_VOL_PEER_FILENAME, O_RDONLY, 0);
    OS_printf("SBN_VOL_PEER_FILENAME = %s\n", SBN_VOL_PEER_FILENAME);
    if (PeerFile != OS_ERROR) {
        OS_printf("thinks it opened the vol...\n");
        SBN_SendFileOpenedEvent(SBN_VOL_PEER_FILENAME);
        FileOpened = TRUE;
    }
    else {
        CFE_EVS_SendEvent(SBN_VOL_FAILED_EID, CFE_EVS_INFORMATION,
                "%s:Peer file %s failed to open", CFE_CPU_NAME,
                SBN_VOL_PEER_FILENAME);
        FileOpened = FALSE;
    }

    /* If ram file failed to open, try to open non vol file */
    if (FileOpened == FALSE) {
        PeerFile = OS_open(SBN_NONVOL_PEER_FILENAME, O_RDONLY, 0);

        if (PeerFile != OS_ERROR) {
            OS_printf("thinks it opened the nonvol...\n");
            SBN_SendFileOpenedEvent(SBN_NONVOL_PEER_FILENAME);
            FileOpened = TRUE;
        }
        else {
            CFE_EVS_SendEvent(SBN_NONVOL_FAILED_EID, CFE_EVS_ERROR,
                    "%s:Peer file %s failed to open", CFE_CPU_NAME,
                    SBN_NONVOL_PEER_FILENAME);
            FileOpened = FALSE;
        }
    }/* end if */

    /*
     ** If no file was opened, SBN must terminate
     */
    if (FileOpened == FALSE) {
        return SBN_ERROR;
    }/* end if */

    memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
    BuffLen = 0;

    /*
     ** Parse the lines from the file
     */

    while (1)
    {
        OS_read(PeerFile, &c, 1);

        if (c == '!')
            break;

        if (c == '\n' || c == ' ' || c == '\t')
        {
            /*
             ** Skip all white space in the file
             */
            ;
        }
        else if (c == ',')
        {
            /*
             ** replace the field delimiter with a space
             ** This is used for the sscanf string parsing
             */
            SBN_PeerData[BuffLen] = ' ';
            if (BuffLen < (SBN_PEER_FILE_LINE_SIZE - 1))
                BuffLen++;
        }
        else if (c != ';')
        {
            /*
             ** Regular data gets copied in
             */
            SBN_PeerData[BuffLen] = c;
            if (BuffLen < (SBN_PEER_FILE_LINE_SIZE - 1))
                BuffLen++;
        }
        else
        {
            /*
             ** Send the line to the file parser
             */
            if (SBN_ParseFileEntry(SBN_PeerData, LineNum) == SBN_ERROR){
                OS_close(PeerFile);
                return SBN_ERROR;
            }
            LineNum++;
            memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
            BuffLen = 0;
        }

    }/* end while */


    OS_close(PeerFile);

    return SBN_OK;

}/* end SBN_GetPeerFileData */

void SBN_SendFileOpenedEvent(char *Filename) {
    CFE_EVS_SendEvent(SBN_FILE_OPENED_EID, CFE_EVS_INFORMATION,
            "%s:Opened SBN Peer Data file %s", CFE_CPU_NAME, Filename);

}/* end SBN_SendFileOpenedEvent */

int SBN_CreatePipe4Peer(int PeerIdx) {
    int32 Stat = 0;
    char PipeName[OS_MAX_API_NAME];

    /* create a pipe name string similar to SBN_CPU2_Pipe */
    sprintf(PipeName, "SBN_%s_Pipe", SBN.Peer[PeerIdx].Name);
    Stat = CFE_SB_CreatePipe(&SBN.Peer[PeerIdx].Pipe, SBN_PEER_PIPE_DEPTH,
            PipeName);

    if (Stat != CFE_SUCCESS) {
        return Stat;
    }

    strncpy(SBN.Peer[PeerIdx].PipeName, PipeName, OS_MAX_API_NAME);

    CFE_EVS_SendEvent(SBN_PEERPIPE_CR_EID, CFE_EVS_INFORMATION, "%s:%s created",
            CFE_CPU_NAME, PipeName);

    return SBN_OK;
}/* end SBN_CreatePipe4Peer */

void SBN_RunProtocol(void) {
    int PeerIdx = 0;

    if(SBN.DebugOn) {
        OS_printf("SBN_RunProtocol\n");
    }

    /* The protocol is run for each peer in use. In Linux, the UDP/IP message
     * jitter/latency is enough to have 2 HB and 2 HB ack messages pending
     * With a low SBN_TIMEOUT_CYCLES value, this simple protocol can deal with
     * that for up to two consecutive cycles */

    /* The protocol engine assumes that messages have been cleared prior to the
     * first run. SBN init function does this */
    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++) {
        /* if peer data is not in use, go to next peer */
        if (SBN.Peer[PeerIdx].InUse != SBN_IN_USE)
            continue;

        if (SBN.DebugOn) {
            OS_printf("Running Protocol for %s\n", SBN.Peer[PeerIdx].Name);
        }

        strncpy(SBN.MsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);

        OS_time_t current_time;
        OS_GetLocalTime(&current_time);

        if (SBN.Peer[PeerIdx].State == SBN_ANNOUNCING) {
            if (current_time.seconds - SBN.Peer[PeerIdx].last_sent.seconds
                    > SBN_ANNOUNCE_TIMEOUT) { /* TODO: make configurable */
                SBN_SendNetMsgNoBuf(SBN_ANNOUNCE_MSG,
                        sizeof(SBN_Hdr_t), PeerIdx, NULL);
            }
            return;
        }
        if (current_time.seconds - SBN.Peer[PeerIdx].last_received.seconds
                    > SBN_HEARTBEAT_TIMEOUT) {
            /* lost connection, reset */
	    OS_printf("peer %d lost connection, resetting\n", PeerIdx);
            SBN_RemoveAllSubsFromPeer(PeerIdx);
            SBN.Peer[PeerIdx].State = SBN_ANNOUNCING;

        }
        if (current_time.seconds - SBN.Peer[PeerIdx].last_sent.seconds
                    > SBN_HEARTBEAT_SENDTIME) {
                SBN_SendNetMsgNoBuf(SBN_HEARTBEAT_MSG,
                        sizeof(SBN_Hdr_t), PeerIdx, NULL);
	}
    }
}/* end SBN_RunProtocol */

int32 SBN_PollPeerPipe(int PeerIdx, CFE_SB_MsgPtr_t *SBMsgPtr) {
    if(SBN.DebugOn) {
        OS_printf("SBN_PollPeerPipe\n");
    }
    return CFE_SB_RcvMsg((CFE_SB_Msg_t**)SBMsgPtr, SBN.Peer[PeerIdx].Pipe, CFE_SB_POLL);
}

uint16 SBN_CheckMsgSize(CFE_SB_MsgPtr_t *SBMsgPtr, int PeerIdx) {
    CFE_SB_SenderId_t *lastSenderPtr = NULL;
    uint16 AppMsgSize = 0, MaxAppMsgSize = 0;

    if(SBN.DebugOn) {
        OS_printf("SBN_CheckMsgSize\n");
    }

    /* Find size of app msg without SBN hdr */
    AppMsgSize = CFE_SB_GetTotalMsgLength(*SBMsgPtr);
    MaxAppMsgSize = (SBN_MAX_MSG_SIZE - sizeof(SBN_Hdr_t));

    CFE_SB_GetLastSenderId(&lastSenderPtr, SBN.Peer[PeerIdx].Pipe);

    if (AppMsgSize >= MaxAppMsgSize) {
        CFE_EVS_SendEvent(SBN_MSG_TRUNCATED_EID, CFE_EVS_INFORMATION,
                "%s:AppMsg 0x%04X,from %s, sz=%d destined for"
                " %s truncated to %d(max sz)",
                CFE_CPU_NAME, CFE_SB_GetMsgId(*SBMsgPtr),
                lastSenderPtr->AppName, AppMsgSize,
                SBN.Peer[PeerIdx].Name, MaxAppMsgSize);

        AppMsgSize = MaxAppMsgSize;
    }/* end if */

    return AppMsgSize;
}

void SBN_CheckPipe(int PeerIdx, int32 * priority_remaining) {
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    uint16 AppMsgSize = 0;
    uint32 NetMsgSize = 0;
    CFE_SB_SenderId_t * lastSenderPtr = NULL;
    int status = 0;

    if(SBN.DebugOn) {
        OS_printf("SBN_CheckPipe\n");
    }

    status = SBN_PollPeerPipe(PeerIdx, &SBMsgPtr);
    if (status == CFE_SUCCESS) {
        /* Pipe should be cleared if not in HEARTBEAT state */
        if (SBN.Peer[PeerIdx].State != SBN_HEARTBEATING)
            return;

        AppMsgSize = SBN_CheckMsgSize(&SBMsgPtr, PeerIdx);

        /* who sent this message */
        CFE_SB_GetLastSenderId(&lastSenderPtr, SBN.Peer[PeerIdx].Pipe);

        /* copy message from SB buffer to network data msg buffer */
        CFE_PSP_MemCpy(&SBN.MsgBuf.Pkt.Data[0], SBMsgPtr, AppMsgSize);
        strncpy((char *) &SBN.MsgBuf.Hdr.MsgSender.AppName,
            lastSenderPtr->AppName,
            sizeof(SBN.MsgBuf.Hdr.MsgSender.AppName));
        SBN.MsgBuf.Hdr.MsgSender.ProcessorId = lastSenderPtr->ProcessorId;
        strncpy(SBN.MsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);

        NetMsgSize = AppMsgSize + sizeof(SBN_Hdr_t);
        SBN.MsgBuf.Hdr.MsgSize = NetMsgSize;
        SBN_SendNetMsg(SBN_APP_MSG, NetMsgSize, PeerIdx, &SBN.MsgBuf.Hdr.MsgSender);
        (*priority_remaining)++;
    }
}

void SBN_CheckPeerPipes(void) {
    int PeerIdx = 0;
    int32 priority = 0;
    int32 priority_remaining = 0;

    if(SBN.DebugOn) {
        OS_printf("SBN_CheckPeerPipes\n");
    }

    while(priority < SBN_MAX_PEER_PRIORITY) {
        priority_remaining = 0;

        for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++) {
            /* if peer data is not in use, go to next peer */
            if (SBN.Peer[PeerIdx].State != SBN_HEARTBEATING)
                continue;

            if((int32)SBN_GetPeerQoSPriority(&SBN.Peer[PeerIdx]) != priority)
                continue;

            SBN_CheckPipe(PeerIdx, &priority_remaining);
        }
        if(!priority_remaining)
            priority++;
    }
}/* end SBN_CheckPeerPipes */

void SBN_RcvNetMsgs(void) {

}/* end SBN_RcvNetMsgs */

void SBN_ProcessNetAppMsg(int MsgLength) {
    int PeerIdx = 0;
    int status = 0;

    if(SBN.DebugOn) {
        OS_printf("SBN_ProcessNetAppMsg\n");
    }

    PeerIdx = SBN_GetPeerIndex(SBN.MsgBuf.Hdr.SrcCpuName);

    if (PeerIdx == SBN_ERROR)
        return;

    if (SBN.Peer[PeerIdx].State == SBN_ANNOUNCING || SBN.MsgBuf.Hdr.Type == SBN_ANNOUNCE_MSG) {
	OS_printf("peer #%d alive, resetting\n", PeerIdx);
        SBN.Peer[PeerIdx].State = SBN_HEARTBEATING;
        SBN_ResetPeerMsgCounts(PeerIdx);
        SBN_SendLocalSubsToPeer(PeerIdx);
    }

    switch (SBN.MsgBuf.Hdr.Type)
    {
        case SBN_ANNOUNCE_MSG:
        case SBN_ANNOUNCE_ACK_MSG:
        case SBN_HEARTBEAT_MSG:
        case SBN_HEARTBEAT_ACK_MSG:
            break;

        case SBN_APP_MSG:
            if (SBN.DebugOn)
            {
                OS_printf("%s:Recvd AppMsg 0x%04x from %s,%d bytes\n",
                        CFE_CPU_NAME,
                        CFE_SB_GetMsgId(
                                (CFE_SB_Msg_t *) &SBN.MsgBuf.Pkt.Data[0]),
                        SBN.MsgBuf.Hdr.SrcCpuName, MsgLength);
            }/* end if */

	    CFE_SB_SenderId_t sender;
	    sender.ProcessorId = SBN.MsgBuf.Hdr.MsgSender.ProcessorId;
	    strncpy(sender.AppName, SBN.MsgBuf.Hdr.MsgSender.AppName, sizeof(sender.AppName));

            status = CFE_SB_SendMsgFull(
                (CFE_SB_Msg_t *) &SBN.MsgBuf.Pkt.Data[0],
                CFE_SB_DO_NOT_INCREMENT, CFE_SB_SEND_ONECOPY);

            if (status != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(SBN_SB_SEND_ERR_EID, CFE_EVS_ERROR,
                    "%s:CFE_SB_SendMsg err %d. From %s type 0x%x",
                    CFE_CPU_NAME, status, SBN.MsgBuf.Hdr.SrcCpuName,
                    SBN.MsgBuf.Hdr.Type);
            }/* end if */
            break;

        case SBN_SUBSCRIBE_MSG:
            /*CFE_EVS_SendEvent(SBN_SUB_RCVD_EID,CFE_EVS_DEBUG,*/
            if (SBN.DebugOn)
            {
                OS_printf("%s:Sub Rcvd from %s,Msg 0x%04X\n", CFE_CPU_NAME,
                        SBN.Peer[PeerIdx].Name, ntohs(SBN.MsgBuf.Sub.MsgId));
            }/* end if */
            SBN_ProcessSubFromPeer(PeerIdx);
            break;

        case SBN_UN_SUBSCRIBE_MSG:
            /*CFE_EVS_SendEvent(SBN_UNSUB_RCVD_EID,CFE_EVS_DEBUG,*/
            if (SBN.DebugOn)
            {
                OS_printf("%s:Unsub Rcvd from %s,Msg 0x%04X\n", CFE_CPU_NAME,
                        SBN.Peer[PeerIdx].Name, ntohs(SBN.MsgBuf.Sub.MsgId));
            }/* end if */
            SBN_ProcessUnsubFromPeer(PeerIdx);
            break;

        default:
            SBN.MsgBuf.Hdr.SrcCpuName[SBN_MAX_PEERNAME_LENGTH - 1] = '\0'; /* make sure of termination */
            CFE_EVS_SendEvent(SBN_MSGTYPE_ERR_EID, CFE_EVS_ERROR,
                    "%s:Unknown Msg Type 0x%x from %s", CFE_CPU_NAME,
                    SBN.MsgBuf.Hdr.Type, SBN.MsgBuf.Hdr.SrcCpuName);
            break;
    } /* end switch */
}/* end SBN_ProcessNetAppMsg */

int32 SBN_CheckCmdPipe(void) {
    int Status = 0;

    /* Command and HK requests pipe */
    while (Status == CFE_SUCCESS) {
        Status = CFE_SB_RcvMsg(&SBN.CmdMsgPtr, SBN.CmdPipe, CFE_SB_POLL);

        if (Status == CFE_SUCCESS) {
            /* Pass Commands/HK Req to AppPipe Processing */
            SBN_AppPipe(SBN.CmdMsgPtr);
        }
    }

    if(Status == CFE_SB_NO_MESSAGE) {
        /* It's OK not to get a message */
        Status = CFE_SUCCESS;
    }

    return Status;
}/* end SBN_CheckCmdPipe */

int SBN_GetPeerIndex(char *NamePtr) {
    /* TODO: rearchitect, terribly inefficient,
        we're checking for the peer index for every packet received! */
    int PeerIdx = 0;

    if(SBN.DebugOn) {
        OS_printf("SBN_GetPeerIndex:  NamePtr = %s\n", NamePtr);
    }

    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {

        if (SBN.Peer[PeerIdx].InUse == SBN_NOT_IN_USE)
            continue;

        if (strcmp(SBN.Peer[PeerIdx].Name, NamePtr) == 0)
            return (PeerIdx);/* Found it, so stop searching */

    }/* end for */

    return SBN_ERROR;

} /* end SBN_GetPeerIndex */

void SBN_SendWakeUpDebugMsg(void) {
    if (SBN.DebugOn)
    {
        OS_printf(" awake ");
    }/* end if */
}/* end  SBN_SendWakeUpDebugMsg */

void SBN_ShowStates(void) {
    int i = 0;

    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].InUse == SBN_IN_USE)
        {
            OS_printf("%s:%s,PeerIdx=%d,State=%s\n", CFE_CPU_NAME,
                    SBN.Peer[i].Name, i, SBN_LIB_StateNum2Str(SBN.Peer[i].State));
        }/* end if */
    }/* end for */
}/* end SBN_ShowStates */

void SBN_ShowPeerData(void) {
    int i = 0;

    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].InUse == SBN_IN_USE)
        {
            switch(SBN.Peer[i].ProtocolId) {
              /*
                case SBN_IPv4:
                    SBN_ShowIPv4PeerData(i);
                    break;
              */
                default:
                    OS_printf("Unknown protocol id %d\n", SBN.Peer[i].ProtocolId);
                    break;
            }
        }/* end if */
    }/* end for */
}/* end SBN_ShowPeerData */

void SBN_NetMsgSendDbgEvt(uint32 MsgType, uint32 PeerIdx, int Status) {
    /*CFE_EVS_SendEvent(SBN_PROTO_SENT_EID,CFE_EVS_DEBUG,*/
    if (!SBN.DebugOn) {
        return;
    }

    switch (MsgType) {
        case SBN_APP_MSG:
            OS_printf("%s:Sent MsgId 0x%04X to %s sz=%d\n", CFE_CPU_NAME,
                    CFE_SB_GetMsgId(
                            (CFE_SB_MsgPtr_t) &SBN.MsgBuf.Pkt.Data[0]),
                    SBN.Peer[PeerIdx].Name, Status);
            break;

        case SBN_SUBSCRIBE_MSG:
        case SBN_UN_SUBSCRIBE_MSG:
            OS_printf("%s:%s for 0x%04X sent to %s sz=%d\n", CFE_CPU_NAME,
                    SBN_LIB_GetMsgName(MsgType),
                    CFE_SB_GetMsgId(
                            (CFE_SB_MsgPtr_t) &SBN.MsgBuf.Sub.MsgId),
                    SBN.Peer[PeerIdx].Name, Status);
            break;

        default:
            OS_printf("%s:%s sent to %s stat=%d\n", CFE_CPU_NAME,
                    SBN_LIB_GetMsgName(MsgType), SBN.Peer[PeerIdx].Name, Status);
            break;
    }/* end switch */

}/* end SBN_NetMsgSendDbgEvt */

void SBN_NetMsgSendErrEvt(uint32 MsgType, uint32 PeerIdx, int Status) {
    CFE_EVS_SendEvent(SBN_PROTO_SEND_ERR_EID, CFE_EVS_ERROR,
            "%s:Error sending %s to %s stat=%d", CFE_CPU_NAME,
            SBN_LIB_GetMsgName(MsgType), SBN.Peer[PeerIdx].Name, Status);

}/* end SBN_NetMsgSendErrEvt */

void SBN_DebugOn(void) {
    SBN.DebugOn = SBN_TRUE;
}/* end SBN_DebugOn */

void SBN_DebugOff(void) {
    SBN.DebugOn = SBN_FALSE;
}/* end SBN_DebugOff */

void SBN_ResetPeerMsgCounts(uint32 PeerIdx) {
  if(SBN.DebugOn) {
    OS_printf("[sbn_app.c : %d : %s]:\n\tresetting count of "
        "messages received from %s\n",
        __LINE__, __FUNCTION__, SBN.Peer[PeerIdx].Name);
    OS_printf("[sbn_app.c : %d : %s]:\n\tresetting count of "
        "messages sent to %s\n",
        __LINE__, __FUNCTION__, SBN.Peer[PeerIdx].Name);
    OS_printf("[sbn_app.c : %d : %s]:\n\tresetting count of "
        "messages missed from %s\n",
        __LINE__, __FUNCTION__, SBN.Peer[PeerIdx].Name);

  }

  SBN.Peer[PeerIdx].SentCount = 0;
  SBN.Peer[PeerIdx].RcvdCount = 0;
  SBN.Peer[PeerIdx].MissCount = 0;
}
