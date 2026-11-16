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

#ifndef SBN_RESOURCE_H
#define SBN_RESOURCE_H

#include "sbn_interfaces.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* SBN application data structures                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * The actual content of the SBN NetRecord struct
 *
 * This contains additional state info which is visible only to SBN itself,
 * it is not shared with modules.
 */
struct SBN_NetRecord
{
    SBN_NetId_t NetId; /**< Tracks allocation status for this net */

    SBN_NetInterface_t Intf; /**< This struct is passed to modules and is part of the SBN-Module interface */
};

/**
 * The actual content of the SBN PeerRecord struct
 *
 * This contains additional state info which is visible only to SBN itself,
 * it is not shared with modules.
 */
struct SBN_PeerRecord
{
    SBN_PeerId_t PeerId; /**< Tracks allocation status for this peer */
    SBN_NetId_t  NetId;  /**< The net associated with this peer */

    SBN_PeerInterface_t Intf; /**< This struct is passed to modules and is part of the SBN-Module interface */
};

/**
 * @brief Function to use with SBN_ForEachConfiguredPeer()
 *
 * A function to be called for every peer
 */
typedef SBN_Status_t (*SBN_PeerFunc_t)(SBN_PeerRecord_t *PeerRec, void *Arg);

/**
 * @brief Function to use with SBN_ForEachConfiguredNet()
 *
 * A function to be called for every net
 */
typedef SBN_Status_t (*SBN_NetFunc_t)(SBN_NetRecord_t *NetRec, void *Arg);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Gets a printable index from a NetId
 *
 * Returns an integer value that identifies the net in a backward-compatible way
 *
 * @returns integer representation of NetId
 */
int SBN_NetId_AsInt(SBN_NetId_t NetId);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Gets a printable index from a PeerId
 *
 * Returns an integer value that identifies the peer in a backward-compatible way
 *
 * @returns integer representation of PeerId
 */
int SBN_PeerId_AsInt(SBN_PeerId_t PeerId);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Check if an net record is in use or free/empty
 *
 * This routine checks if the net table entry is in use or if it is free
 *
 * @param[in]   NetRecPtr   pointer to app table entry
 * @returns true if the entry is in use/configured, or false if it is free/empty
 */
