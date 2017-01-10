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
**            C. Knight/ARC Code TI
**
******************************************************************************/

#ifndef _sbn_app_
#define _sbn_app_

#include "osconfig.h"
#include "cfe.h"
#include "sbn_interfaces.h"
#include "sbn_msg.h"
#include "sbn_platform_cfg.h"
#include "sbn_tables.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* SBN application data structures                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * \brief SBN global data structure definition
 */
typedef struct
{
    /** \brief Data only on host definitions. */
    SBN_HostInterface_t Hosts[SBN_MAX_NETWORK_PEERS];

    /** \brief Data only no devices that are not the host */
    SBN_PeerInterface_t Peers[SBN_MAX_NETWORK_PEERS];

    /** \brief The application ID provided by ES */
    uint32 AppId;

    /** \brief The application full name provided by SB */
    char App_FullName[(OS_MAX_API_NAME * 2)];

    /**
     * \brief The subscription pipe used to monitor local subscriptions.
     */
    CFE_SB_PipeId_t SubPipe;

    /**
     * \brief The command pipe used to receive commands.
     */
    CFE_SB_PipeId_t CmdPipe;

    /**
     * \brief All subscriptions by apps connected to the SB.
     *
     * When a peer and I connect, I send that peer all subscriptions I have
     * and they send me theirs. All messages on the local bus that are
     * subscribed to by the peer are sent over, and vice-versa.
     */
    SBN_Subs_t LocalSubs[SBN_MAX_SUBS_PER_PEER + 1];
    /** \brief The number of entries in the LocalSubs array. */
    int LocalSubCnt;

    /** \brief CFE scheduling pipe */
    CFE_SB_PipeId_t SchPipeId;

    /**
     * Each SBN back-end module provides a number of functions to
     * implement the protocols to connect peers.
     */
    SBN_InterfaceOperations_t *IfOps[SBN_MAX_INTERFACE_TYPES + 1];

    /** \brief Housekeeping information. */
    SBN_HkPacket_t Hk;

    CFE_TBL_Handle_t TableHandle;
    SBN_RemapTable_t *RemapTable;
} SBN_App_t;

/**
 * \brief SBN glocal data structure reference.
 */
extern SBN_App_t SBN;

/*
** Prototypes
*/
void SBN_AppMain(void);
void SBN_ProcessNetMsg(SBN_MsgType_t MsgType, SBN_CpuId_t CpuId,
    SBN_MsgSize_t MsgSize, void *Msg);
int SBN_GetPeerIndex(uint32 ProcessorId);

#ifdef SBN_DEBUG_MSGS
#define DEBUG_MSG(...) CFE_EVS_SendEvent(SBN_DEBUG_EID, CFE_EVS_DEBUG, __VA_ARGS__)
#define DEBUG_START() CFE_EVS_SendEvent(SBN_DEBUG_EID, CFE_EVS_DEBUG, "%s starting", __FUNCTION__)
#else /* !SBN_DEBUG_MSGS */
#define DEBUG_START() ;
#define DEBUG_MSG(...) ;
#endif /* SBN_DEBUG_MSGS */

#endif /* _sbn_app_ */
/*****************************************************************************/
