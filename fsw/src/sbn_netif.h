<<<<<<< HEAD
/******************************************************************************
** File: sbn_netif.h
**
**  Copyright © 2007-2014 United States Government as represented by the 
**  Administrator of the National Aeronautics and Space Administration. 
**  All Other Rights Reserved.  
**
**  This software was created at NASA's Goddard Space Flight Center.
**  This software is governed by the NASA Open Source Agreement and may be 
**  used, distributed and modified only pursuant to the terms of that 
**  agreement.
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


/*
** Defines
*/
#define SBN_IPv4 1

int32 SBN_ParseFileEntry(char *FileEntry, uint32 LineNum);
int32 SBN_InitPeerInterface(void);
int   SBN_CreateSocket(int port);
void  SBN_ClearSocket(int SockID);
int32 SBN_CheckForNetProtoMsg(uint32 PeerIdx);
void  SBN_CheckForNetAppMsgs(void);
int32 SBN_SendNetMsg(uint32 MsgType, uint32 MsgSize, uint32 PeerIdx, CFE_SB_SenderId_t *SenderPtr);
int32 SBN_CopyFileData(uint32 PeerIdx,uint32 LineNum);

void  SBN_SendSockFailedEvent(uint32 Line, int RtnVal);
void  SBN_SendBindFailedEvent(uint32 Line, int RtnVal);

extern sbn_t  SBN;

#endif /* _sbn_netif_ */
/*****************************************************************************/
=======
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
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
