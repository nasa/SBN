/******************************************************************************
<<<<<<< HEAD
 ** File: sbn_app.c
 **
 **  Copyright © 2007-2014 United States Government as represented by the 
 **  Administrator of the National Aeronautics and Space Administration. 
 **  All Other Rights Reserved.  
 **
 **  This software was created at NASA's Goddard Space Flight Center.
 **  This software is governed by the NASA Open Source Agreement and may be 
 **  used, distributed and modified only pursuant to the terms of that 
 **  agreement.
=======
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
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
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
#include <string.h>
<<<<<<< HEAD
#include <strings.h>
=======
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30

#include "cfe.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"
#include "sbn_app.h"
#include "sbn_netif.h"
#include "sbn_msgids.h"
<<<<<<< HEAD
/* #include "app_msgids.h" */
#include <arpa/inet.h>
=======
#include "sbn_loader.h"
#include "sbn_cmds.h"
#include "sbn_buf.h"
#include "sbn_subs.h"
#include "cfe_sb_events.h" /* For event message IDs */
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30

/*
 **   Task Globals
 */
<<<<<<< HEAD
sbn_t SBN;
uint32 stopPrinting;

/* SBN Main Routine */
void SBN_AppMain(void)
{
    if (SBN_Init() == SBN_ERROR)
        return;

    /* Loop Forever */
    while (TRUE)
    {
        SBN_RcvMsg(SBN_MAIN_LOOP_DELAY);
    }/* end while */

}/* end SBN_AppMain */

int32 SBN_Init(void)
{
    int32  iStatus = CFE_SUCCESS;

    CFE_SB_CmdHdr_t SBCmdMsg;

    CFE_ES_RegisterApp();

    CFE_EVS_Register(NULL, 0, CFE_EVS_BINARY_FILTER);

    if (SBN_GetPeerFileData() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_ERR_EID, CFE_EVS_ERROR,
                "SBN APP Will Terminate, Peer File Not Found or Data Invalid!");
        return SBN_ERROR;
    }/* end if */

    if (SBN_InitProtocol() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_PROTO_INIT_ERR_EID, CFE_EVS_ERROR,
                "SBN APP Will Terminate, Error Creating Interfaces!");
        return SBN_ERROR;
    }/* end if */
=======

/** \brief SBN Main Routine */
void SBN_AppMain(void) {
    int32 Status = CFE_SUCCESS;
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
int32 SBN_Init(void) {
    int32  iStatus = CFE_SUCCESS;

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
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30

    CFE_ES_GetAppID(&SBN.AppId);

    /* Init schedule pipe */
    SBN.usSchPipeDepth = SBN_SCH_PIPE_DEPTH;
    memset((void*)SBN.cSchPipeName, '\0', sizeof(SBN.cSchPipeName));
    strncpy(SBN.cSchPipeName, "SBN_SCH_PIPE", OS_MAX_API_NAME-1);

    /* Subscribe to Wakeup messages */
    iStatus = CFE_SB_CreatePipe(&SBN.SchPipeId, SBN.usSchPipeDepth,
<<<<<<< HEAD
                                SBN.cSchPipeName);
    if (iStatus == CFE_SUCCESS)
    {
        CFE_SB_Subscribe(SBN_WAKEUP_MID, SBN.SchPipeId);
    }
    else
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
                        "SBN APP Failed to create SCH pipe (0x%08X)", iStatus);
=======
            SBN.cSchPipeName);
    if (iStatus == CFE_SUCCESS) {
        CFE_SB_Subscribe(SBN_WAKEUP_MID, SBN.SchPipeId);
    } else {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
                "SBN APP Failed to create SCH pipe (0x%08X)", iStatus);
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        return SBN_ERROR;
    }

    /* Create pipe for subscribes and unsubscribes from SB */
    CFE_SB_CreatePipe(&SBN.SubPipe, SBN_SUB_PIPE_DEPTH, "SBNSubPipe");
    CFE_SB_SubscribeLocal(CFE_SB_ALLSUBS_TLM_MID, SBN.SubPipe,
            SBN_MAX_ALLSUBS_PKTS_ON_PIPE);

    CFE_SB_SubscribeLocal(CFE_SB_ONESUB_TLM_MID, SBN.SubPipe,
            SBN_MAX_ONESUB_PKTS_ON_PIPE);

<<<<<<< HEAD
    /* Turn on SB subscription reporting */
    CFE_SB_InitMsg(&SBCmdMsg.Pri, CFE_SB_CMD_MID, sizeof(CFE_SB_CmdHdr_t),
            TRUE);
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) &SBCmdMsg,
            CFE_SB_ENABLE_SUB_REPORTING_CC);
    CFE_SB_SendMsg((CFE_SB_MsgPtr_t) &SBCmdMsg);

    /* Request a list of previous subscriptions from SB */
    CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) &SBCmdMsg, CFE_SB_SEND_PREV_SUBS_CC);
    CFE_SB_SendMsg((CFE_SB_MsgPtr_t) &SBCmdMsg);

#if 0
=======
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    /* Create pipe for HK requests and gnd commands */
    CFE_SB_CreatePipe(&SBN.CmdPipe,20,"SBNCmdPipe");
    CFE_SB_Subscribe(SBN_CMD_MID,SBN.CmdPipe);
    CFE_SB_Subscribe(SBN_SEND_HK_MID,SBN.CmdPipe);
<<<<<<< HEAD
#endif
=======
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30

    SBN.DebugOn = SBN_FALSE;

    CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
            "SBN APP Initialized V1.1, AppId=%d", SBN.AppId);

<<<<<<< HEAD
=======

    /* Initialize HK Message */
    CFE_SB_InitMsg(&SBN.HkPkt, SBN_HK_TLM_MID, sizeof(SBN_HkPacket_t), TRUE);
    SBN_InitializeCounters();

    /* Create event message pipe */
    CFE_SB_CreatePipe(&SBN.EventPipe, 20, "SBNEventPipe");

    /* Wait for event from SB saying it is initialized OR a response from SB
       to the above messages. SBN_TRUE means it needs to re-send subscription
       requests */
    if (SBN_WaitForSBStartup() == SBN_TRUE) {
        SBN_SendSubsRequests();
    }

>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    return SBN_OK;

}/* end SBN_Init */

<<<<<<< HEAD
/*=====================================================================================
** Name: SBN_RcvMsg
=======
/**
 * Waits for either a response to the "get subscriptions" message from SB, OR
 * an event message that says SB has finished initializing. The latter message
 * means that SB was not started at the time SBN sent the "get subscriptions"
 * message, so that message will need to be sent again.
 * @return SBN_TRUE if message received was a initialization message and requests
 *                  need to be sent again, or
 * @return SBN_FALSE if message received was a response
 */
int32 SBN_WaitForSBStartup() {
    CFE_EVS_Packet_t *EvsPacket;
    CFE_SB_MsgPtr_t SBMsgPtr;
    uint8 MsgFound = SBN_FALSE;
    uint8 counter = 0;

    /* Subscribe to event messages temporarily to be notified when SB is done initializing */
    CFE_SB_Subscribe(CFE_EVS_EVENT_MSG_MID, SBN.EventPipe);

    while (MsgFound == SBN_FALSE)
    {
        /* Check for subscription message from SB */
        if (SBN_CheckSubscriptionPipe() == SBN_TRUE) {
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
                if ( strcmp(EvsPacket->Payload.PacketID.AppName, "CFE_SB") == 0 ) {
                    if (EvsPacket->Payload.PacketID.EventID == CFE_SB_INIT_EID) {
                        /* Unsubscribe from event messages */
                        CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID, SBN.EventPipe);
                        MsgFound = SBN_TRUE;
                        return SBN_TRUE; /* SBN needs to re-send request messages */
                    }
                }
            }
        }

        counter++;
    }

    return SBN_TRUE; /* This code should never be reached */
}

