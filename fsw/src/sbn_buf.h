/**********************************************************
**
** File: sbn_buf.h
**
** Author: ejtimmon
** Date: 02 Sep 2015
**
** Purpose:
**    This file contains utility functions for using the
**    various buffers included in peers and hosts.
**********************************************************/

#ifndef _sbn_buf_h_
#define _sbn_buf_h_

#include "cfe.h"
#include "sbn_app.h"

int32 SBN_InitMsgBuf(SBN_PeerMsgBuf_t* buffer);
int32 SBN_AddMsgToMsgBufOverwrite(NetDataUnion* SentMsg, SBN_PeerMsgBuf_t* buffer);
int32 SBN_AddMsgToMsgBufSend(NetDataUnion* Msg, SBN_PeerMsgBuf_t* buffer, 
        int32 PeerIdx);
int32 SBN_ClearMsgBuf(SBN_PeerMsgBuf_t* buffer);
int32 SBN_ClearMsgBufBeforeSeq(SBN_NetProtoMsg_t* Ack, SBN_PeerMsgBuf_t* buffer);
int32 SBN_SendConsecutiveFromBuf(SBN_PeerMsgBuf_t* buffer, int32 seq, 
        int32 PeerIdx);
int32 SBN_RetransmitSeq(SBN_PeerMsgBuf_t* buffer, int32 seq, int32 PeerIdx);

#endif /* _sbn_buf_h_ */