static inline bool SBN_NetRecordIsUsed(const SBN_NetRecord_t *NetRecPtr)
{
    return CFE_RESOURCEID_TEST_DEFINED(NetRecPtr->NetId);
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Get the ID value from an app table entry
 *
 * This routine converts the table entry back to an abstract ID.
 *
 * @param[in]   NetRecPtr   pointer to app table entry
 * @returns NetId of entry
 */
static inline SBN_NetId_t SBN_NetRecordGetID(const SBN_NetRecord_t *NetRecPtr)
{
    return NetRecPtr->NetId;
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Marks a net record entry as used (not free)
 *
 * This sets the internal field(s) within this entry, and marks
 * it as being associated with the given net ID.
 *
 * @param[in]   NetRecPtr   pointer to app table entry
 * @param[in]   PendingId   the net ID of this entry
 */
static inline void SBN_NetRecordSetUsed(SBN_NetRecord_t *NetRecPtr, CFE_ResourceId_t PendingId)
{
    NetRecPtr->NetId = SBN_NETID_C(PendingId);
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Set a net record table entry free (not used)
 *
 * This clears the internal field(s) within this entry, and allows the
 * memory to be re-used in the future.
 *
 * @param[in]   NetRecPtr   pointer to app table entry
 */
static inline void SBN_NetRecordSetFree(SBN_NetRecord_t *NetRecPtr)
{
    NetRecPtr->NetId = SBN_NETID_UNDEFINED;
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Check if a net record is a match for the given NetId
 *
 * This routine confirms that the previously-located record is valid
 * and matches the expected net ID.
 *
 * This function may be used in conjunction with SBN_LocateNetRecordByID()
 * to confirm that the located record is a positive match to the expected ID.
 * As such, the record pointer is also permitted to be NULL, to alleviate the
 * need for the caller to handle this possibility explicitly.
 *
 * Once a record pointer has been successfully validated using this routine,
 * it may be safely passed to all other internal functions.
 *
 * @sa SBN_LocateNetRecordByID
 *
 * @param[in]   NetRecPtr   pointer to net table entry, or NULL
 * @param[in]   NetId       expected net ID
 * @returns true if the entry matches the given net ID
 */
static inline bool SBN_NetRecordIsMatch(const SBN_NetRecord_t *NetRecPtr, SBN_NetId_t NetId)
{
    return (NetRecPtr != NULL && CFE_RESOURCEID_TEST_EQUAL(NetRecPtr->NetId, NetId));
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Check if a peer record is in use or free/empty
 *
 * This routine checks if the peer record entry is in use or if it is free
 *
 * @param[in]   PeerRecPtr   pointer to peer record entry
 * @returns true if the entry is in use/configured, or false if it is free/empty
 */
static inline bool SBN_PeerRecordIsUsed(const SBN_PeerRecord_t *PeerRecPtr)
{
    return CFE_RESOURCEID_TEST_DEFINED(PeerRecPtr->PeerId);
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Get the ID value from a peer record entry
 *
 * This routine converts the table entry back to an abstract ID.
 *
 * @param[in]   PeerRecPtr   pointer to peer record entry
 * @returns PeerId of entry
 */
static inline SBN_PeerId_t SBN_PeerRecordGetID(const SBN_PeerRecord_t *PeerRecPtr)
{
    return PeerRecPtr->PeerId;
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Marks a peer record entry as used (not free)
 *
 * This sets the internal field(s) within this entry, and marks
 * it as being associated with the given peer ID.
 *
 * @param[in]   PeerRecPtr   pointer to peer record entry
 * @param[in]   PendingId   the peer ID of this entry
 */
static inline void SBN_PeerRecordSetUsed(SBN_PeerRecord_t *PeerRecPtr, CFE_ResourceId_t PendingId)
{
    PeerRecPtr->PeerId = SBN_PEERID_C(PendingId);
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Set a peer record table entry free (not used)
 *
 * This clears the internal field(s) within this entry, and allows the
 * memory to be re-used in the future.
 *
 * @param[in]   PeerRecPtr   pointer to peer record entry
 */
static inline void SBN_PeerRecordSetFree(SBN_PeerRecord_t *PeerRecPtr)
{
    PeerRecPtr->PeerId = SBN_PEERID_UNDEFINED;
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Check if a peer record is a match for the given PeerId
 *
 * This routine confirms that the previously-located record is valid
 * and matches the expected peer ID.
 *
 * As this dereferences fields within the record, global data must be
 * locked prior to invoking this function.
 *
 * This function may be used in conjunction with SBN_LocatePeerRecordByID()
 * to confirm that the located record is a positive match to the expected ID.
 * As such, the record pointer is also permitted to be NULL, to alleviate the
 * need for the caller to handle this possibility explicitly.
 *
 * Once a record pointer has been successfully validated using this routine,
 * it may be safely passed to all other internal functions.
 *
 * @sa SBN_LocatePeerRecordByID
 *
 * @param[in]   PeerRecPtr   pointer to peer record entry
 * @param[in]   PeerId       expected peer ID
 * @returns true if the entry matches the given peer ID
 */
static inline bool SBN_PeerRecordIsMatch(const SBN_PeerRecord_t *PeerRecPtr, SBN_PeerId_t PeerId)
{
    return (PeerRecPtr != NULL && CFE_RESOURCEID_TEST_EQUAL(PeerRecPtr->PeerId, PeerId));
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Access the spacecraft ID from a Peer Record
 *
 * Simplifies the access to the spacecraft ID, often used for logging
 * Returned as an int for use with "%d" specifier in printf-style calls
 */
static inline int SBN_Peer_GetSpacecraftID(const SBN_PeerRecord_t *PeerRecPtr)
{
    return (int)PeerRecPtr->Intf.SpacecraftID;
}

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Access the processor ID from a Peer Record
 *
 * Simplifies the access to the processor ID, often used for logging
 * Returned as an int for use with "%d" specifier in printf-style calls
 */
static inline int SBN_Peer_GetProcessorID(const SBN_PeerRecord_t *PeerRecPtr)
{
    return (int)PeerRecPtr->Intf.ProcessorID;
}

/*
 * Availability check functions used in conjunction with CFE_ResourceId_FindNext()
 */

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Checks if net record slot is currently used
 *
 * Helper function, Aids in allocating a new ID by checking if
 * a given ID is available.
 *
 * @returns false if slot is unused/available, true if used/unavailable
 */
bool SBN_CheckNetIdSlotUsed(CFE_ResourceId_t CheckId);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Checks if peer record slot is currently used
 *
 * Helper function, Aids in allocating a new ID by checking if
 * a given ID is available.
 *
 * @returns false if slot is unused/available, true if used/unavailable
 */
bool SBN_CheckPeerIdSlotUsed(CFE_ResourceId_t CheckId);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Finds the slot corresponding to a peer ID
 *
 * Finds the slot which will hold the given ID, if it is configured
 * Note this does not check the validity of the ID, it only finds where
 * the record is supposed to be.
 *
 * @param[in]   PeerId       expected peer ID
 *
 * @returns Pointer to record
 * @retval NULL on error
 */
SBN_PeerRecord_t *SBN_LocatePeerRecordByID(SBN_PeerId_t PeerId);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Finds the slot corresponding to a net ID
 *
 * Finds the slot which will hold the given ID, if it is configured
 * Note this does not check the validity of the ID, it only finds where
 * the record is supposed to be.
 *
 * @param[in]   NetId       expected net ID
 *
 * @returns Pointer to record
 * @retval NULL on error
 */
SBN_NetRecord_t *SBN_LocateNetRecordByID(SBN_NetId_t NetId);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Registers a new peer
 *
 * Allocates a free record from the table and assigns a new ID
 *
 * @returns Status code
 * @retval  SBN_SUCCESS if successful
 */
SBN_Status_t SBN_RegisterPeer(SBN_PeerId_t *PeerIdOut);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Registers a new net
 *
 * Allocates a free record from the table and assigns a new ID
 *
 * @returns Status code
 * @retval  SBN_SUCCESS if successful
 */
SBN_Status_t SBN_RegisterNet(SBN_NetId_t *NetIdOut);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Finds a net interface using a 0-based array index
 *
 * Backward compatibility helper for existing code that uses
 * a 0-based index value to identify a net.  This replaces old
 * refs to e.g. `SBN_AppData.Nets[Idx]`
 *
 * @returns Pointer to Net Interface struct
 */
SBN_NetRecord_t *SBN_LocateNetRecordByExtIdx(SBN_NetIdx_t Idx);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Finds a peer interface using a 0-based array index
 *
 * Backward compatibility helper for existing code that uses
 * a 0-based index value to identify a peer within a net.  This
 * replaces old refs to e.g. `Net->Peers[Idx]`
 *
 * @returns Pointer to Peer Interface struct
 */
SBN_PeerRecord_t *SBN_LocatePeerRecordInNet(SBN_NetRecord_t *NetRec, SBN_PeerIdx_t Idx);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Call the user-supplied function for every peer
 *
 * @param Func The function to call
 * @param Arg  Opaque argument passed to function
 *
 * @returns First non-zero status code from helper function
 */
SBN_Status_t SBN_ForEachConfiguredPeer(SBN_PeerFunc_t Func, void *Arg);

/*---------------------------------------------------------------------------------------*/
/**
 * @brief Call the user-supplied function for every net
 *
 * @param Func The function to call
 * @param Arg  Opaque argument passed to function
 *
 * @returns First non-zero status code from helper function
 */
SBN_Status_t SBN_ForEachConfiguredNet(SBN_NetFunc_t Func, void *Arg);

#endif /* SBN_RESOURCE_H */