/*=====================================================================================
** \parName: SBN_RcvMsg
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
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
<<<<<<< HEAD
int32 SBN_RcvMsg(int32 iTimeOut)
{
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
    if (iStatus == CFE_SUCCESS)
    {
        MsgId = CFE_SB_GetMsgId(MsgPtr);
        switch (MsgId)
        {
            case SBN_WAKEUP_MID:
//CFE_ES_WriteToSysLog("SBN RECEIVED SCH WAKEUP: 0x%X\n", SBN_WAKEUP_MID);
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
                                  "SBN - Recvd invalid SCH msgId (0x%08X)", MsgId);
        }
    }
    else if (iStatus == CFE_SB_TIME_OUT)
    {
//CFE_ES_WriteToSysLog("SBN TIMEOUT: MID: 0x%X\n", SBN_WAKEUP_MID);
        /* For sbn, we still want to perform cyclic processing
         * if the RcvMsg time out
         * cyclic processing at timeout rate */
=======
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
    MsgId = CFE_SB_GetMsgId(MsgPtr);
    switch (MsgId) {
      case SBN_WAKEUP_MID:
        /* cyclic processing at sch wakeup rate */
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        SBN_SendWakeUpDebugMsg();
        SBN_RunProtocol();
        SBN_CheckForNetAppMsgs();
        SBN_CheckSubscriptionPipe();
        SBN_CheckPeerPipes();
        SBN_CheckCmdPipe();
<<<<<<< HEAD
    }
    else if (iStatus == CFE_SB_NO_MESSAGE)
    {
        /* If there's no incoming message, you can do something here, or do nothing */
    }
    else
    {
        /* This is an example of exiting on an error.
        ** Note that a SB read error is not always going to result in an app quitting.
        */
        CFE_EVS_SendEvent(SBN_PIPE_ERR_EID, CFE_EVS_ERROR,
                         "SBN: SB pipe read error (0x%08X), app will exit", iStatus);
        /* sbn does not have a run status var: SBN.uiRunStatus= CFE_ES_APP_ERROR; */
    }

    /* Performance Log Exit stamp */
    /* CFS_ES_PerfLogExit(SBN_MAIN_TASK_PERF_ID); */

    return (iStatus);
}

int32 SBN_InitProtocol(void)
{
    stopPrinting = 0;
    SBN_InitPeerVariables();
    return SBN_InitPeerInterface();

}/* end SBN_InitProtocol */

