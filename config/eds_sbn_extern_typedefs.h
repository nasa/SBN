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
 *
 * Declarations and prototypes for sbn_extern_typedefs module
 */

#ifndef EDS_SBN_EXTERN_TYPEDEFS_H
#define EDS_SBN_EXTERN_TYPEDEFS_H

#include "common_types.h"

/* Source the definitions from EDS */
#include "sbn_eds_typedefs.h"

/* mapping for task flag enum values (needed for compatibility with existing defs) */
typedef enum
{
    SBN_TASK_POLL = EdsLabel_SBN_TaskFlag_POLL,
    SBN_TASK_SEND = EdsLabel_SBN_TaskFlag_SEND,
    SBN_TASK_RECV = EdsLabel_SBN_TaskFlag_RECV,
    SBN_TASKS     = EdsLabel_SBN_TaskFlag_SENDRECV
} SBN_Task_Flag_t;

typedef SBN_ModuleIdx_Atom_t    SBN_ModuleIdx_t;
typedef SBN_NetIdx_Atom_t       SBN_NetIdx_t;
typedef SBN_PeerIdx_Atom_t      SBN_PeerIdx_t;
typedef SBN_HkCounter_Atom_t    SBN_HkCounter_t;
typedef SBN_SubCnt_Atom_t       SBN_SubCnt_t;
typedef SBN_Timestamp_Atom_t    SBN_Timestamp_t;
typedef SBN_EventID_Atom_t      SBN_EventID_t;
typedef SBN_ProcessorID_Atom_t  SBN_ProcessorID_t;
typedef SBN_SpacecraftID_Atom_t SBN_SpacecraftID_t;

#endif
