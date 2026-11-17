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

#ifndef SBN_SUBS_H
#define SBN_SUBS_H

#include "sbn_interfaces.h"
#include "cfe_sb_msg.h"

/* This is the mask applied when accessing ring buffer entries based on version. */
#define SBN_SUBSCRIPTION_UPDATE_QUEUE_MASK (SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH - 1)

/**
 * Tracks a singular subscription change event
 */
typedef struct SBN_SubChange
{
    bool IsSubscribe; /**< true if subscription, false if unsubscription */

    CFE_SB_Qos_t   QoS;
    CFE_SB_MsgId_t MsgId;

} SBN_SubChange_t;

/**
 * Tracks a series of subscription change events (ring buffer)
 *
 * This allows the local changes to be incrementally sent to the peer,
 * or peer updates to be incrementally applied locally.
 */
typedef struct SBN_SubJournal
{
    /**
     * This field indicates the running total number of updates
     * For peers:
     *  - It is updated by the task that handles incoming peer messages.
     *  - It is read by the main task to identify when the local pipe needs a change.
     * For local subs:
     *  - It is updated by the main task when the update message from SB comes in
     *  - It is read by the thread that sends messages to the peer to send updates.
     */
    volatile uint32 CurrVersion;
    SBN_SubChange_t Entries[SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH];

} SBN_SubJournal_t;

/*---------------------------------------------------------------------------*/
/**
 * @brief Gets a historical subscription change event from the journal
 *
 * Returns a pointer to the entry, which indicates what changed in that
 * specific version update.
 *
 * @param[inout] Journal   Pointer to the subscription tracking journal
 * @param[in]    Version   The historical version identifier
 *
 * @returns Pointer to entry, if valid
 * @retval  NULL if entry is not in the journal (e.g. too old)
 */
SBN_SubChange_t *SBN_Subs_GetPrevChangeEntry(SBN_SubJournal_t *Journal, uint32 Version);

/*---------------------------------------------------------------------------*/
/**
 * @brief Adds a subscription change event to the journal
 *
 * Appends a new change entry to the subscription journal
 *
 * @param[inout] Journal     Pointer to the subscription tracking journal
 * @param[in]    IsSubscribe True for subscription events, False for unsubscription events
 * @param[in]    MsgId       The subject MsgId
 * @param[in]    QoS         The QoS (placeholder; QoS not currently implemented in SB)
 */
void SBN_Subs_RegisterChange(SBN_SubJournal_t *Journal, bool IsSubscribe, CFE_SB_MsgId_t MsgId, CFE_SB_Qos_t QoS);

/*---------------------------------------------------------------------------*/
/**
 * @brief Sends all local subscriptions over the wire to a peer.
 *
 * @param[inout] PeerRec  The local peer record.
 * @param[inout] BufD     The buffer to use for creating protocol message(s)
 *
 * @returns SBN status code
 * @retval  #SBN_SUCCESS if successful
 */
SBN_Status_t SBN_Subs_SendLocalToPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);

/*---------------------------------------------------------------------------*/
/**
 * @brief Maintain subscriptions
 *
 * This is the top-level function for maintaining subscriptions between peers.
 * It is invoked during the normal periodic polling cycle
 *
 * @returns SBN status code
 * @retval #SBN_SUCCESS if subscriptions received
 * @retval #SBN_IF_EMPTY if no subs recvd
 */
SBN_Status_t SBN_Subs_PollCycle(void);

/*---------------------------------------------------------------------------*/
/**
 * @brief Process a subscription update message from a peer.
 *
 * This handles both subscription and unsubscription messages, as the format is bascially the same.
 *
 * @param[inout] PeerRec     The local peer record.
 * @param[inout] BufD        The buffer containing the protocol message
 *
 * @retval #SBN_SUCCESS on successfully handling all subscriptions from peer, otherwise SBN_ERROR
 */
SBN_Status_t SBN_Subs_ProcessMsgFromPeer(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);

/*---------------------------------------------------------------------------*/
/**
 * @brief Informs the software bus to send this application all subscription requests.
 * @retval #SBN_SUCCESS on success
 */
SBN_Status_t SBN_SendSubsRequests(void);

/*---------------------------------------------------------------------------*/
/**
 * @brief Creates the local SB pipe for the peer
 *
 * Creates the local pipe and applies all known subscriptions that the peer
 * has informed the local instance of.  On a new/initial connection this will
 * be none.
 *
 * @note If the pipe already exists, it will be deleted first.  Upon re-creating
 * the pipe, the known subscriptions are applied.  This allows a peer that has
 * gotten too far out of sync to fully re-establish the local state.
 *
 * @param[inout] PeerRec  The local peer record.
 */
void SBN_Subs_SetupLocalPipe(SBN_PeerRecord_t *PeerRec);

/*---------------------------------------------------------------------------*/
/**
 * @brief Generate subscription update messages to peer
 *
 * This applies all incremental updates that have not yet been applied.
 * For local subscriptions, it will propagate them to the peer
 *
 * @param[inout] PeerRec  The local peer record.
 * @param[inout] BufD     The buffer to use when creating the protocol message(s)
 */
void SBN_Subs_UpdateConnectedPeerRemoteSubs(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD);

/*---------------------------------------------------------------------------*/
/**
 * @brief Synchronize the local subscription state for a peer
 *
 * This applies all incremental updates that have not yet been applied.
 * For peer subscriptions, it will propagate them to the local pipe.
 *
 * @param[inout] PeerRec  The local peer record.
 */
void SBN_Subs_UpdateConnectedPeerLocalSubs(SBN_PeerRecord_t *PeerRec);

/*---------------------------------------------------------------------------*/
/**
 * @brief Reset the local subscription state for a disconnected peer
 *
 * unsubscribe from the local SB for the specified peer, particularly when the
 * peer connection has been lost.
 *
 * @param[inout] PeerRec  The local peer record.
 */
void SBN_Subs_UpdateDisconnectedPeerLocalSubs(SBN_PeerRecord_t *PeerRec);

/*---------------------------------------------------------------------------*/
/**
 * @brief Synchronize the subscription state for all peers
 *
 * Invokes SBN_Subs_UpdateConnectedPeerLocalSubs() or SBN_Subs_UpdateDisconnectedPeerLocalSubs()
 * for all configured peers, depending on connectivity.
 */
void SBN_Subs_SynchronizeAllPeers(void);

#endif /* _sbn_subs_h_ */
