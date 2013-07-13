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