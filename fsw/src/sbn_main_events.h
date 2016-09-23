/************************************************************************
**   sbn_events.h
**   
**   2014/11/21 ejtimmon
**
**   Specification for the Software Bus Network event identifers.
**
*************************************************************************/

#ifndef _sbn_main_events_h_
#define _sbn_main_events_h_

#include "sbn_events.h"

#define SBN_SB_EID           FIRST_SBN_EID + 0
#define SBN_INIT_EID           FIRST_SBN_EID + 1
#define SBN_MSG_EID             FIRST_SBN_EID + 2
#define SBN_FILE_EID              FIRST_SBN_EID + 3
#define SBN_PEER_EID       FIRST_SBN_EID + 4
#define SBN_PROTO_EID             FIRST_SBN_EID + 5
#define SBN_CMD_EID              FIRST_SBN_EID + 6
#define SBN_SUB_EID           FIRST_SBN_EID + 7

#endif /* _sbn_main_events_h_ */
