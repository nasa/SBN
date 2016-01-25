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
#define SBN_TIMEOUT_CYCLES            3
#define SBN_PEER_PIPE_DEPTH           64
#define SBN_DEFAULT_MSG_LIM           8
#define SBN_ITEMS_PER_FILE_LINE       6
#define SBN_MSG_BUFFER_SIZE           (SBN_PEER_PIPE_DEPTH * 2)

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

/*
** Event IDs
*/
#define SBN_INIT_EID                  1
#define SBN_MSGID_ERR_EID             2
#define SBN_FILE_ERR_EID              3
#define SBN_PROTO_INIT_ERR_EID        4
#define SBN_VOL_FAILED_EID            5
#define SBN_NONVOL_FAILED_EID         6
#define SBN_INV_LINE_EID              7
#define SBN_FILE_OPENED_EID           8
#define SBN_CPY_ERR_EID               9
#define SBN_PEERPIPE_CR_ERR_EID       10

#define SBN_SOCK_FAIL_EID             12
#define SBN_BIND_FAIL_EID             13
#define SBN_PEERPIPE_CR_EID           14
#define SBN_SND_APPMSG_ERR_EID        15
#define SBN_APPMSG_SENT_EID           16
#define SBN_PEER_ALIVE_EID            17
#define SBN_HB_LOST_EID               18
#define SBN_STATE_ERR_EID             19

#define SBN_MSG_TRUNCATED_EID         21
#define SBN_LSC_ERR1_EID              22
#define SBN_SUBTYPE_ERR_EID           23
#define SBN_NET_RCV_PROTO_ERR_EID     24
#define SBN_SRCNAME_ERR_EID           25
#define SBN_ANN_RCVD_EID              26
#define SBN_ANN_ACK_RCVD_EID          27
#define SBN_HB_RCVD_EID               28
#define SBN_HB_ACK_RCVD_EID           29
#define SBN_SUB_RCVD_EID              30
#define SBN_UNSUB_RCVD_EID            31
#define SBN_PEERIDX_ERR1_EID          32
#define SBN_SUB_ERR_EID               33
#define SBN_DUP_SUB_EID               34
#define SBN_PEERIDX_ERR2_EID          35
#define SBN_NO_SUBS_EID               36
#define SBN_SUB_NOT_FOUND_EID         37
#define SBN_SOCK_RCV_APP_ERR_EID      38
#define SBN_SB_SEND_ERR_EID           39
#define SBN_MSGTYPE_ERR_EID           40
#define SBN_LSC_ERR2_EID              41
#define SBN_ENTRY_ERR_EID             42
#define SBN_PEERIDX_ERR3_EID          43
#define SBN_UNSUB_CNT_EID             44
#define SBN_PROTO_SENT_EID            45
#define SBN_PROTO_SEND_ERR_EID        46

#define SBN_PIPE_ERR_EID              47
#define SBN_PEER_LIMIT_EID            48




#endif /* _sbn_constants_h_ */
/*****************************************************************************/
