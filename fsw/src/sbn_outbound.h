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

#ifndef SBN_OUTBOUND_H
#define SBN_OUTBOUND_H

/*
 ** Include Files
 */
#include "sbn_interfaces.h"
#include "cfe_sb_api_typedefs.h"

/**
 * Sends an outbound message to a peer using the module's send API.
 *
 * @param[inout] BufD The buffer containing the message to send
 * @param[in] Peer The peer to send the message to.
 * @returns SBN status value
 * @retval  #SBN_SUCCESS if successfully sent
 */
SBN_Status_t SBN_Outbound_SendToPeer(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer);

/**
 * Gets an outbound message from the peer software bus pipe
 *
 * @param[inout] BufD    The buffer where the message can be stored
 * @param[in]    Peer    The peer to send the message to.
 * @param[in]    Timeout Timeout to use when reading from SB
 * @returns SBN status value
 * @retval  #SBN_SUCCESS if successfully retrieved
 * @retval  #SBN_IF_EMPTY if the software bus pipe had no messages
 */
SBN_Status_t SBN_Outbound_GetMsg(SBN_BufferDesc_t *BufD, CFE_SB_PipeId_t PeerPipe, int32 Timeout);

#endif