void SBN_InitPeerVariables(void)
{
    int32 PeerIdx, j;

    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {
        SBN.Peer[PeerIdx].InUse = SBN_NOT_IN_USE;
        SBN.Peer[PeerIdx].SubCnt = 0;
        for (j = 0; j < SBN_MAX_SUBS_PER_PEER; j++)
        {
=======
        break;

      /* TODO:  Add code here to handle other commands, if needed */

      default:
        CFE_EVS_SendEvent(SBN_MSGID_ERR_EID, CFE_EVS_ERROR,
                          "SBN - Recvd invalid SCH msgId (0x%08X)", MsgId);
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

int32 SBN_InitProtocol(void) {
    SBN_InitPeerVariables();
    SBN_InitPeerInterface();
    SBN_VerifyPeerInterfaces();
    SBN_VerifyHostInterfaces();
    return SBN_OK;
}/* end SBN_InitProtocol */

void SBN_InitPeerVariables(void) {
    int32 PeerIdx, j;

    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++) {
        SBN.Peer[PeerIdx].InUse = SBN_NOT_IN_USE;
        SBN.Peer[PeerIdx].SubCnt = 0;
        for (j = 0; j < SBN_MAX_SUBS_PER_PEER; j++) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
            SBN.Peer[PeerIdx].Sub[j].InUseCtr = 0;
        }/* end for */

        SBN.LocalSubCnt = 0;
    }/* end for */

}/* end SBN_InitPeerVariables */

<<<<<<< HEAD
int32 SBN_GetPeerFileData(void)
{
    static char SBN_PeerData[SBN_PEER_FILE_LINE_SIZE]; /* A buffer for a line in a file */
=======
int32 SBN_GetPeerFileData(void) {
    static char SBN_PeerData[SBN_PEER_FILE_LINE_SIZE];
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    int BuffLen; /* Length of the current buffer */
    int PeerFile = 0;
    char c;
    uint32 FileOpened = FALSE;
    uint32 LineNum = 0;

    /* First check for the file in RAM */
    PeerFile = OS_open(SBN_VOL_PEER_FILENAME, O_RDONLY, 0);
<<<<<<< HEAD

    if (PeerFile != OS_ERROR)
    {
        SBN_SendFileOpenedEvent(SBN_VOL_PEER_FILENAME);
        FileOpened = TRUE;
    }
    else
    {
=======
    printf("SBN_VOL_PEER_FILENAME = %s\n", SBN_VOL_PEER_FILENAME);
    if (PeerFile != OS_ERROR) {
        printf("thinks it opened the vol...\n");
        SBN_SendFileOpenedEvent(SBN_VOL_PEER_FILENAME);
        FileOpened = TRUE;
    }
    else {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        CFE_EVS_SendEvent(SBN_VOL_FAILED_EID, CFE_EVS_INFORMATION,
                "%s:Peer file %s failed to open", CFE_CPU_NAME,
                SBN_VOL_PEER_FILENAME);
        FileOpened = FALSE;
    }

    /* If ram file failed to open, try to open non vol file */
<<<<<<< HEAD
    if (FileOpened == FALSE)
    {
        PeerFile = OS_open(SBN_NONVOL_PEER_FILENAME, O_RDONLY, 0);

        if (PeerFile != OS_ERROR)
        {
            SBN_SendFileOpenedEvent(SBN_NONVOL_PEER_FILENAME);
            FileOpened = TRUE;
        }
        else
        {
=======
    if (FileOpened == FALSE) {
        PeerFile = OS_open(SBN_NONVOL_PEER_FILENAME, O_RDONLY, 0);

        if (PeerFile != OS_ERROR) {
            printf("thinks it opened the nonvol...\n");
            SBN_SendFileOpenedEvent(SBN_NONVOL_PEER_FILENAME);
            FileOpened = TRUE;
        }
        else {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
            CFE_EVS_SendEvent(SBN_NONVOL_FAILED_EID, CFE_EVS_ERROR,
                    "%s:Peer file %s failed to open", CFE_CPU_NAME,
                    SBN_NONVOL_PEER_FILENAME);
            FileOpened = FALSE;
        }
<<<<<<< HEAD

=======
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    }/* end if */

    /*
     ** If no file was opened, SBN must terminate
     */
<<<<<<< HEAD
    if (FileOpened == FALSE)
    {
=======
    if (FileOpened == FALSE) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        return SBN_ERROR;
    }/* end if */

    memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
    BuffLen = 0;

    /*
     ** Parse the lines from the file
     */
<<<<<<< HEAD
=======

>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
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
<<<<<<< HEAD
            if (SBN_ParseFileEntry(SBN_PeerData, LineNum) == SBN_ERROR)
                return SBN_ERROR;
=======
            if (SBN_ParseFileEntry(SBN_PeerData, LineNum) == SBN_ERROR){
                OS_close(PeerFile);
                return SBN_ERROR;
            }
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
            LineNum++;
            memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
            BuffLen = 0;
        }

    }/* end while */

<<<<<<< HEAD
=======

>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    OS_close(PeerFile);

    return SBN_OK;

}/* end SBN_GetPeerFileData */

<<<<<<< HEAD
void SBN_SendFileOpenedEvent(char *Filename)
{

=======
void SBN_SendFileOpenedEvent(char *Filename) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    CFE_EVS_SendEvent(SBN_FILE_OPENED_EID, CFE_EVS_INFORMATION,
            "%s:Opened SBN Peer Data file %s", CFE_CPU_NAME, Filename);

}/* end SBN_SendFileOpenedEvent */

<<<<<<< HEAD
int32 SBN_CreatePipe4Peer(uint32 PeerIdx)
{

=======
int32 SBN_CreatePipe4Peer(uint32 PeerIdx) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    int32 Stat;
    char PipeName[OS_MAX_API_NAME];

    /* create a pipe name string similar to SBN_CPU2_Pipe */
    sprintf(PipeName, "SBN_%s_Pipe", SBN.Peer[PeerIdx].Name);
    Stat = CFE_SB_CreatePipe(&SBN.Peer[PeerIdx].Pipe, SBN_PEER_PIPE_DEPTH,
            PipeName);
<<<<<<< HEAD
    if (Stat != CFE_SUCCESS)
    {
        return Stat;
    }/* end if */
=======

    if (Stat != CFE_SUCCESS) {
        return Stat;
    }
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30

    strncpy(SBN.Peer[PeerIdx].PipeName, PipeName, OS_MAX_API_NAME);

    CFE_EVS_SendEvent(SBN_PEERPIPE_CR_EID, CFE_EVS_INFORMATION, "%s:%s created",
            CFE_CPU_NAME, PipeName);

    return SBN_OK;

}/* end SBN_CreatePipe4Peer */

<<<<<<< HEAD
void SBN_RunProtocol(void)
{

    uint32 PeerIdx;
    int32 status;
=======
void SBN_RunProtocol(void) {
    uint32 PeerIdx;
    int32 status, origDebug;
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    uint32 msgCount;
    uint32 AnnounceRcved;
    uint32 AnnounceAckRcved;
    uint32 HeartBeatRcved;
    uint32 HeartBeatAckRcved;
    uint32 UnkownRcved;

<<<<<<< HEAD
    /* The protocol is run for each peer in use. */
    /* In Linux, the UDP/IP message jitter/latency is enough to have 2 HB and 2 HB ack messages pending */
    /* With a low SBN_TIMEOUT_CYCLES value, this simple protocol can deal with that for up to
     /* two consecutive cycles */

    /* The protocol engine assumes that messages have been cleared prior to the first run. SBN init function does this */
    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {

=======
    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_RunProtocol\n");
    }

    /* The protocol is run for each peer in use. In Linux, the UDP/IP message
     * jitter/latency is enough to have 2 HB and 2 HB ack messages pending
     * With a low SBN_TIMEOUT_CYCLES value, this simple protocol can deal with
     * that for up to two consecutive cycles */

    /* The protocol engine assumes that messages have been cleared prior to the
     * first run. SBN init function does this */
    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        /* if peer data is not in use, go to next peer */
        if (SBN.Peer[PeerIdx].InUse != SBN_IN_USE)
            continue;

<<<<<<< HEAD
        if (SBN.DebugOn == SBN_TRUE)
        {
            OS_printf("Running Protocol for %s\n", SBN.Peer[PeerIdx].Name);
        }/* end if */
=======
        if (SBN.DebugOn == SBN_TRUE) {
            OS_printf("Running Protocol for %s\n", SBN.Peer[PeerIdx].Name);
        }
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30

        AnnounceRcved = 0;
        AnnounceAckRcved = 0;
        HeartBeatRcved = 0;
        HeartBeatAckRcved = 0;
        UnkownRcved = 0;
        msgCount = 0;

        /* Read all messages prior to executing the protocol state machine */
<<<<<<< HEAD
        while (msgCount < 6)
        {
            status = SBN_CheckForNetProtoMsg(PeerIdx);

            if (status <= SBN_NO_MSG) /* Assumes 0 or negative for no msg or errors on read */
                break;

            msgCount++;
            /* the order is optimized for normal operations */
	    switch(ntohl(SBN.ProtoMsgBuf.Hdr.Type)) {
		case SBN_HEARTBEAT_MSG:
		    HeartBeatRcved++;
		    break;
		case SBN_HEARTBEAT_ACK_MSG:
		    HeartBeatAckRcved++;
		    break;
		case SBN_ANNOUNCE_MSG:
		    AnnounceRcved++;
		    break;
		case SBN_ANNOUNCE_ACK_MSG:
		    AnnounceAckRcved++;
		    break;
		default:
		    UnkownRcved++; /* Currently not used, but left in for debugging */
		    break;
	    } /* end switch */
        } /* end while */

        switch (SBN.Peer[PeerIdx].State)
        {

            /*--------------------------------------------------------------------------*/
            /* This is the peer discovery state. It will transtition to SBN_HEARTBEATING when
             an SBN_ANNOUNCE_ACK_MSG has been received
             */
            case SBN_ANNOUNCING:
            {
                if (msgCount > 0)
                { /* at least one message */

                    if (AnnounceRcved > 0)
                        SBN_SendNetMsg(SBN_ANNOUNCE_ACK_MSG,
                                sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL );

                    if (AnnounceAckRcved > 0)
                    {
                        SBN.Peer[PeerIdx].State = SBN_HEARTBEATING;
                        SBN_SendNetMsg(SBN_HEARTBEAT_MSG,
                                sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL );

                        CFE_EVS_SendEvent(SBN_PEER_ALIVE_EID,
                                CFE_EVS_INFORMATION,
                                "%s:%s Alive, changing state to SBN_HEARTBEATING",
                                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name);
                    }
                    else
                        /* no ack received, so keep announcing */
                        SBN_SendNetMsg(SBN_ANNOUNCE_MSG,
                                sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL );

                    /* ignore if (HeartBeatAckRcved > 0) */
                    /* ignore if (HeartBeatRcved > 0) */
                }
                else
                    /* No messages, so keep announcing  */
                    SBN_SendNetMsg(SBN_ANNOUNCE_MSG, sizeof(SBN_NetProtoMsg_t),
                            PeerIdx, NULL );

            }
                break; /* SBN_ANNOUNCING */

                /*--------------------------------------------------------------------------*/
            case SBN_HEARTBEATING:
            {
                if (msgCount > 0)
                {
                    if (HeartBeatRcved > 0)
                        SBN_SendNetMsg(SBN_HEARTBEAT_ACK_MSG,
                                sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL );

                    if (HeartBeatAckRcved > 0)
                    {
                        SBN.Peer[PeerIdx].Timer = 0;
                        if (SBN.Peer[PeerIdx].SentLocalSubs == SBN_FALSE)
                        { /* Send on first ack message only */
                            SBN_SendLocalSubsToPeer(PeerIdx);
                            SBN.Peer[PeerIdx].SentLocalSubs = SBN_TRUE;
                        }
                    }
                    else
                        SBN.Peer[PeerIdx].Timer++;

                    /* ignore if (AnnounceRcved > 0) */
                    /* ignore if (AnnounceRcved > 0) */

                } /* end msgCount > 0 */
                else
                    SBN.Peer[PeerIdx].Timer++;

                if (SBN.Peer[PeerIdx].Timer >= SBN_TIMEOUT_CYCLES)
                {
                    SBN.Peer[PeerIdx].State = SBN_ANNOUNCING;
                    SBN_SendNetMsg(SBN_ANNOUNCE_MSG, sizeof(SBN_NetProtoMsg_t),
                            PeerIdx, NULL );
                    CFE_EVS_SendEvent(SBN_HB_LOST_EID, CFE_EVS_INFORMATION,
                            "%s:%s Heartbeat lost, changing state to SBN_ANNOUNCING",
                            CFE_CPU_NAME, SBN.Peer[PeerIdx].Name);
                    /* unsubscribe to all subscriptions from that peer and reset timer */
                    SBN_RemoveAllSubsFromPeer(PeerIdx);
                    SBN.Peer[PeerIdx].SentLocalSubs = SBN_FALSE;
                    SBN.Peer[PeerIdx].Timer = 0;
                }
                else /* No timeout, so keep sending heart beats */
                {
                    SBN_SendNetMsg(SBN_HEARTBEAT_MSG, sizeof(SBN_NetProtoMsg_t),
                            PeerIdx, NULL );
                }

            }
                break; /* SBN_HEARTBEATING */

            default:
                CFE_EVS_SendEvent(SBN_STATE_ERR_EID, CFE_EVS_ERROR,
                        "%s:Unexpected state(%d) in SBN_RunProtocol for %s",
                        CFE_CPU_NAME, SBN.Peer[PeerIdx].State,
                        SBN.Peer[PeerIdx].Name);

        }/* end .State switch */

    }/* end PeerIdx for */

}/* end SBN_RunProtocol */

void SBN_CheckPeerPipes(void)
{

    CFE_SB_MsgPtr_t SBMsgPtr;
    uint16 AppMsgSize;
    uint16 MaxAppMsgSize;
    uint32 NetMsgSize;
    uint32 PeerIdx;
    CFE_SB_SenderId_t * lastSenderPtr;

    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {

        /* if peer data is not in use, go to next peer */
        if (SBN.Peer[PeerIdx].State != SBN_HEARTBEATING)
            continue;

        while (CFE_SB_RcvMsg(&SBMsgPtr, SBN.Peer[PeerIdx].Pipe, CFE_SB_POLL)
                == CFE_SUCCESS)
        {

            /* Pipe should be cleared if not in HEARTBEAT state */
            if (SBN.Peer[PeerIdx].State != SBN_HEARTBEATING)
                continue;

            /* who sent this message */
            CFE_SB_GetLastSenderId(&lastSenderPtr, SBN.Peer[PeerIdx].Pipe);

            /* Find size of app msg without SBN hdr */
            AppMsgSize = CFE_SB_GetTotalMsgLength(SBMsgPtr);
            MaxAppMsgSize = (SBN_MAX_MSG_SIZE - sizeof(SBN_Hdr_t));

            if (AppMsgSize >= MaxAppMsgSize)
            {
                CFE_EVS_SendEvent(SBN_MSG_TRUNCATED_EID, CFE_EVS_INFORMATION,
                        "%s:AppMsg 0x%04X,from %s, sz=%d destined for %s truncated to %d(max sz)",
                        CFE_CPU_NAME, CFE_SB_GetMsgId(SBMsgPtr),
                        lastSenderPtr->AppName, AppMsgSize,
                        SBN.Peer[PeerIdx].Name, MaxAppMsgSize);
                AppMsgSize = MaxAppMsgSize;
            }/* end if */

            /* copy message from SB buffer to network data msg buffer */
            CFE_PSP_MemCpy(&SBN.DataMsgBuf.Pkt.Data[0], SBMsgPtr, AppMsgSize);
            strncpy((char *) &SBN.DataMsgBuf.Hdr.MsgSender.AppName,
                    lastSenderPtr->AppName, OS_MAX_API_NAME);

            NetMsgSize = AppMsgSize + sizeof(SBN_Hdr_t);

            SBN_SendNetMsg(SBN_APP_MSG, NetMsgSize, PeerIdx, lastSenderPtr);

        }/* end while */

    }/* end for */

}/* end SBN_CheckPeerPipes */

void SBN_SendLocalSubsToPeer(uint32 PeerIdx)
{

    uint32 i;

    if (SBN.LocalSubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_LSC_ERR1_EID, CFE_EVS_ERROR,
                "%s:Error sending subs to %s,LclSubCnt=%d,max=%d", CFE_CPU_NAME,
                SBN.Peer[PeerIdx].Name, SBN.LocalSubCnt, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    for (i = 0; i < SBN.LocalSubCnt; i++)
    {

        /* Load the Protocol Buffer with the subscription details */
        SBN.DataMsgBuf.Sub.MsgId = htons(SBN.LocalSubs[i].MsgId);
        SBN.DataMsgBuf.Sub.Qos = SBN.LocalSubs[i].Qos;

        SBN_SendNetMsg(SBN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), PeerIdx, NULL );

    }/* end for */

}/* end SBN_SendLocalSubsToPeer */

void SBN_CheckSubscriptionPipe(void)
{

    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_SubRprtMsg_t *SubRprtMsgPtr;
    CFE_SB_SubEntries_t SubEntry;
    CFE_SB_SubEntries_t *SubEntryPtr = &SubEntry;

    while (CFE_SB_RcvMsg(&SBMsgPtr, SBN.SubPipe, CFE_SB_POLL) == CFE_SUCCESS)
    {

        switch (CFE_SB_GetMsgId(SBMsgPtr))
        {

            case CFE_SB_ONESUB_TLM_MID:
                SubRprtMsgPtr = (CFE_SB_SubRprtMsg_t *) SBMsgPtr;
                SubEntry.MsgId = SubRprtMsgPtr->MsgId;
                SubEntry.Qos = SubRprtMsgPtr->Qos;
                SubEntry.Pipe = SubRprtMsgPtr->Pipe;
                if (SubRprtMsgPtr->SubType == CFE_SB_SUBSCRIPTION)
                    SBN_ProcessLocalSub(SubEntryPtr);
                else if (SubRprtMsgPtr->SubType == CFE_SB_UNSUBSCRIPTION)
                    SBN_ProcessLocalUnsub(SubEntryPtr);
                else
                    CFE_EVS_SendEvent(SBN_SUBTYPE_ERR_EID, CFE_EVS_ERROR,
                            "%s:Unexpected SubType %d in SBN_CheckSubscriptionPipe",
                            CFE_CPU_NAME, SubRprtMsgPtr->SubType);
                break;

            case CFE_SB_ALLSUBS_TLM_MID:
                SBN_ProcessAllSubscriptions((CFE_SB_PrevSubMsg_t *) SBMsgPtr);
                break;

            default:
                CFE_EVS_SendEvent(SBN_MSGID_ERR_EID, CFE_EVS_ERROR,
                        "%s:Unexpected MsgId 0x%0x on SBN.SubPipe",
                        CFE_CPU_NAME, CFE_SB_GetMsgId(SBMsgPtr));

        }/* end switch */

    }/* end while */

}/* end SBN_CheckSubscriptionPipe */

void SBN_RcvNetMsgs(void)
{

}/* end SBN_RcvNetMsgs */

void SBN_ProcessSubFromPeer(uint32 PeerIdx)
{

    uint32 FirstOpenSlot;
    uint32 idx;

    if (PeerIdx >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR1_EID, CFE_EVS_ERROR,
                "%s:Cannot process Subscription,PeerIdx(%d)OutOfRange",
                CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    if (SBN.Peer[PeerIdx].SubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_SUB_ERR_EID, CFE_EVS_ERROR,
                "%s:Cannot process subscription from %s,max(%d)met.",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if msg id already in the list, ignore */
    if (SBN_CheckPeerSubs4MsgId(&idx, ntohs(SBN.DataMsgBuf.Sub.MsgId),
            PeerIdx) == SBN_MSGID_FOUND)
    {
        CFE_EVS_SendEvent(SBN_DUP_SUB_EID, CFE_EVS_INFORMATION,
                "%s:subscription from %s ignored,msg 0x%04X already logged",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, ntohs(SBN.DataMsgBuf.Sub.MsgId));
        return;
    }/* end if */

    /* SubscibeLocal suppresses the subscribption report */
    CFE_SB_SubscribeLocal(ntohs(SBN.DataMsgBuf.Sub.MsgId), SBN.Peer[PeerIdx].Pipe,
            SBN_DEFAULT_MSG_LIM);

    FirstOpenSlot = SBN.Peer[PeerIdx].SubCnt;

    /* log the subscription in the peer table */
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].MsgId = ntohs(SBN.DataMsgBuf.Sub.MsgId);
    SBN.Peer[PeerIdx].Sub[FirstOpenSlot].Qos = SBN.DataMsgBuf.Sub.Qos;

    SBN.Peer[PeerIdx].SubCnt++;

}/* SBN_ProcessSubFromPeer */

void SBN_ProcessUnsubFromPeer(uint32 PeerIdx)
{

    uint32 i;
    uint32 idx;

    if (PeerIdx >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR2_EID, CFE_EVS_ERROR,
                "%s:Cannot process unsubscription,PeerIdx(%d)OutOfRange",
                CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    if (SBN.Peer[PeerIdx].SubCnt == 0)
    {
        CFE_EVS_SendEvent(SBN_NO_SUBS_EID, CFE_EVS_INFORMATION,
                "%s:Cannot process unsubscription from %s,none logged",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name);
        return;
    }/* end if */

    if (SBN_CheckPeerSubs4MsgId(&idx, ntohs(SBN.DataMsgBuf.Sub.MsgId),
            PeerIdx)==SBN_MSGID_NOT_FOUND)
    {
        CFE_EVS_SendEvent(SBN_SUB_NOT_FOUND_EID, CFE_EVS_INFORMATION,
                "%s:Cannot process unsubscription from %s,msg 0x%04X not found",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].Name, ntohs(SBN.DataMsgBuf.Sub.MsgId));
        return;
    }/* end if */

    /* remove sub from array for that peer and ... */
    /* shift all subscriptions in higher elements to fill the gap */
    for (i = idx; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        CFE_PSP_MemCpy(&SBN.Peer[PeerIdx].Sub[i], &SBN.Peer[PeerIdx].Sub[i + 1],
                sizeof(SBN_Subs_t));
    }/* end for */

    /* decrement sub cnt */
    SBN.Peer[PeerIdx].SubCnt--;

    /* unsubscribe to the msg id on the peer pipe */
    CFE_SB_UnsubscribeLocal(ntohs(SBN.DataMsgBuf.Sub.MsgId), SBN.Peer[PeerIdx].Pipe);

}/* SBN_ProcessUnsubFromPeer */

void SBN_ProcessNetAppMsg(int MsgLength)
{
    uint8 PeerIdx;
    int32 status;

    PeerIdx = SBN_GetPeerIndex(SBN.DataMsgBuf.Hdr.SrcCpuName);

    if (PeerIdx >= SBN_MAX_NETWORK_PEERS)
        return;

    switch (ntohl(SBN.DataMsgBuf.Hdr.Type))
=======
        while (msgCount < 6) {
            status = SBN_CheckForNetProtoMsg(PeerIdx);
            /* Assumes 0 or negative for no msg or errors on read */
            if (status <= SBN_NO_MSG)
                break;
            msgCount++;
            
            /* the order is optimized for normal operations */
            if (SBN.ProtoMsgBuf.Hdr.Type == SBN_HEARTBEAT_MSG) {
                HeartBeatRcved++;
            }
            else if (SBN.ProtoMsgBuf.Hdr.Type == SBN_HEARTBEAT_ACK_MSG) {
                HeartBeatAckRcved++;
            }
            else if (SBN.ProtoMsgBuf.Hdr.Type == SBN_ANNOUNCE_MSG) {
                AnnounceRcved++;
            }
            else if (SBN.ProtoMsgBuf.Hdr.Type == SBN_ANNOUNCE_ACK_MSG) {
                AnnounceAckRcved++;
            }
            else {
                UnkownRcved++; /* Currently not used, but left for debugging */
            }
        } /* end while */

        strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);

        switch (SBN.Peer[PeerIdx].State) {
          /*
           * This is the peer discovery state. It will transtition to
           * SBN_HEARTBEATING when an SBN_ANNOUNCE_ACK_MSG has been received
           */
          case SBN_ANNOUNCING: {
            if (msgCount > 0) { /* at least one message */
              if (AnnounceRcved > 0) {
                SBN_SendNetMsg(SBN_ANNOUNCE_ACK_MSG,
                        sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL, SBN_FALSE);
              }

              if (AnnounceAckRcved > 0) {
                SBN.Peer[PeerIdx].State = SBN_HEARTBEATING;


                origDebug = SBN.DebugOn;
                SBN.DebugOn = SBN_TRUE;
                SBN_ResetPeerMsgCounts(PeerIdx);
                SBN.DebugOn = origDebug;

                SBN_SendNetMsg(SBN_HEARTBEAT_MSG,
                        sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL, SBN_FALSE);

                CFE_EVS_SendEvent(SBN_PEER_ALIVE_EID,
                        CFE_EVS_INFORMATION,
                        "%s:%s Alive, changing state to SBN_HEARTBEATING ",
                        CFE_CPU_NAME, SBN.Peer[PeerIdx].Name);


              }
              else {
                  /* no ack received, so keep announcing */
                  SBN_SendNetMsg(SBN_ANNOUNCE_MSG,
                          sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL, SBN_FALSE);
              }

              /* ignore if (HeartBeatAckRcved > 0) */
              /* ignore if (HeartBeatRcved > 0) */
            }
            else {
                /* No messages, so keep announcing  */
                SBN_SendNetMsg(SBN_ANNOUNCE_MSG, sizeof(SBN_NetProtoMsg_t),
                        PeerIdx, NULL, SBN_FALSE);
            }
            break;
          } /* SBN_ANNOUNCING */
          case SBN_HEARTBEATING: {
            if (msgCount > 0) {
              if (HeartBeatRcved > 0) {
                SBN_SendNetMsg(SBN_HEARTBEAT_ACK_MSG,
                    sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL, SBN_FALSE);
              }

              if (HeartBeatAckRcved > 0) {
                SBN.Peer[PeerIdx].Timer = 0;
                if (SBN.Peer[PeerIdx].SentLocalSubs == SBN_FALSE) {
                  /* Send on first ack message only */
                  SBN_SendLocalSubsToPeer(PeerIdx);
                  SBN.Peer[PeerIdx].SentLocalSubs = SBN_TRUE;
                }
              } else {
                SBN.Peer[PeerIdx].Timer++;
              }
              /* ignore if (AnnounceRcved > 0) */
              /* ignore if (AnnounceRcved > 0) */
            } else {
              SBN.Peer[PeerIdx].Timer++;
            }

            if (SBN.Peer[PeerIdx].Timer >= SBN_TIMEOUT_CYCLES) {
              SBN.Peer[PeerIdx].State = SBN_ANNOUNCING;
              SBN_SendNetMsg(SBN_ANNOUNCE_MSG, sizeof(SBN_NetProtoMsg_t),
                  PeerIdx, NULL, SBN_FALSE);
              CFE_EVS_SendEvent(SBN_HB_LOST_EID, CFE_EVS_INFORMATION,
                  "%s:%s Heartbeat lost, changing state to SBN_ANNOUNCING",
                  CFE_CPU_NAME, SBN.Peer[PeerIdx].Name);
              /* unsubscribe to all subscriptions from that peer and reset timer */
              SBN_RemoveAllSubsFromPeer(PeerIdx);
              SBN.Peer[PeerIdx].SentLocalSubs = SBN_FALSE;
              SBN.Peer[PeerIdx].Timer = 0;
            }
            else {
              /* No timeout, so keep sending heart beats */
              SBN_SendNetMsg(SBN_HEARTBEAT_MSG, sizeof(SBN_NetProtoMsg_t),
                  PeerIdx, NULL, SBN_FALSE);
            }
            break;
          } /* SBN_HEARTBEATING */
          default:
            CFE_EVS_SendEvent(SBN_STATE_ERR_EID, CFE_EVS_ERROR,
                "%s:Unexpected state(%d) in SBN_RunProtocol for %s",
                CFE_CPU_NAME, SBN.Peer[PeerIdx].State,
                SBN.Peer[PeerIdx].Name);
        }/* end .State switch */
    }/* end PeerIdx for */
}/* end SBN_RunProtocol */

int32 SBN_PollPeerPipe(uint32 PeerIdx, CFE_SB_MsgPtr_t *SBMsgPtr) {
    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_PollPeerPipe\n");
    }
    return CFE_SB_RcvMsg((CFE_SB_Msg_t**)SBMsgPtr, SBN.Peer[PeerIdx].Pipe, CFE_SB_POLL);
}

uint16 SBN_CheckMsgSize(CFE_SB_MsgPtr_t *SBMsgPtr, uint32 PeerIdx) {
    CFE_SB_SenderId_t *lastSenderPtr;
    uint16 AppMsgSize, MaxAppMsgSize;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckMsgSize\n");
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

void SBN_CheckPipe(uint32 PeerIdx, int32 * priority_remaining) {
    CFE_SB_MsgPtr_t SBMsgPtr;
    uint16 AppMsgSize;
    uint32 NetMsgSize;
    CFE_SB_SenderId_t * lastSenderPtr;
    int32 status;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckPipe\n");
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
        CFE_PSP_MemCpy(&SBN.DataMsgBuf.Pkt.Data[0], SBMsgPtr, AppMsgSize);
        strncpy((char *) &SBN.DataMsgBuf.Hdr.MsgSender.AppName,
                lastSenderPtr->AppName, OS_MAX_API_NAME * 2);
        strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);

        NetMsgSize = AppMsgSize + sizeof(SBN_Hdr_t);
        SBN.DataMsgBuf.Hdr.MsgSize = NetMsgSize;
        SBN_SendNetMsg(SBN_APP_MSG, NetMsgSize, PeerIdx, lastSenderPtr, SBN_FALSE);
        (*priority_remaining)++;
    }
}

