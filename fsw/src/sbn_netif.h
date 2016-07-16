/******************************************************************************
** File: sbn_netif.h
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


#ifndef _sbn_netif_
#define _sbn_netif_

#include "cfe.h"

void  SBN_ShowPeerData(void);
int32 SBN_GetPeerFileData(void);

int SBN_InitInterfaces(void);

void  SBN_RecvNetMsgs(void);

int SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSize_t MsgSize,
    SBN_Payload_t *Msg, int PeerIdx);

void  SBN_VerifyPeers(void);
void  SBN_VerifyHosts(void);

uint8 SBN_GetReliabilityFromQoS(uint8 QoS);
uint8 SBN_GetPriorityFromQoS(uint8 QoS);
uint8 SBN_GetPeerQoSReliability(const SBN_PeerData_t * peer);
uint8 SBN_GetPeerQoSPriority(const SBN_PeerData_t * peer);

#endif /* _sbn_netif_ */
/*****************************************************************************/
