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

#ifndef _sbn_app_
#define _sbn_app_

#include <string.h>
#include <errno.h>

#include "osconfig.h"
#include "cfe.h"
#include "sbn_version.h"
#include "sbn_interfaces.h"
#include "sbn_msg.h"
#include "sbn_platform_cfg.h"
#include "sbn_tbl.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"
#include "sbn_msgids.h"
#include "sbn_cmds.h"
#include "sbn_subs.h"
#include "sbn_main_events.h"
#include "sbn_perfids.h"
#include "sbn_types.h"

#include "cfe_platform_cfg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* SBN application data structures                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * \brief SBN global data structure definition
 */
typedef struct
{
    SBN_NetIdx_t NetCnt;

    /** \brief Data only on host definitions. */
    SBN_NetInterface_t Nets[SBN_MAX_NETS];

    /** \brief The application ID provided by ES */
    CFE_ES_AppId_t AppID;

    /** \brief The application full name provided by SB */
    char App_FullName[(OS_MAX_API_NAME * 2)];

    /**
     * \brief The subscription pipe used to monitor local subscriptions.
     */
    CFE_SB_PipeId_t SubPipe;

    /**
     * \brief The command pipe used to receive commands.
     */
    CFE_SB_PipeId_t CmdPipe;

    /**
     * \brief Number of local subs.
     */
    SBN_SubCnt_t SubCnt;

    /**
     * \brief All subscriptions by apps connected to the SB.
     *
     * When a peer and I connect, I send that peer all subscriptions I have
     * and they send me theirs. All messages on the local bus that are
     * subscribed to by the peer are sent over, and vice-versa.
     */
    SBN_Subs_t Subs[SBN_MAX_SUBS_PER_PEER + 1];

    /** \brief CFE scheduling pipe */
    CFE_SB_PipeId_t SchPipe;

    /**
     * Each SBN back-end module provides a number of functions to
     * implement the protocols to connect peers.
     */
    SBN_IfOps_t *IfOps[SBN_MAX_MOD_CNT];

    /** @brief Retain the module ID's for each interface in case we need to unload. */
    CFE_ES_ModuleID_t ProtocolModules[SBN_MAX_MOD_CNT];

    /** @brief Retain the module ID's for each interface in case we need to unload. */
    CFE_ES_ModuleID_t FilterModules[SBN_MAX_MOD_CNT];

    SBN_ConfTbl_t *ConfTbl;

    /** Global mutex for Send Tasks. */
    CFE_ES_MutexID_t SendMutex;

    /** Global mutex for reconfiguring. */
    CFE_ES_MutexID_t ConfMutex;

    SBN_HKTlm_t CmdCnt, CmdErrCnt;

    CFE_TBL_Handle_t ConfTblHandle;

    /* Buffer for receiving messages, allocated here to avoid stack smashing */
    uint8 MsgBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
} SBN_AppData_t;

/**
 * \brief SBN glocal data structure references, indexed by AppId.
 */
extern SBN_AppData_t SBN_AppData;

/*
** Prototypes
*/
void         SBN_AppMain(void);
SBN_Status_t SBN_ProcessNetMsg(SBN_NetInterface_t *Net,
                               SBN_MsgType_t       MsgType,
                               CFE_ProcessorID_t   ProcessorID,
                               CFE_SpacecraftID_t  SpacecraftID,
                               SBN_MsgSz_t         MsgSz,
                               void               *Msg);
SBN_PeerInterface_t              *
SBN_GetPeer(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID);
SBN_Status_t SBN_ReloadConfTbl(void);
void         SBN_RecvNetTask(void);
void         SBN_RecvPeerTask(void);
void         SBN_SendTask(void);
SBN_Status_t SBN_Connected(SBN_PeerInterface_t *Peer);
SBN_Status_t SBN_Disconnected(SBN_PeerInterface_t *Peer);
void         SBN_PackMsg(void              *SBNBuf,
                         SBN_MsgSz_t        MsgSz,
                         SBN_MsgType_t      MsgType,
                         CFE_ProcessorID_t  ProcessorID,
                         CFE_SpacecraftID_t SpacecraftID,
                         void              *Msg);
bool         SBN_UnpackMsg(void               *SBNBuf,
                           SBN_MsgSz_t        *MsgSzPtr,
                           SBN_MsgType_t      *MsgTypePtr,
                           CFE_ProcessorID_t  *ProcessorIDPtr,
                           CFE_SpacecraftID_t *SpacecraftIDPtr,
                           void               *Msg);
SBN_Status_t SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer);
SBN_Status_t SBN_RecvNetMsgs(void);

#endif /* _sbn_app_ */