void SBN_CheckPeerPipes(void) {
    uint32 PeerIdx;
    int32 priority = 0;
    int32 priority_remaining = 0;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckPeerPipes\n");
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
    int32 PeerIdx;
    int32 status;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_ProcessNetAppMsg\n");
    }

    PeerIdx = SBN_GetPeerIndex(SBN.DataMsgBuf.Hdr.SrcCpuName);

    printf("SBN_ProcessNetAppMsg:  PeerIdx = %d, Type = %x\n", PeerIdx, SBN.DataMsgBuf.Hdr.Type);

    if (PeerIdx == SBN_ERROR)
        return;

    switch (SBN.DataMsgBuf.Hdr.Type)
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    {

        case SBN_APP_MSG:
            if (SBN.DebugOn == SBN_TRUE)
            {
                OS_printf("%s:Recvd AppMsg 0x%04x from %s,%d bytes\n",
                        CFE_CPU_NAME,
                        CFE_SB_GetMsgId(
                                (CFE_SB_Msg_t *) &SBN.DataMsgBuf.Pkt.Data[0]),
                        SBN.DataMsgBuf.Hdr.SrcCpuName, MsgLength);
            }/* end if */

            if (SBN.Peer[PeerIdx].State == SBN_HEARTBEATING)
            {
<<<<<<< HEAD
		CFE_SB_SenderId_t msgsender;
		msgsender.ProcessorId = SBN.DataMsgBuf.Hdr.MsgSender.ProcessorId;
		strncpy(msgsender.AppName, SBN.DataMsgBuf.Hdr.MsgSender.AppName, OS_MAX_API_NAME);

                status = CFE_SB_SendMsgFull(
                        (CFE_SB_Msg_t *) &SBN.DataMsgBuf.Pkt.Data[0],
                        0, 1,
                        &msgsender);
=======
                CFE_SB_SenderId_t sender;

                sender.ProcessorId = SBN.DataMsgBuf.Hdr.MsgSender.ProcessorId;
                strncpy(sender.AppName, SBN.DataMsgBuf.Hdr.MsgSender.AppName, sizeof(sender.AppName));

                status = CFE_SB_SendMsgFull(
                        (CFE_SB_Msg_t *) &SBN.DataMsgBuf.Pkt.Data[0],
                        CFE_SB_DO_NOT_INCREMENT, CFE_SB_SEND_ONECOPY,
                        &sender);
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30

                if (status != CFE_SUCCESS)
                {
                    CFE_EVS_SendEvent(SBN_SB_SEND_ERR_EID, CFE_EVS_ERROR,
                            "%s:CFE_SB_SendMsg err %d. From %s type 0x%x",
                            CFE_CPU_NAME, status, SBN.DataMsgBuf.Hdr.SrcCpuName,
<<<<<<< HEAD
                            ntohl(SBN.DataMsgBuf.Hdr.Type));
=======
                            SBN.DataMsgBuf.Hdr.Type);
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
                }/* end if */
            }/* end if */
            break;

        case SBN_SUBSCRIBE_MSG:
            /*CFE_EVS_SendEvent(SBN_SUB_RCVD_EID,CFE_EVS_DEBUG,*/
            if (SBN.DebugOn == SBN_TRUE)
            {
                OS_printf("%s:Sub Rcvd from %s,Msg 0x%04X\n", CFE_CPU_NAME,
<<<<<<< HEAD
                        SBN.Peer[PeerIdx].Name, ntohs(SBN.DataMsgBuf.Sub.MsgId));
=======
                        SBN.Peer[PeerIdx].Name, SBN.DataMsgBuf.Sub.MsgId);
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
            }/* end if */
            SBN_ProcessSubFromPeer(PeerIdx);
            break;

        case SBN_UN_SUBSCRIBE_MSG:
            /*CFE_EVS_SendEvent(SBN_UNSUB_RCVD_EID,CFE_EVS_DEBUG,*/
            if (SBN.DebugOn == SBN_TRUE)
            {
                OS_printf("%s:Unsub Rcvd from %s,Msg 0x%04X\n", CFE_CPU_NAME,
<<<<<<< HEAD
                        SBN.Peer[PeerIdx].Name, ntohs(SBN.DataMsgBuf.Sub.MsgId));
=======
                        SBN.Peer[PeerIdx].Name, SBN.DataMsgBuf.Sub.MsgId);
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
            }/* end if */
            SBN_ProcessUnsubFromPeer(PeerIdx);
            break;

        default:
            SBN.DataMsgBuf.Hdr.SrcCpuName[SBN_MAX_PEERNAME_LENGTH - 1] = '\0'; /* make sure of termination */
            CFE_EVS_SendEvent(SBN_MSGTYPE_ERR_EID, CFE_EVS_ERROR,
                    "%s:Unknown Msg Type 0x%x from %s", CFE_CPU_NAME,
<<<<<<< HEAD
                    ntohl(SBN.DataMsgBuf.Hdr.Type), SBN.DataMsgBuf.Hdr.SrcCpuName);
