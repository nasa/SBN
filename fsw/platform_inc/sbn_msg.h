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

#ifndef _sbn_msg_h_
#define _sbn_msg_h_

#include "sbn_msgdefs.h"
#include "sbn_platform_cfg.h"
#include "sbn_types.h"
#include "cfe.h"

#define SBN_CMD_NET_LEN sizeof(CFE_MSG_CommandHeader_t) + sizeof(SBN_NetIdx_t)

#define SBN_CMD_PEER_LEN sizeof(CFE_MSG_CommandHeader_t) + sizeof(SBN_PeerIdx_t)

/** @brief CC, CmdCnt, CmdErrCnt, SubCnt, NetCnt */
#define SBN_HK_LEN (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + (sizeof(SBN_HKTlm_t) * 4))

/** @brief CC, SBN_SubCnt_t SubCnt, CFE_SB_MsgId_t Subs[SBN_MAX_SUBS_PER_PEER] */
#define SBN_HKMYSUBS_LEN                                                      \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_SubCnt_t) \
     + SBN_MAX_SUBS_PER_PEER * sizeof(CFE_SB_MsgId_t))

/** @brief CC, NetIdx, PeerIdx, SubCnt, Subs[SBN_MAX_SUBS_PER_PEER] */
#define SBN_HKPEERSUBS_LEN                                                                            \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_NetIdx_t) + sizeof(SBN_PeerIdx_t) \
     + sizeof(SBN_SubCnt_t) + SBN_MAX_SUBS_PER_PEER * sizeof(CFE_SB_MsgId_t))

/** @brief CC, SubCnt, ProcessorID, LastSend, LastRecv, SendCnt, RecvCnt, SendErrCnt, RecvErrCnt */
#define SBN_HKPEER_LEN                                                                                    \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_SubCnt_t) + sizeof(SBN_ProcessorID_t) \
     + sizeof(OS_time_t) * 2 + sizeof(SBN_HKTlm_t) * 4)

/** @brief CC, ProtocolID, PeerCnt */
#define SBN_HKNET_LEN \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_ModuleIdx_t) + sizeof(SBN_PeerIdx_t))

/**
 * @brief Module status response packet structure
 */
typedef struct
{
    /**
     * This is a struct that will be sent (as-is) in a response to a HK command.
     */
    CFE_MSG_TelemetryHeader_t TlmHeader;
    /** @brief The Protocol ID being queried. */
    SBN_ModuleIdx_t           ProtocolIdx;
    /** @brief The module status as returned by the module. */
    uint8                     ModuleStatus[SBN_MOD_STATUS_MSG_SZ];
} SBN_ModuleStatusPacket_t;

#endif /* _sbn_msg_h_ */
