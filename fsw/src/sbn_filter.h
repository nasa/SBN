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

#ifndef SBN_FILTER_H
#define SBN_FILTER_H

/*
 ** Include Files
 */
#include "cfe_msg_hdr.h"
#include "sbn_resource.h"
#include "sbn_interfaces.h"

/**
 * @brief Values for direction parameter of filter functions.
 */
typedef enum
{
    SBN_Filter_Direction_INVALID  = 0, /* reserved value, not used */
    SBN_Filter_Direction_INBOUND  = 1, /* aka receive */
    SBN_Filter_Direction_OUTBOUND = 2, /* aka send */
} SBN_Filter_Direction_Enum_t;

/**
 * @brief Applies the Gatekeeper filter(s) on a message to/from a given peer
 *
 * A gatekeeper filter accepts a direct message pointer does not modify the message.
 * It determines whether the message should be passed through or dropped.  If the message
 * is to be dropped/blocked, then no other processing work needs to be performed.
 *
 * @param[in] MsgPtr   Pointer to message (read-only)
 * @param[in] Peer     Pointer to peer interface for this message
 * @param[in] Dir      The direction of message flow
 *
 * @retval  #SBN_SUCCESS if the message passed the filter and should be forwarded
 * @returns Any other non-success status code to block the message
 */
SBN_Status_t
SBN_Filter_Gatekeeper(const CFE_MSG_Message_t *MsgPtr, SBN_PeerInterface_t *Peer, SBN_Filter_Direction_Enum_t Dir);

/**
 * @brief Applies the Mutation filter(s) on a message to/from a given peer
 *
 * A mutation filter accepts a full buffer descriptor which is read/write.
 * It can perform any needed modifications to the message as it passes through SBN.
 * Typically this would be for remapping the MsgIDs in configurations where the
 * peers do not share the same MsgID address space (i.e a form of NAT for MsgIDs).
 *
 * The buffer content will be a message header, same as with a gatekeeper filter, but
 * as a modifiable copy in a local buffer rather than the original message pointer.
 *
 * This filter can still choose to drop/block the message in which case it will not
 * be forwarded to the peer.  However, this less efficient than blocking in a
 * gatekeeper since the message has been processed to some extent.  Whenever possible,
 * message blocking should be performed in gatekeeper filters, not as a mutation.
 *
 * This provides compatibility with the existing/historical filter API where it
 * did not distinguish between simple pass/block checks vs. modifications.
 *
 * @param[inout] BufD     Pointer to buffer, content may be modified
 * @param[in]    Peer     Pointer to peer interface for this message
 * @param[in]    Dir      The direction of message flow

 * @retval  #SBN_SUCCESS if the message passed the filter and should be forwarded
 * @returns Any other non-success status code to block the message
 */
SBN_Status_t SBN_Filter_Mutation(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer, SBN_Filter_Direction_Enum_t Dir);

/**
 * @brief Applies the Remap filter(s) on a message to/from a given peer
 *
 * A remap filter operates specifically on a MsgID, providing a NAT-like service
 * for SBN peers.  This differs from the standard mutation filter in that it
 * strictly focuses on the MsgID and the message ID translations, as opposed to
 * a message buffer.
 *
 * This is used to translate the MsgIDs within subscribe/unsubscribe protocol
 * messages using the same translations that will be used for message filtering.
 *
 * @param[inout] MsgIdPtr Pointer to MsgID, content may be modified
 * @param[in]    Peer     Pointer to peer interface for this message
 *
 * @retval  #SBN_SUCCESS if the message passed the filter and should be forwarded
 * @returns Any other non-success status code to block the message
 */
SBN_Status_t SBN_Filter_Remap(CFE_SB_MsgId_t *MsgIdPtr, SBN_PeerInterface_t *Peer);

#endif
