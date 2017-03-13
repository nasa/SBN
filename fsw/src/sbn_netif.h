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

void SBN_ShowPeerData(void);
int32 SBN_GetPeerFileData(void);

int SBN_InitInterfaces(void);

#ifndef SBN_RECV_TASK
void SBN_RecvNetMsgs(void);
#endif /* !SBN_RECV_TASK */

#ifndef SBN_SEND_TASK
void SBN_CheckPeerPipes(void);
#endif /* !SBN_SEND_TASK */

int SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSize_t MsgSize,
    SBN_Payload_t *Msg, SBN_PeerInterface_t *Peer);

#endif /* _sbn_netif_ */
/*****************************************************************************/
