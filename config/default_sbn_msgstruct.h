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
 *   message data types.
 *
 * @note
 *   Constants and enumerated types related to these message structures
 *   are defined in sbn_msgdefs.h.
 */
#ifndef DEFAULT_SBN_MSGSTRUCT_H
#define DEFAULT_SBN_MSGSTRUCT_H

/************************************************************************
 * Includes
 ************************************************************************/

#include "sbn_mission_cfg.h"
#include "sbn_msgdefs.h"
#include "cfe_msg_hdr.h"

/*************************************************************************/

typedef struct SBN_TlmBase
{
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    uint8                     TlmId;
} SBN_TlmBase_t;

typedef struct SBN_HkTlm
{
    SBN_TlmBase_t       TlmBase;
    SBN_HkTlm_Payload_t Payload;
} SBN_HkTlm_t;

typedef struct SBN_HkNetTlm
{
    SBN_TlmBase_t          TlmBase;
    SBN_HkNetTlm_Payload_t Payload;
} SBN_HkNetTlm_t;

typedef struct SBN_HkPeerTlm
{
    SBN_TlmBase_t           TlmBase;
    SBN_HkPeerTlm_Payload_t Payload;
} SBN_HkPeerTlm_t;

typedef struct SBN_HkPeerSubsTlm
{
    SBN_TlmBase_t               TlmBase;
    SBN_HkPeerSubsTlm_Payload_t Payload;
} SBN_HkPeerSubsTlm_t;

typedef struct SBN_HkMySubsTlm
{
    SBN_TlmBase_t             TlmBase;
    SBN_HkMySubsTlm_Payload_t Payload;
} SBN_HkMySubsTlm_t;

typedef struct SBN_NoopCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SBN_NoopCmd_t;

typedef struct SBN_SendHkCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SBN_SendHkCmd_t;

typedef struct SBN_SendHkNetCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
    SBN_NetCmd_Payload_t    Payload;
} SBN_SendHkNetCmd_t;

typedef struct SBN_SendHkPeerCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
    SBN_PeerCmd_Payload_t   Payload;
} SBN_SendHkPeerCmd_t;

typedef struct SBN_SendHkPeerSubsCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
    SBN_PeerCmd_Payload_t   Payload;
} SBN_SendHkPeerSubsCmd_t;

typedef struct SBN_SendHkMySubsCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SBN_SendHkMySubsCmd_t;

typedef struct SBN_ResetCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SBN_ResetCmd_t;

typedef struct SBN_ResetPeerCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
    SBN_PeerCmd_Payload_t   Payload;
} SBN_ResetPeerCmd_t;

typedef struct SBN_WakeupCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SBN_WakeupCmd_t;

typedef struct SBN_ReloadTblCmd
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SBN_ReloadTblCmd_t;

#endif /* SBN_MSGSTRUCT_H */
