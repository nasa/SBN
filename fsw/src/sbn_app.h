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
******************************************************************************/

#ifndef _sbn_app_
#define _sbn_app_

#include "osconfig.h"
#include "cfe.h"
#include "sbn_interfaces.h"
#include "sbn_msg.h"
#include "sbn_platform_cfg.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"

typedef struct {
  SBN_InterfaceData IfData[SBN_MAX_NETWORK_PEERS*2];  /* Data on all devices in the peer file (allow a host for every peer) */
  SBN_InterfaceData *Host[SBN_MAX_NETWORK_PEERS];   /* Data only on devices that are the host */
  SBN_PeerData_t    Peer[SBN_MAX_NETWORK_PEERS];    /* Data only no devices that are not the host */
  uint32            AppId;
  char              App_FullName[(OS_MAX_API_NAME * 2)];
  CFE_SB_PipeId_t   SubPipe;
  CFE_SB_PipeId_t   CmdPipe;

  SBN_Subs_t        LocalSubs[SBN_MAX_SUBS_PER_PEER + 1];

  SBN_InterfaceOperations *IfOps[SBN_MAX_INTERFACE_TYPES + 1];

  SBN_HkPacket_t   Hk;
}sbn_t;

sbn_t SBN;

/*
** Prototypes
*/
void SBN_AppMain(void);
int SBN_CreatePipe4Peer(int PeerIdx);
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