=======
                    SBN.DataMsgBuf.Hdr.Type, SBN.DataMsgBuf.Hdr.SrcCpuName);
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
            break;
    } /* end switch */
}/* end SBN_ProcessNetAppMsg */

<<<<<<< HEAD
void SBN_CheckCmdPipe(void)
{

}/* end SBN_CheckCmdPipe */

void SBN_ProcessLocalSub(CFE_SB_SubEntries_t *Ptr)
{
    uint32 i;
    uint32 index;

    if (SBN.LocalSubCnt >= SBN_MAX_SUBS_PER_PEER)
    {
        CFE_EVS_SendEvent(SBN_LSC_ERR2_EID, CFE_EVS_ERROR,
                "%s:Local Subscription Ignored for MsgId 0x%04x,max(%d)met",
                CFE_CPU_NAME, Ptr->MsgId, SBN_MAX_SUBS_PER_PEER);
        return;
    }/* end if */

    /* if there is already an entry for this msg id,just incr InUseCtr */
    if (SBN_CheckLocSubs4MsgId(&index, Ptr->MsgId) == SBN_MSGID_FOUND)
    {
        /* check index before writing */
        if (index < SBN.LocalSubCnt)
        {
            SBN.LocalSubs[index].InUseCtr++;
        }/* end if */
        return;
    }/* end if */

    /* log new entry into LocalSubs array */
    SBN.LocalSubs[SBN.LocalSubCnt].InUseCtr = 1;
    SBN.LocalSubs[SBN.LocalSubCnt].MsgId = Ptr->MsgId;
    SBN.LocalSubs[SBN.LocalSubCnt].Qos = Ptr->Qos;
    SBN.LocalSubCnt++;

    /* initialize the network msg */
    SBN.DataMsgBuf.Hdr.Type = htonl(SBN_SUBSCRIBE_MSG);
    strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);
    SBN.DataMsgBuf.Sub.MsgId = htons(Ptr->MsgId);
    SBN.DataMsgBuf.Sub.Qos = Ptr->Qos;

    /* send subscription to all peers if peer state is heartbeating */
    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].State == SBN_HEARTBEATING)
        {
            SBN_SendNetMsg(SBN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), i, NULL );
        }/* end if */
    }/* end for */

}/* end SBN_ProcessLocalSub */

