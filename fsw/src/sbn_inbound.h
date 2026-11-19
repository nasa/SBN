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

#ifndef SBN_INBOUND_H
#define SBN_INBOUND_H

/*
 ** Include Files
 */
#include "sbn_interfaces.h"
#include "sbn_resource.h"

/**
 * Worker function - Move inbound data at peer scope
 *
 * Depending on the network topology, some nets permit receiving direct from a
 * single peer.  This utilizes the RecvFromPeer method defined in the net intf
 * to pull data from a specific peer.
 *
 * This is intended to be used with the worker tasks.  In this pattern it
 * does not return a value, but performs its work and sends events and/or
 * updates any state variables in the record reflecting what was done.  It
 * may also reschedule itself to reduce latency, if configured.
 *
 * @note This should not be used unless RecvFromPeer is defined for
 * the underlying net.  Use SBN_Inbound_ServiceNet() instead.
 *
 * @param[inout] PeerRec   The peer record structure
 * @param[inout] BufD      The buffer to use for receiving messages
 */
void SBN_Inbound_ServicePeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);

/**
 * Worker function - Move inbound data at net scope
 *
 * Depending on the network topology, some nets do not permit receiving from a
 * single peer.  This utilizes the RecvFromNet method defined in the net intf
 * to pull data from the net.  The peer is then identified from the received
 * data, and the message is proceesed under the context of that peer.
 *
 * This is intended to be used with the worker tasks.  In this pattern it
 * does not return a value, but performs its work and sends events and/or
 * updates any state variables in the record reflecting what was done.  It
 * may also reschedule itself to reduce latency, if configured.
 *
 * @note This should not be used unless RecvFromNet is defined for
 * the underlying net.  Use SBN_Inbound_ServicePeer() instead.
 *
 * @param[inout] NetRec    The net record structure
 * @param[inout] BufD      The buffer to use for receiving messages
 */
void SBN_Inbound_ServiceNet(SBN_NetRecord_t *NetRec, SBN_BufferDesc_t *BufD);

/**
 * Forwards a received message to the local software bus
 *
 * After a message is identified as containing a CFE software bus
 * message intended for this endpoint, this will forward the message
 * to the local software bus.
 *
 * @param[inout] PeerRec   The peer record structure
 * @param[inout] BufD      The buffer containing the software bus message
 */
SBN_Status_t SBN_Inbound_ForwardMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec);

/**
 * Checks if this peer has an active/pending inbound data job
 *
 * Only one inbound job should be pending at any time on any peer.  This
 * effectively means that only one thread at a time will be receiving from
 * a peer, thus reducing the need for locking.
 *
 * @note This only checks for jobs directly scheduled at the peer level.
 * If the peer is operating on a net that receives at the net level, this
 * will always be false.  The net must be checked in that case.
 *
 * @retval true  if this peer has a pending job (should not be re-scheduled)
 * @retval false if this peer does not have a pending job
 */
bool SBN_Inbound_IsPeerScheduled(SBN_PeerRecord_t *PeerRec);

/**
 * Checks if this net has an active/pending inbound data job
 *
 * Only one inbound job should be pending at any time on any net.  This
 * effectively means that only one thread at a time will be receiving from
 * a net, thus reducing the need for locking.
 *
 * @note This only checks for jobs directly scheduled at the net level.
 * If the net receives messages at the peer level, this will always be false.
 * The specific peer must be checked in that case.
 *
 * @retval true  if this net has a pending job (should not be re-scheduled)
 * @retval false if this net does not have a pending job
 */
bool SBN_Inbound_IsNetScheduled(SBN_NetRecord_t *NetRec);

/**
 * Process a single inbound message from the network interface
 *
 * @param[inout] PeerRec   The peer record structure
 * @param[inout] BufD      The buffer containing a message from the peer
 *
 * @return SBN_SUCCESS on successful processing, SBN_ERROR otherwise
 */
SBN_Status_t SBN_Inbound_ProcessMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec);

/**
 * @brief Inbound data top-level maintenance function
 *
 * This must be called periodically from the SBN task main polling loop
 * to check and schedule inbound data tasks for the various peers.
 */
void SBN_Inbound_PollCycle(void);

#endif
