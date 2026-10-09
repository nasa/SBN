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
 *   Specification for the SBN table related
 *   constant definitions.
 */
#ifndef SBN_TBLDEFS_H
#define SBN_TBLDEFS_H

#include "common_types.h"
#include "cfe_mission_cfg.h"
#include "sbn_mission_cfg.h"
#include "sbn_extern_typedefs.h"

/****
 * @brief The config table contains entries for peers (other CPU's),
 * modules (back-end libraries used to talk to peers), networks
 * (all peers that communicate amonsgt each other using a specific
 * protocol/network technology), and interfaces (the interconnection
 * between the "host" end and the peer end of the network.)
 */
typedef struct
{
    /** @brief The name for this protocol module. */
    char Name[SBN_MISSION_MAX_MOD_NAME_LEN];

    /** @brief The file name to load the module from, if it's not already loaded by ES. */
    char LibFileName[CFE_MISSION_MAX_PATH_LEN];

    /** @brief The entry symbol to call when loaded. For protocol modules, this is the initialization fn.
     * For filter modules, this is the filter function symbol name.
     */
    char LibSymbol[CFE_MISSION_MAX_API_LEN];

    SBN_EventID_t BaseEID;
} SBN_Module_Entry_t;

typedef struct
{
    /** @brief Needs to match the ProcessorID of the peer. */
    SBN_ProcessorID_t ProcessorID;

    /** @brief Needs to match the SpacecraftID of the peer. */
    SBN_SpacecraftID_t SpacecraftID;

    /** @brief Network number indicating peers that inter-communicate using a common protocol. */
    SBN_NetIdx_t NetNum;

    /** @brief The name of the protocol module for which to use for this peer. */
    char ProtocolName[SBN_MISSION_MAX_MOD_NAME_LEN];

    /** @brief The modules name for the filter interface for this peer. */
    char Filters[SBN_MISSION_MAX_FILTERS_PER_PEER][SBN_MISSION_MAX_MOD_NAME_LEN];

    /** @brief Protocol-specific address, such as "127.0.0.1:3234". */
    uint8 Address[SBN_MISSION_ADDR_SZ];

    /** @brief Indicates whether to spawn tasks for send/recv; for a given netnum, probably wise to use the same
     *         TaskFlags setting.
     */
    SBN_Task_Flag_t TaskFlags;
} SBN_Peer_Entry_t;

#endif
