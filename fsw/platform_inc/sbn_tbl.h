#ifndef _sbn_tbl_h_
#define _sbn_tbl_h_

#include "cfe.h"

/****
 * @brief The RemapTbl defines, for a peer, which MID's should be remapped
 * (listing a "from" MID and a "to" MID, determining when a message is
 * published locally with the "from" MID, it is sent to the peer with the "to"
 * MID.)
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
    uint32 ProcessorID;

    /** @brief The local MID I'll receive from the pipe. */
    CFE_SB_MsgId_t FromMID;

    /** @brief The MID to send to the peer. If 0x0000, filter the from MID. */
    CFE_SB_MsgId_t ToMID;
}
SBN_RemapTblEntry_t;

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

    /** @brief The number of entries in the table (determined at runtime.) */
    uint32 EntryCnt;

    /** @brief The remapping entries. */
    SBN_RemapTblEntry_t Entries[SBN_REMAP_TABLE_SIZE];
}
SBN_RemapTbl_t;

/**
 * This function loads SBN tables (only one, the remap table, currently)
 * into memory, and validates the table's contents.
 * 
 * @param HandlePtr[in] The table handle to attach the table to.
 * @return SBN_SUCCESS upon successful loading.
 */
int32 SBN_LoadTbl(CFE_TBL_Handle_t *HandlePtr);

extern SBN_RemapTbl_t SBN_RemapTbl;

#endif /* _sbn_tbl_h_ */
