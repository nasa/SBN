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

#ifndef SBN_CONNECTION_H
#define SBN_CONNECTION_H

/*
 ** Include Files
 */
#include "sbn_interfaces.h"
#include "sbn_resource.h"

/**
 * Called by a protocol module to signal that a peer has been connected.
 *
 * @param[in] Peer The peer that has been connected.
 * @return SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Connection_HandleConnect(SBN_PeerInterface_t *Peer);

/**
 * Called by a protocol module to signal that a peer has been disconnected.
 *
 * @param[in] Peer The peer that has been disconnected.
 * @return SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Connection_HandleDisconnect(SBN_PeerInterface_t *Peer);

/**
 * @brief Checks the health of a peer connection.
 *
 * Invokes the "Poll" operation on the configured peer.  This checks the status
 * of the connection.  It may generate heartbeat/keepalive messages to the peer.
 *
 * This is intended to be called from the outbound data task associated with a peer
 * as it may produce outbound data, thus should be synchronized with other outbound
 * traffic.
 *
 * @param[in]    PeerRec The peer that has been connected.
 * @param[inout] BufD    A buffer that may be used for sending messages to the peer
 *
 * @retval #SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Connection_CheckPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);

/**
 * @brief Sends the protocol version message to the peer
 *
 * This is a message to check compatibility between peers and is part of connection
 * establishment.  This function will generate that message to the peer.
 *
 * This is intended to be called from the outbound data task associated with a peer
 * as it produces outbound data, thus should be synchronized with other outbound
 * traffic.
 *
 * @param[inout] BufD    The buffer descriptor to use for building the message
 * @param[inout] PeerRec The peer record that has been connected.
 * @retval #SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Connection_SendProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec);

/**
 * @brief Check/verify the protocol version message from the peer
 *
 * This is a message to check compatibility between peers and is part of connection
 * establishment.  This checks that the message from the peer is valid.
 *
 * This is intended to be called from the inbound data task associated with a peer
 * as it processes inbound data, thus should be synchronized with other inbound
 * traffic.
 *
 * @param[inout] BufD    The buffer descriptor to containing the message
 * @param[inout] PeerRec The peer record that has been connected.
 * @retval #SBN_SUCCESS or SBN_ERROR if there was an issue.
 */
SBN_Status_t SBN_Connection_RecvProtocolVersionMsg(SBN_BufferDesc_t *BufD, SBN_PeerRecord_t *PeerRec);

/**
 * @brief Connection management top-level maintenance function
 *
 * This must be called periodically from the SBN task main polling loop
 * to check and maintain the connection states of the various peers.
 */
void SBN_Connection_PollCycle(void);

#endif
