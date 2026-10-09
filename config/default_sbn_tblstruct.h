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
 *   Specification for the SBN table structures
 *
 * Provides default definitions for SBN table structures
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef SBN_TBLSTRUCT_H
#define SBN_TBLSTRUCT_H

/*************************************************************************
 * Includes
 *************************************************************************/
#include "sbn_mission_cfg.h"
#include "sbn_tbldefs.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*************************************************************************
 * Type Definitions
 *************************************************************************/
typedef struct
{
    SBN_Module_Entry_t ProtocolModules[SBN_MISSION_MAX_MOD_COUNT];
    SBN_ModuleIdx_t    ProtocolCnt;
    SBN_Module_Entry_t FilterModules[SBN_MISSION_MAX_MOD_COUNT];
    SBN_ModuleIdx_t    FilterCnt;
    SBN_Peer_Entry_t   Peers[SBN_MISSION_MAX_PEER_COUNT];
    SBN_PeerIdx_t      PeerCnt;
} SBN_ConfTbl_t;

#endif