void SBN_ProcessLocalUnsub(CFE_SB_SubEntries_t *Ptr)
{
    uint32 i;
    uint32 index;

    /* find index of matching subscription */
    if (SBN_CheckLocSubs4MsgId(&index, Ptr->MsgId) == SBN_MSGID_NOT_FOUND)
        return;

    SBN.LocalSubs[index].InUseCtr--;

    /* do not modify the array and tell peers until the # of local subscriptions = 0 */
    if (SBN.LocalSubs[index].InUseCtr != 0)
        return;

    /* move array up one */
    for (i = index; i < SBN.LocalSubCnt; i++)
    {
        SBN.LocalSubs[i].InUseCtr = SBN.LocalSubs[i + 1].InUseCtr;
        SBN.LocalSubs[i].MsgId = SBN.LocalSubs[i + 1].MsgId;
        SBN.LocalSubs[i].Qos = SBN.LocalSubs[i + 1].Qos;
    }/* end for */

    SBN.LocalSubCnt--;

    /* initialize the network msg */
    SBN.DataMsgBuf.Hdr.Type = htonl(SBN_UN_SUBSCRIBE_MSG);
    strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
            SBN_MAX_PEERNAME_LENGTH);
    SBN.DataMsgBuf.Sub.MsgId = htons(Ptr->MsgId);
    SBN.DataMsgBuf.Sub.Qos = Ptr->Qos;

    /* send unsubscription to all peers if peer state is heartbeating and */
    /* only if no more local subs (InUseCtr = 0)  */
    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].State == SBN_HEARTBEATING)
        {
            SBN_SendNetMsg(SBN_UN_SUBSCRIBE_MSG, sizeof(SBN_NetSub_t), i,
                    NULL );
        }/* end if */
    }/* end for */

}/* end SBN_ProcessLocalUnsub */

