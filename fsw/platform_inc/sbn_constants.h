/******************************************************************************
** File: sbn_constants.h
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

#ifndef _sbn_constants_h_
#define _sbn_constants_h_

/**
 * Below are constants that the user shouldn't have to change, but are useful
 * to know and are shared with modules.
 * If it's a compile-time configuration option, it should be in
 * sbn_platform_cfg.h instead.
 */

#define SBN_SUCCESS                   0
#define SBN_ERROR                     (-1)
#define SBN_IF_EMPTY                  (-2)
#define SBN_NOT_IMPLEMENTED           (-3)

#define SBN_UDP                       1
#define SBN_TCP                       2
#define SBN_SPACEWIRE_RMAP            3
#define SBN_SPACEWIRE_PKT             4
#define SBN_SHMEM                     5
#define SBN_SERIAL                    6
#define SBN_1553                      7
#define SBN_DTN                       8

/* SBN States */
#define SBN_ANNOUNCING                0
#define SBN_HEARTBEATING              1

/* Message types definitions */
#define SBN_NO_MSG                    0x0000
#define SBN_ANNOUNCE_MSG              0x0010
#define SBN_HEARTBEAT_MSG             0x0020
#define SBN_SUBSCRIBE_MSG             0x0030
#define SBN_UN_SUBSCRIBE_MSG          0x0040
#define SBN_APP_MSG                   0x0050

#define SBN_IDENT           "$Id$"
#define SBN_IDENT_LEN       48 /**< \brief Id is always the same len, plus \0 */

#endif /* _sbn_constants_h_ */
/*****************************************************************************/
