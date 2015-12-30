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
**
** $Log: sbn_netif.h  $
**
******************************************************************************/


#ifndef _sbn_netif_
#define _sbn_netif_

#include "cfe.h"

char *SBN_FindFileEntryAppData(char *entry, int num_fields);
int32 SBN_ParseFileEntry(char *FileEntry, uint32 LineNum);
int32 SBN_InitPeerInterface(void);
int32 SBN_CheckForNetProtoMsg(uint32 PeerIdx);
void inline SBN_ProcessNetAppMsgsFromHost(int HostIdx);
void  SBN_CheckForNetAppMsgs(void);
//int32 SBN_SendNetMsg(uint32 MsgType, uint32 MsgSize, uint32 PeerIdx, CFE_SB_SenderId_t *SenderPtr, uint8 IsRetransmit);
void  SBN_VerifyPeerInterfaces(void);
void  SBN_VerifyHostInterfaces(void);

uint8 inline SBN_GetReliabilityFromQoS(uint8 QoS);
uint8 inline SBN_GetPriorityFromQoS(uint8 QoS);
uint8 inline SBN_GetPeerQoSReliability(const SBN_PeerData_t * peer);
uint8 inline SBN_GetPeerQoSPriority(const SBN_PeerData_t * peer);
int SBN_ComparePeerQoS(const void * peer1, const void * peer2);

extern sbn_t  SBN;

#endif /* _sbn_netif_ */
/*****************************************************************************/
