/******************************************************************************
** File: sbn_platform_cfg.h
**
**      Copyright (c) 2004-2016, United States government as represented by the
**      administrator of the National Aeronautics Space Administration.
**      All rights reserved. This software(cFE) was created at NASA's Goddard
**      Space Flight Center pursuant to government contracts.
**
**      This software may be used only pursuant to a United States government
**      sponsored project and the United States government may not be charged
**      for use thereof.
**
** Purpose:
**      This header file contains prototypes for private functions and type
**      definitions for the Software Bus Network Application.
**
** Authors:   J. Wilmot/GSFC Code582
**            R. McGraw/SSI
**            C. Knight/ARC Code TI
******************************************************************************/
#include "cfe.h"

#ifndef _sbn_platform_cfg_h
#define _sbn_platform_cfg_h

#define SBN_MAX_NETS                16
#define SBN_MAX_PEERS_PER_NET       32
#define SBN_MAX_SUBS_PER_PEER       256 

#define SBN_MAX_PEERNAME_LENGTH     32
#define SBN_MAX_NET_NAME_LENGTH     16

/* at most process this many SB messages per peer per wakeup */
#define SBN_MAX_MSG_PER_WAKEUP        32

#define SBN_MAIN_LOOP_DELAY           200 /* milli-seconds */

/* If I don't send out traffic for a period of time, call the module's poll
 * just so it can handle protocol matters.
 */
#define SBN_POLL_TIME        5

#define SBN_PEER_PIPE_DEPTH           64
#define SBN_DEFAULT_MSG_LIM           8

#define SBN_SUB_PIPE_DEPTH            256
#define SBN_MAX_ONESUB_PKTS_ON_PIPE   256
#define SBN_MAX_ALLSUBS_PKTS_ON_PIPE  64

#define SBN_VOL_PEER_FILENAME         "/ram/SbnPeerData.dat"
#define SBN_NONVOL_PEER_FILENAME      "/cf/SbnPeerData.dat"
#define SBN_PEER_FILE_LINE_SIZE       128
#define SBN_NETWORK_MSG_MARGIN        SBN_MAX_NETWORK_PEERS * 2  /* Double it to handle clock skews where a peer */
                                                             /* can send two messages in my single cycle */
#define SBN_VOL_MODULE_FILENAME       "/ram/SbnModuleData.dat"
#define SBN_NONVOL_MODULE_FILENAME    "/cf/SbnModuleData.dat"
#define SBN_MODULE_FILE_LINE_SIZE     128
#define SBN_MAX_INTERFACE_TYPES       8

#define SBN_SCH_PIPE_DEPTH            10

#define SBN_MOD_STATUS_MSG_SIZE       128 /* bytes */

/** \brief define this to use one task per peer pipe to send messages (each
 * task blocks on read). Otherwise, pipes will be polled periodically.
 */
#undef SBN_SEND_TASK

/** \brief define this to use one task per peer to receive messages (each
 * task blocks on read). Otherwise, another method (e.g. select) must be used
 * to prevent blocking.
 */
#undef SBN_RECV_TASK

#endif /* _sbn_platform_cfg_h_ */
