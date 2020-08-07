/******************************************************************************
** File: sbn_subs.h
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
**      This header file contains prototypes for private functions related to 
**      handling message subscriptions
**
** Authors:   J. Wilmot/GSFC Code582
**            R. McGraw/SSI
**            E. Timmons/GSFC Code587
**            C. Knight/ARC Code TI
**
******************************************************************************/

#ifndef _sbn_subs_h_
#define _sbn_subs_h_

#include "sbn_app.h"

SBN_Status_t SBN_SendLocalSubsToPeer(SBN_PeerInterface_t *Peer);
SBN_Status_t SBN_CheckSubscriptionPipe(void); 
SBN_Status_t SBN_ProcessSubsFromPeer(SBN_PeerInterface_t *Peer, void *submsg);
SBN_Status_t SBN_ProcessUnsubsFromPeer(SBN_PeerInterface_t *Peer, void *submsg);
SBN_Status_t SBN_ProcessAllSubscriptions(CFE_SB_AllSubscriptionsTlm_t *Ptr);
SBN_Status_t SBN_RemoveAllSubsFromPeer(SBN_PeerInterface_t *Peer);
SBN_Status_t SBN_SendSubsRequests(void);

#endif /* _sbn_subs_h_ */
