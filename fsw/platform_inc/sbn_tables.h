#ifndef _sbn_tables_h_
#define _sbn_tables_h_

#include "cfe.h"

/****
 * The RemapTable must be sorted and unique for ProcessorID + from
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
    uint32 ProcessorID;
    CFE_SB_MsgId_t from;
    CFE_SB_MsgId_t to;
}
SBN_RemapTableEntry_t;

#define SBN_REMAP_TABLE_SHARENAME "SBN.RemapTable"

#define SBN_REMAP_DEFAULT_IGNORE 0
#define SBN_REMAP_DEFAULT_SEND 1
#define SBN_REMAP_TABLE_SIZE 256

typedef struct
{
    uint32 RemapDefaultFlag;
    uint32 Entries;
    SBN_RemapTableEntry_t Entry[SBN_REMAP_TABLE_SIZE];
}
SBN_RemapTable_t;

int32 SBN_LoadTables(CFE_TBL_Handle_t *HandlePtr);

extern SBN_RemapTable_t SBN_RemapTable;

#endif /* _sbn_tables_h_ */
