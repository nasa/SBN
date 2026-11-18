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

/**
 * Checks if this peer has an active/pending outbound data job
 *
 * Only one outbound job should be pending at any time on any peer.  This
 * effectively means that only one thread at a time will be sending to
 * a peer, thus reducing the need for locking.
 *
 * @retval true  if this peer has a pending job (should not be re-scheduled)
 * @retval false if this peer does not have a pending job
 */
bool SBN_Outbound_IsScheduled(SBN_PeerRecord_t *PeerRec);

/**
 * Worker function - Move outbound data at peer scope
 *
 * Generates any outbound messages to the peer, including protocol messages
 * and/or application data messages pulled from the local bus.
 *
 * This is intended to be used with the worker tasks.  In this pattern it
 * does not return a value, but performs its work and sends events and/or
 * updates any state variables in the record reflecting what was done.  It
 * may also reschedule itself to reduce latency, if configured.
 *
 * @param[inout] PeerRec   The peer record structure
 * @param[inout] BufD      The buffer to use for receiving messages
 */
void SBN_Outbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);

/**
 * @brief Outbound data top-level maintenance function
 *
 * This must be called periodically from the SBN task main polling loop
 * to check and schedule outbound data tasks for the various peers.
 */
void SBN_Outbound_PollCycle(void);

#endif
