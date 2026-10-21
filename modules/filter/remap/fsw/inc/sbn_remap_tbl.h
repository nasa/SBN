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

#ifndef _sbn_remap_tbl_h_
#define _sbn_remap_tbl_h_

#include "cfe.h"
#include "sbn_platform_cfg.h"
#include "sbn_extern_typedefs.h"

/****
 * @brief The RemapTbl defines, for a peer, which MID's should be remapped
 * (listing a "from" MID and a "to" MID, determining when a message is
 * published locally with the "from" MID, it is sent to the peer with the "to"
 * MID.)
 *
 * Note that there should only be one "to" for any "from". One-to-many is not supported by the module.
 *
 * The RemapTbl must be sorted and unique for ProcessorID + from
 * Logic is as follows:
 * for each message->
 *      if there's an entry->
 *          if the "to" is defined->remap the MID to the new MID
 *              (note new MID can be the same as the old MID to send unmodified)
 *          else->ignore the message
 *      else->
 *          if the RemapDefaultFlag == SBN_REMAP_DEFAULT_IGNORE->
 *              ignore the message
 *          else->send the message on unmodified
 */
typedef struct
{
    /** @brief The ProcessorID of the peer to remap this MID for. */
    SBN_ProcessorID_t ProcessorID;

    /** @brief The SpacecraftID of the peer to remap this MID for. */
    SBN_SpacecraftID_t SpacecraftID;

    /** @brief The local MID I'll receive from the pipe. */
    CFE_SB_MsgId_t FromMID;

    /** @brief The MID to send to the peer. If 0, filter the from MID. */
    CFE_SB_MsgId_t ToMID;
} SBN_RemapTblEntry_t;

#define SBN_REMAP_TABLE_SHARENAME "SBN.RemapTbl"

/** @brief If the default flag is set to "IGNORE", SBN will ONLY send messages
 * defined in this table.
 */
#define SBN_REMAP_DEFAULT_IGNORE 0

/** @brief If the default flag is set to "SEND", SBN will send all messages
 * not defined in this table.
 */
#define SBN_REMAP_DEFAULT_SEND 1

/** @brief The maximum number of remapping definitions.
 */
#define SBN_REMAP_TABLE_SIZE 256

typedef struct
{
    /** @brief Behavior for MID's not found in this table. */
    uint32 RemapDefaultFlag;

    /** @brief The remapping entries. */
    SBN_RemapTblEntry_t Entries[SBN_REMAP_TABLE_SIZE];
} SBN_RemapTbl_t;

#endif /* _sbn_remap_tbl_h_ */
