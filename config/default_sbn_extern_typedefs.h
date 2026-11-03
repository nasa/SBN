/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautisbn and Space Administration.
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
 *   Specification for the CFS Checksum command and telemetry
 *   messages.
 */
#ifndef DEFAULT_SBN_EXTERN_TYPEDEFS_H
#define DEFAULT_SBN_EXTERN_TYPEDEFS_H

#include "common_types.h"

/**
 * Message types definitions, reserve the high 128 values for module-specific
 * message types.
 */
typedef enum SBN_MsgType
{
    SBN_NO_MSG    = 0x00, /**< @brief no payload */
    SBN_SUB_MSG   = 0x01, /**< @brief payload is subs */
    SBN_UNSUB_MSG = 0x02, /**< @brief payload is unsubs */
    SBN_APP_MSG   = 0x03, /**< @brief payload is SB msg */
    SBN_PROTO_MSG = 0x04, /**< @brief payload is SBN proto */
} SBN_MsgType_t;

typedef uint8 SBN_MsgType_Enum_t; /* This uses SBN_MsgType enum labels but fixed as 8 bits */

typedef uint8  SBN_ModuleIdx_t;
typedef uint8  SBN_NetIdx_t;
typedef uint8  SBN_PeerIdx_t;
typedef uint16 SBN_HkCounter_t;
typedef uint16 SBN_SubCnt_t;
typedef int64  SBN_Timestamp_t;
typedef uint32 SBN_MsgSz_t;

typedef uint16 SBN_EventID_t;
typedef uint32 SBN_ProcessorID_t;
typedef uint32 SBN_SpacecraftID_t;

typedef enum
{
    SBN_TASK_POLL = 0,
    SBN_TASK_SEND = 1,
    SBN_TASK_RECV = 2,
    SBN_TASKS     = 3
} SBN_Task_Flag_t;

typedef uint8 SBN_TaskFlag_Enum_t; /* This uses SBN_TaskFlag enum labels but fixed as 8 bits */

/**\}*/

#endif