void SBN_ProcessAllSubscriptions(CFE_SB_PrevSubMsg_t *Ptr)
{
    uint32 i;
    CFE_SB_SubEntries_t SubEntry;
    CFE_SB_SubEntries_t *SubEntryPtr = &SubEntry;

    if (Ptr->Entries > CFE_SB_SUB_ENTRIES_PER_PKT)
    {
        CFE_EVS_SendEvent(SBN_ENTRY_ERR_EID, CFE_EVS_ERROR,
                "%s:Entries value %d in SB PrevSubMsg exceeds max %d, aborting",
                CFE_CPU_NAME, Ptr->Entries, CFE_SB_SUB_ENTRIES_PER_PKT);
        return;
    }/* end if */

    for (i = 0; i < Ptr->Entries; i++)
    {
        SubEntry.MsgId = Ptr->Entry[i].MsgId;
        SubEntry.Pipe = Ptr->Entry[i].Pipe;
        SubEntry.Qos = Ptr->Entry[i].Qos;

        SBN_ProcessLocalSub(SubEntryPtr);
    }/* end for */

}/* end SBN_ProcessAllSubscriptions */

int32 SBN_CheckLocSubs4MsgId(uint32 *IdxPtr, CFE_SB_MsgId_t MsgId)
{
    uint32 i;

    for (i = 0; i < SBN.LocalSubCnt; i++)
    {
        if (SBN.LocalSubs[i].MsgId == MsgId)
        {
            *IdxPtr = i;
            return SBN_MSGID_FOUND;
        }/* end if */
    }/* end for */

    return SBN_MSGID_NOT_FOUND;

}/* end SBN_CheckLocSubs4MsgId */

int32 SBN_CheckPeerSubs4MsgId(uint32 *SubIdxPtr, CFE_SB_MsgId_t MsgId,
        uint32 PeerIdx)
{
    uint32 i;

    for (i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        if (SBN.Peer[PeerIdx].Sub[i].MsgId == MsgId)
        {
            *SubIdxPtr = i;
            return SBN_MSGID_FOUND;
        }/* end if */
    }/* end for */

    return SBN_MSGID_NOT_FOUND;

}/* end SBN_CheckPeerSubs4MsgId */

void SBN_RemoveAllSubsFromPeer(uint32 PeerIdx)
{
    uint32 i;

    if (PeerIdx >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_PEERIDX_ERR3_EID, CFE_EVS_ERROR,
                "%s:Cannot remove all subs from peer,PeerIdx(%d)OutOfRange",
                CFE_CPU_NAME, PeerIdx);
        return;
    }/* end if */

    for (i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        CFE_SB_UnsubscribeLocal(SBN.Peer[PeerIdx].Sub[i].MsgId,
                SBN.Peer[PeerIdx].Pipe);
    }/* end for */

    CFE_EVS_SendEvent(SBN_UNSUB_CNT_EID, CFE_EVS_INFORMATION,
            "%s:UnSubscribed %d MsgIds from %s", CFE_CPU_NAME,
            SBN.Peer[PeerIdx].SubCnt, SBN.Peer[PeerIdx].Name);

    SBN.Peer[PeerIdx].SubCnt = 0;

}/* end SBN_RemoveAllSubsFromPeer */

int32 SBN_GetPeerIndex(char *NamePtr)
{
    uint32 PeerIdx;

=======
int32 SBN_CheckCmdPipe(void) {
    int32 Status;

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

int32 SBN_GetPeerIndex(char *NamePtr) {
    uint32 PeerIdx;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_GetPeerIndex:  NamePtr = %s\n", NamePtr);
    }

>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    for (PeerIdx = 0; PeerIdx < SBN_MAX_NETWORK_PEERS; PeerIdx++)
    {

        if (SBN.Peer[PeerIdx].InUse == SBN_NOT_IN_USE)
            continue;

        if (strcmp(SBN.Peer[PeerIdx].Name, NamePtr) == 0)
            return (PeerIdx);/* Found it, so stop searching */

    }/* end for */

    return SBN_ERROR;

} /* end SBN_GetPeerIndex */

<<<<<<< HEAD
void SBN_SendWakeUpDebugMsg(void)
{
=======
void SBN_SendWakeUpDebugMsg(void) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    if (SBN.DebugOn == SBN_TRUE)
    {
        OS_printf(" awake ");
    }/* end if */
}/* end  SBN_SendWakeUpDebugMsg */

<<<<<<< HEAD
char *SBN_StateNum2Str(uint32 StateNum)
{

    static char SBN_StateName[OS_MAX_API_NAME];

    switch (StateNum)
    {
        case 0:
            strcpy(SBN_StateName, "SBN_INIT");
            break;

        case 1:
            strcpy(SBN_StateName, "SBN_ANNOUNCING");
            break;

        case 2:
            strcpy(SBN_StateName, "SBN_HEARTBEATING");
            break;

        default:
            strcpy(SBN_StateName, "SBN_UNKNOWN");
            break;

            return SBN_StateName;

    }/* end switch */

    return SBN_StateName;

}/* end SBN_StateNum2Str */

void SBN_ShowStates(void)
{
=======
void SBN_ShowStates(void) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    uint32 i;

    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].InUse == SBN_IN_USE)
        {
            OS_printf("%s:%s,PeerIdx=%d,State=%s\n", CFE_CPU_NAME,
<<<<<<< HEAD
                    SBN.Peer[i].Name, i, SBN_StateNum2Str(SBN.Peer[i].State));
=======
                    SBN.Peer[i].Name, i, SBN_LIB_StateNum2Str(SBN.Peer[i].State));
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        }/* end if */
    }/* end for */

}/* end SBN_ShowStates */

