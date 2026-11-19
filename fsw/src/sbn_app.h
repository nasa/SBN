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

#include "sbn_resource.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* SBN application data structures                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct
{
    char               ProtocolName[CFE_MISSION_MAX_API_LEN];
    const SBN_IfOps_t *Intf;
} SBN_ProtocolModInfo_t;

typedef struct
{
    char                         FilterName[CFE_MISSION_MAX_API_LEN];
    const SBN_FilterInterface_t *Intf;
} SBN_FilterModInfo_t;

/**
 * \brief SBN global data structure definition
 */
typedef struct
{
    SBN_PeerRecord_t PeerRecords[SBN_MISSION_MAX_PEER_CNT];
    SBN_NetRecord_t  NetRecords[SBN_PLATFORM_MAX_NETS];

    SBN_NetId_t  NetIds[SBN_PLATFORM_MAX_NETS];
    SBN_NetIdx_t NetCnt;

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
     * \brief All subscriptions by local apps connected to this SB.
     *
     * When a peer and I connect, I send that peer all subscriptions I have
     * and they send me theirs. All messages on the local bus that are
     * subscribed to by the peer are sent over, and vice-versa.
     */
    SBN_SubscriptionSet_t SubSet;

    /* Tracks individual changes to the local subscriptions */
    SBN_SubJournal_t SubQ;

    /** \brief CFE scheduling pipe */
    CFE_SB_PipeId_t SchPipe;

    /**
     * Each SBN back-end module provides a number of functions to
     * implement the protocols to connect peers.
     */
    SBN_ProtocolModInfo_t ProtocolModInfo[SBN_MISSION_MAX_MOD_CNT];
    SBN_FilterModInfo_t   FilterModInfo[SBN_MISSION_MAX_MOD_CNT];

    SBN_HkCounter_t CmdCnt, CmdErrCnt;

    CFE_TBL_Handle_t ConfTblHandle;

    CFE_ResourceId_t LastPeerId;
    CFE_ResourceId_t LastNetId;

} SBN_AppData_t;

/**
 * \brief SBN glocal data structure references, indexed by AppId.
 */
extern SBN_AppData_t SBN_AppData;

/*
** Prototypes
*/

void SBN_AppMain(void);
SBN_PeerRecord_t              *
SBN_GetPeerRecord(SBN_NetRecord_t *NetRec, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID);
SBN_Status_t SBN_ReloadConfTbl(void);

/**
 * @brief Initializes the housekeeping counters for a peer.
 *
 * @param[in] Peer The peer interface for which to reset housekeeping.
 */
void SBN_InitializePeerCounters(SBN_PeerRecord_t *PeerRec);

/**
 * @brief Initializes the housekeeping counters both at the app level
 *        and for each peer.
 */
void SBN_InitializeCounters(void);

/**
 * @brief Table validation function
 *
 * Sanity-checks the pending config table data before the table gets activated
 *
 * This is compatible with the Table Services validation API
 *
 * @param[in] TableData The table data being loaded
 * @retval SBN_SUCCESS (0) if table OK
 */
SBN_Status_t SBN_ValidateConfTbl(void *TableData);

/**
 * @brief Compatibility routine for Peer interfaces
 *
 * Get the private data record associated with the interface object for a peer
 * To simplify error checking around transitional use cases, NULL is passed through
 */
static inline SBN_PeerRecord_t *SBN_Peer_GetPrivateData(SBN_PeerInterface_t *PeerIf)
{
    if (PeerIf != NULL)
    {
        return (void *)(((char *)PeerIf) - offsetof(SBN_PeerRecord_t, Intf));
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Compatibility routine for Peer interfaces
 *
 * Get the public (module) interface struct from the full (private) record
 * To simplify error checking around transitional use cases, NULL is passed through
 */
static inline SBN_PeerInterface_t *SBN_Peer_GetIntfData(SBN_PeerRecord_t *PeerRec)
{
    if (PeerRec != NULL)
    {
        return &PeerRec->Intf;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Compatibility routine for Net interfaces
 *
 * Get the private data record associated with the interface object for a net
 * To simplify error checking around transitional use cases, NULL is passed through
 */
static inline SBN_NetRecord_t *SBN_Net_GetPrivateData(SBN_NetInterface_t *NetIf)
{
    if (NetIf != NULL)
    {
        return (void *)(((char *)NetIf) - offsetof(SBN_NetRecord_t, Intf));
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Compatibility routine for Net interfaces
 *
 * Get the public (module) interface struct from the full (private) record
 * To simplify error checking around transitional use cases, NULL is passed through
 */
static inline SBN_NetInterface_t *SBN_Net_GetIntfData(SBN_NetRecord_t *NetRec)
{
    if (NetRec != NULL)
    {
        return &NetRec->Intf;
    }
    else
    {
        return NULL;
    }
}

/*
 * ~~~~~~~~~~~~ MODULE INTERFACE METHODS ~~~~~~~~~~~~~~~
 *
 * The declarations within this section are part of the module <-> SBN app interface
 * They cannot be changed without affecting module compatibility.
 * All I/O should be in the form of "Interface" structs where applicable.
 */
SBN_PeerInterface_t              *
SBN_GetPeer(SBN_NetInterface_t *Net, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID);
void         SBN_PackMsg(void              *SBNBuf,
                         SBN_MsgSz_t        MsgSz,
                         SBN_MsgType_t      MsgType,
                         SBN_ProcessorID_t  ProcessorID,
                         SBN_SpacecraftID_t SpacecraftID,
                         void              *Msg);
bool         SBN_UnpackMsg(void               *SBNBuf,
                           SBN_MsgSz_t        *MsgSzPtr,
                           SBN_MsgType_t      *MsgTypePtr,
                           SBN_ProcessorID_t  *ProcessorIDPtr,
                           SBN_SpacecraftID_t *SpacecraftIDPtr,
                           void               *Msg);
SBN_Status_t SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer);

#endif /* _sbn_app_ */
