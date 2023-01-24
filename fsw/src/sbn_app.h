/******************************************************************************
** File: sbn_app.h
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
**      This header file contains prototypes for private functions and type
**      definitions for the Software Bus Network Application.
**
** Authors:   J. Wilmot/GSFC Code582
**            R. McGraw/SSI
**            C. Knight/ARC Code TI
**
******************************************************************************/

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

#ifndef SBN_TLM_MID
/* backwards compatibility in case you're using a MID generator */
#define SBN_TLM_MID SBN_HK_TLM_MID
#endif /* SBN_TLM_MID */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* SBN application data structures                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void  SBN_ShowPeerData(void);
int32 SBN_GetPeerFileData(void);

SBN_Status_t SBN_RecvNetMsgs(void);

void SBN_CheckPeerPipes(void);

/**
 * \brief SBN global data structure definition
 */
typedef struct
{
    SBN_NetIdx_t NetCnt;

    /** \brief Data only on host definitions. */
    SBN_NetInterface_t Nets[SBN_MAX_NETS];

    /** \brief The application ID provided by ES */
    CFE_ES_AppID_t AppID;

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
} SBN_App_t;

/**
 * \brief SBN glocal data structure references, indexed by AppId.
 */
extern SBN_App_t SBN;

/*
** Prototypes
*/
void                 SBN_AppMain(void);
SBN_Status_t         SBN_ProcessNetMsg(SBN_NetInterface_t *Net, SBN_MsgType_t MsgType, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID,
                                       SBN_MsgSz_t MsgSz, void *Msg);
SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID);
SBN_Status_t         SBN_ReloadConfTbl(void);
void                 SBN_RecvNetTask(void);
void                 SBN_RecvPeerTask(void);
void                 SBN_SendTask(void);
SBN_Status_t         SBN_Connected(SBN_PeerInterface_t *Peer);
SBN_Status_t         SBN_Disconnected(SBN_PeerInterface_t *Peer);
void                 SBN_PackMsg(void *SBNBuf, SBN_MsgSz_t MsgSz, SBN_MsgType_t MsgType, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID, void *Msg);
bool                 SBN_UnpackMsg(void *SBNBuf, SBN_MsgSz_t *MsgSzPtr, SBN_MsgType_t *MsgTypePtr, CFE_ProcessorID_t *ProcessorIDPtr, CFE_SpacecraftID_t *SpacecraftIDPtr, void *Msg);
SBN_Status_t         SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer);
SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID);

#endif /* _sbn_app_ */