<<<<<<< HEAD
void SBN_ShowPeerSubs(uint32 PeerIdx)
{

    uint32 i;

    OS_printf("\n%s:PeerIdx %d, Name %s, State %s\n", CFE_CPU_NAME, PeerIdx,
            SBN.Peer[PeerIdx].Name, SBN_StateNum2Str(SBN.Peer[PeerIdx].State));
    OS_printf("%s:Rcvd %d subscriptions from %s\n", CFE_CPU_NAME,
            SBN.Peer[PeerIdx].SubCnt, SBN.Peer[PeerIdx].Name);

    for (i = 0; i < SBN.Peer[PeerIdx].SubCnt; i++)
    {
        OS_printf("0x%04X  ", SBN.Peer[PeerIdx].Sub[i].MsgId);
        if (((i + 1) % 5) == 0)
            OS_printf("\n");
    }/* end for */

}/* end SBN_ShowPeerSubs */

void SBN_ShowLocSubs(void)
{

    uint32 i;

    OS_printf("\n%s:%d Local Subscriptions\n", CFE_CPU_NAME, SBN.LocalSubCnt);
    for (i = 0; i < SBN.LocalSubCnt; i++)
    {
        OS_printf("0x%04X  ", SBN.LocalSubs[i].MsgId);
        if (((i + 1) % 5) == 0)
            OS_printf("\n");
    }/* end for */

}/* end SBN_ShowLocSubs */

void SBN_ShowAllSubs(void)
{

    uint32 i;

    SBN_ShowLocSubs();
    OS_printf("\n");
=======
void SBN_ShowPeerData(void) {
    uint32 i, protocol;

>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].InUse == SBN_IN_USE)
        {
<<<<<<< HEAD
            SBN_ShowPeerSubs(i);
        }/* end if */
    }/* end for */
}/* end SBN_ShowAllSubs */

void SBN_ShowPeerData(void)
{
    uint32 i;

    for (i = 0; i < SBN_MAX_NETWORK_PEERS; i++)
    {
        if (SBN.Peer[i].InUse == SBN_IN_USE)
        {
            OS_printf(
                    "%s:%s,Element %d,Adr=%s,Pipe=%d,PipeNm=%s,State=%s,SubCnt=%d\n",
                    CFE_CPU_NAME, SBN.Peer[i].Name, i, SBN.Peer[i].Addr,
                    SBN.Peer[i].Pipe, SBN.Peer[i].PipeName,
                    SBN_StateNum2Str(SBN.Peer[i].State), SBN.Peer[i].SubCnt);
=======
            protocol = SBN.Peer[i].ProtocolId;
            switch(protocol) {
              /*
                case SBN_IPv4:
                    SBN_ShowIPv4PeerData(i);
                    break;
              */
                default:
                    OS_printf("Unknown protocol id %d\n", protocol);
                    break;
            }
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        }/* end if */
    }/* end for */
}/* end SBN_ShowPeerData */

<<<<<<< HEAD
void SBN_NetMsgSendDbgEvt(uint32 MsgType, uint32 PeerIdx, int Status)
{

    /*CFE_EVS_SendEvent(SBN_PROTO_SENT_EID,CFE_EVS_DEBUG,*/
    if (SBN.DebugOn == SBN_FALSE)
    {
        return;
    }/* end if */

    switch (MsgType)
    {

=======
void SBN_NetMsgSendDbgEvt(uint32 MsgType, uint32 PeerIdx, int Status) {
    /*CFE_EVS_SendEvent(SBN_PROTO_SENT_EID,CFE_EVS_DEBUG,*/
    if (SBN.DebugOn == SBN_FALSE) {
        return;
    }

    switch (MsgType) {
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
        case SBN_APP_MSG:
            OS_printf("%s:Sent MsgId 0x%x to %s sz=%d\n", CFE_CPU_NAME,
                    CFE_SB_GetMsgId(
                            (CFE_SB_MsgPtr_t) &SBN.DataMsgBuf.Pkt.Data[0]),
                    SBN.Peer[PeerIdx].Name, Status);
            break;

        case SBN_SUBSCRIBE_MSG:
        case SBN_UN_SUBSCRIBE_MSG:
            OS_printf("%s:%s for 0x%x sent to %s sz=%d\n", CFE_CPU_NAME,
<<<<<<< HEAD
                    SBN_GetMsgName(MsgType),
                    ntohs(SBN.DataMsgBuf.Sub.MsgId),
=======
                    SBN_LIB_GetMsgName(MsgType),
                    CFE_SB_GetMsgId(
                            (CFE_SB_MsgPtr_t) &SBN.DataMsgBuf.Sub.MsgId),
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
                    SBN.Peer[PeerIdx].Name, Status);
            break;

        default:
            OS_printf("%s:%s sent to %s stat=%d\n", CFE_CPU_NAME,
<<<<<<< HEAD
                    SBN_GetMsgName(MsgType), SBN.Peer[PeerIdx].Name, Status);
=======
                    SBN_LIB_GetMsgName(MsgType), SBN.Peer[PeerIdx].Name, Status);
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
            break;
    }/* end switch */

}/* end SBN_NetMsgSendDbgEvt */

<<<<<<< HEAD
void SBN_NetMsgSendErrEvt(uint32 MsgType, uint32 PeerIdx, int Status)
{

    CFE_EVS_SendEvent(SBN_PROTO_SEND_ERR_EID, CFE_EVS_ERROR,
            "%s:Error sending %s to %s stat=%d", CFE_CPU_NAME,
            SBN_GetMsgName(MsgType), SBN.Peer[PeerIdx].Name, Status);

}/* end SBN_NetMsgSendErrEvt */

char *SBN_GetMsgName(uint32 MsgType)
{

    static char SBN_MsgName[OS_MAX_API_NAME];

    bzero(SBN_MsgName, OS_MAX_API_NAME);

    switch (MsgType)
    {

        case SBN_ANNOUNCE_MSG:
            strcpy(SBN_MsgName, "Announce");
            break;

        case SBN_ANNOUNCE_ACK_MSG:
            strcpy(SBN_MsgName, "Announce Ack");
            break;

        case SBN_HEARTBEAT_MSG:
            strcpy(SBN_MsgName, "Heartbeat");
            break;

        case SBN_HEARTBEAT_ACK_MSG:
            strcpy(SBN_MsgName, "Heartbeat Ack");
            break;

        case SBN_SUBSCRIBE_MSG:
            strcpy(SBN_MsgName, "Subscribe");
            break;

        case SBN_UN_SUBSCRIBE_MSG:
            strcpy(SBN_MsgName, "UnSubscribe");
            break;

        case SBN_APP_MSG:
            strcpy(SBN_MsgName, "App Msg");
            break;

        default:
            strcpy(SBN_MsgName, "Unknown");
            break;
    }/* end switch */

    SBN_MsgName[OS_MAX_API_NAME - 1] = '\0';

    return SBN_MsgName;

}/* SBN_GetMsgName */

void SBN_DebugOn(void)
{
    SBN.DebugOn = SBN_TRUE;
}/* end SBN_DebugOn */

void SBN_DebugOff(void)
{
    SBN.DebugOn = SBN_FALSE;
}/* end SBN_DebugOff */

=======
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
  if(SBN.DebugOn == SBN_TRUE) {
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



>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
