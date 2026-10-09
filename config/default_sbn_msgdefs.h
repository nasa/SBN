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

/**
 * @file
 *   Specification for the SBN command and telemetry
 *   message payload and constant definitions.
 */
#ifndef DEFAULT_SBN_MSGDEFS_H
#define DEFAULT_SBN_MSGDEFS_H

#include "common_types.h"
#include "sbn_fcncodes.h"
#include "cfe_sb_extern_typedefs.h"
#include "sbn_extern_typedefs.h"
#include "sbn_mission_cfg.h"

/*
 *  ******************************************************************************
 *  **                             Type Definitions                             **
 *  ******************************************************************************
 */

typedef uint8_t        SBN_ModuleStatus_t[SBN_MISSION_MOD_STATUS_MSG_SZ];
typedef CFE_SB_MsgId_t SBN_SubsList_t[SBN_MISSION_MAX_SUBS_PER_PEER];

typedef struct SBN_HkTlm_Payload
{
    SBN_HkCounter_t CmdCnt;
    SBN_HkCounter_t CmdErrCnt;
    SBN_SubCnt_t    SubCnt;
    SBN_HkCounter_t NetCnt;
} SBN_HkTlm_Payload_t;

typedef struct SBN_HkNetTlm_Payload
{
    SBN_ModuleIdx_t ProtocolIdx;
    SBN_PeerIdx_t   PeerCnt;
} SBN_HkNetTlm_Payload_t;

typedef struct SBN_HkPeerTlm_Payload
{
    uint32          ProcessorId;
    SBN_Timestamp_t LastSend;
    SBN_Timestamp_t LastRecv;
    SBN_HkCounter_t SendCnt;
    SBN_HkCounter_t RecvCnt;
    SBN_HkCounter_t SendErrCnt;
    SBN_HkCounter_t RecvErrCnt;
    SBN_SubCnt_t    SubCnt;
} SBN_HkPeerTlm_Payload_t;

typedef struct SBN_HkPeerSubsTlm_Payload
{
    SBN_NetIdx_t   NetIdx;
    SBN_PeerIdx_t  PeerIdx;
    SBN_SubCnt_t   SubCnt;
    SBN_SubsList_t Subs;
} SBN_HkPeerSubsTlm_Payload_t;

typedef struct SBN_HkMySubsTlm_Payload
{
    SBN_SubCnt_t   SubCnt;
    SBN_SubsList_t Subs;
} SBN_HkMySubsTlm_Payload_t;

typedef struct SBN_HkModuleStatusTlm_Payload
{
    SBN_ModuleIdx_t    ProtocolIdx;
    SBN_ModuleStatus_t ModuleStatus;
} SBN_HkModuleStatusTlm_Payload_t;

typedef struct SBN_NetCmd_Payload
{
    SBN_NetIdx_t NetIdx;
} SBN_NetCmd_Payload_t;

typedef struct SBN_PeerCmd_Payload
{
    SBN_NetIdx_t  NetIdx;
    SBN_PeerIdx_t PeerIdx;
} SBN_PeerCmd_Payload_t;

#endif
