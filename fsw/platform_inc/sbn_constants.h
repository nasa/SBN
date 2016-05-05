/******************************************************************************
** File: sbn_app.h
**
**      Copyright (c) 2004-2006, United States government as represented by the
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
**
** $Log: sbn_app.h  $
** Revision 1.3 2010/10/05 15:24:14EDT jmdagost 
** Cleaned up copyright symbol.
** Revision 1.2 2008/04/08 08:07:10EDT ruperera 
** Member moved from sbn_app.h in project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/cfs_sbn.pj to sbn_app.h in project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/fsw/src/project.pj.
** Revision 1.1 2008/04/08 07:07:10ACT ruperera 
** Initial revision
** Member added to project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/cfs_sbn.pj
** Revision 1.1 2007/11/30 17:18:03EST rjmcgraw 
** Initial revision
** Member added to project d:/mksdata/MKS-CFS-REPOSITORY/sbn/cfs_sbn.pj
** Revision 1.14 2007/04/19 15:52:53EDT rjmcgraw 
** DCR3052:7 Moved and renamed CFE_SB_MAX_NETWORK_PEERS to this file
** Revision 1.13 2007/03/13 13:18:50EST rjmcgraw 
** Added #define SBN_ITEMS_PER_FILE_LINE
** Revision 1.12 2007/03/07 11:07:23EST rjmcgraw 
** Removed references to channel
** Revision 1.11 2007/02/28 15:05:03EST rjmcgraw 
** Removed unused #defines
** Revision 1.10 2007/02/27 09:37:42EST rjmcgraw 
** Minor cleanup
** Revision 1.9 2007/02/23 14:53:41EST rjmcgraw 
** Moved subscriptions from protocol port to data port
** Revision 1.8 2007/02/22 15:15:50EST rjmcgraw 
** Changed internal structs to exclude mac address
** Revision 1.7 2007/02/21 14:49:39EST rjmcgraw 
** Debug events changed to OS_printfs
**
******************************************************************************/
#include "cfe.h"

#ifndef _sbn_constants_h_
#define _sbn_constants_h_

#define SBN_OK                        0
#define SBN_ERROR                     (-1)
#define SBN_IF_EMPTY                  (-2)
#define SBN_NOT_IMPLEMENTED           (-3)
#define SBN_IPv4                      1
#define SBN_FALSE                     0
#define SBN_TRUE                      1
#define SBN_VALID                     1
#define SBN_NOT_VALID                 0

#define SBN_NOT_IN_USE                0
#define SBN_IN_USE                    1

#define SBN_MSGID_FOUND               0
#define SBN_MSGID_NOT_FOUND           1

#define SBN_MAX_MSG_SIZE              1400
#define SBN_MAX_SUBS_PER_PEER         256 
#define SBN_MAX_PEERNAME_LENGTH       8
#define SBN_DONT_CARE                 0
#define SBN_MAX_PEER_PRIORITY         16

/* at most process this many SB messages per peer per wakeup */
#define SBN_MAX_MSG_PER_WAKEUP        32

#define SBN_ETHERNET                  0
#define SBN_IPv4                      1
#define SBN_IPv6                      2  /* not implemented */
#define SBN_SPACEWIRE_RMAP            3  /* not implemented */
#define SBN_SPACEWIRE_PKT             4  /* not implemented */
#define SBN_SHMEM                     5
#define SBN_SERIAL                    6
#define SBN_1553                      7  /* not implemented */
#define SBN_NA_BUS                    (-1)

#define SBN_MAIN_LOOP_DELAY           200 /* milli-seconds */
/* A peer can either be disconnected (and we have it marked as "ANNOUNCING")
 * or it is connected and we expect traffic or if we don't see any in a period
 * of time, we send a heartbeat to see if the peer is alive.
 */
/* How many seconds to wait between announce messages. */
#define SBN_ANNOUNCE_TIMEOUT          10
/* If I don't send out traffic for a period of time, send out a heartbeat
 * so that my peer doesn't think I've disappeared. (This should be shorter
 * than the SBN_HEARTBEAT_TIMEOUT below!)
 */
#define SBN_HEARTBEAT_SENDTIME        5
/* How many seconds since I last saw a message from my peer do I mark it as
 * timed out?
 */
#define SBN_HEARTBEAT_TIMEOUT         10
#define SBN_PEER_PIPE_DEPTH           64
#define SBN_DEFAULT_MSG_LIM           8
#define SBN_ITEMS_PER_FILE_LINE       6
#define SBN_MSG_BUFFER_SIZE           (SBN_PEER_PIPE_DEPTH * 2) /* uint8 */

/* Interface Roles */
#define SBN_HOST         1
#define SBN_PEER         2

/* SBN States */
#define SBN_INIT                      0
#define SBN_ANNOUNCING                1
#define SBN_PEER_SYNC                 2
#define SBN_HEARTBEATING              3

/* Sync word */
#define SBN_SYNC_LENGTH               4
#define SBN_SYNC_1                    0x6A
#define SBN_SYNC_2                    0x89
#define SBN_SYNC_3                    0x98
#define SBN_SYNC_4                    0xB9

#endif /* _sbn_constants_h_ */
/*****************************************************************************/
