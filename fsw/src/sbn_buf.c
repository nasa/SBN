/**********************************************************
**
** File: sbn_buf.c
**
** Author: ejtimmon
** Date: 02 Sep 2015
**
** Purpose:
**    This file contains utility functions for using the
**    various buffers included in peers and hosts.
**********************************************************/
#include "cfe.h"
#include "sbn_buf.h"
#include "sbn_netif.h"
#include <string.h>


/**
 * Initializes the specified message buffer.  Sets each entry in the buffer to
 * NULL.  Can be used with both the sent message buffer and the deferred 
 * message buffer.
 *
 * @param buffer   Buffer to initialize
 * @return SBN_OK
 */
int32 SBN_InitMsgBuf(SBN_PeerMsgBuf_t* buffer)
{
    return SBN_ClearMsgBuf(buffer);
} /* end SBN_InitMsgBuf */

/**
 * Adds the provided message to the specified message buffer.  If the buffer
 * is full and a new message must be added, the oldest message in the buffer 
 * is overwritten.
 * Can be used with both the sent message buffer and the deferred message
 * buffer.
 *
 * @param SentMsg  Message to add
 * @param buffer   Buffer to which to add the message
 * @return SBN_OK
 */
int32 SBN_AddMsgToMsgBufOverwrite(NetDataUnion* Msg, SBN_PeerMsgBuf_t* buffer)
{
    int32   addIdx = buffer->AddIndex;
 
    if(CFE_PSP_MemCpy(&buffer->Msgs[addIdx], Msg, sizeof(NetDataUnion))
	    != CFE_PSP_SUCCESS) return SBN_ERROR;

    /* if oldest message was overwritten, update OldestIndex */
    if((addIdx == buffer->OldestIndex) || (buffer->OldestIndex == -1))
        buffer->OldestIndex = (buffer->OldestIndex + 1) % SBN_MSG_BUFFER_SIZE;
    
    buffer->AddIndex = ((addIdx + 1) % SBN_MSG_BUFFER_SIZE);

    if(buffer->MsgCount != SBN_MSG_BUFFER_SIZE) buffer->MsgCount++;

    return SBN_OK;
} /* end SBN_AddMsgToMsgBufOverwrite */

/**
 * Adds the provided message to the specified message buffer.  If the buffer
 * is full and a new message must be added, the oldest message in the buffer 
 * is sent.
 * Can be used with both the sent message buffer and the deferred message
 * buffer.
 *
 * @param SentMsg  Message to add
 * @param buffer   Buffer to which to add the message
 * @param PeerIdx  Index of peer
 * @return SBN_OK
 */
int32 SBN_AddMsgToMsgBufSend(NetDataUnion* Msg, SBN_PeerMsgBuf_t* buffer, 
        int32 PeerIdx)
{
    int32   addIdx = buffer->AddIndex;
    uint32  NetMsgSize = 0;

    /* if oldest message wil be overwritten, send it and update OldestIndex */
    if(addIdx == buffer->OldestIndex)
    {
        NetMsgSize = buffer->Msgs[buffer->OldestIndex].Hdr.MsgSize;
        if(CFE_PSP_MemCpy(&SBN.MsgBuf, 
		&buffer->Msgs[buffer->OldestIndex], NetMsgSize)
		!= CFE_PSP_SUCCESS)
	    return SBN_ERROR;
        
        SBN_ProcessNetAppMsg(NetMsgSize);
        SBN.Peer[PeerIdx].RcvdCount++;

        buffer->OldestIndex = (buffer->OldestIndex + 1) % SBN_MSG_BUFFER_SIZE;
    } /* end if */
    
    NetMsgSize = Msg->Hdr.MsgSize;
    if(CFE_PSP_MemCpy(&buffer->Msgs[addIdx], Msg, NetMsgSize)
	    != CFE_PSP_SUCCESS)
	return SBN_ERROR;

    if(buffer->OldestIndex == -1)
        buffer->OldestIndex = (buffer->OldestIndex + 1) % SBN_MSG_BUFFER_SIZE;
    
    buffer->AddIndex = ((addIdx + 1) % SBN_MSG_BUFFER_SIZE);

    if(buffer->MsgCount != SBN_MSG_BUFFER_SIZE)
        buffer->MsgCount++;

    return SBN_OK;
} /* end SBN_AddMsgToMsgBufSend */
/**
 * Clears all messages out of the specified message buffer.  Sets each entry in
 * the buffer to NULL.  Can be used with both the sent message buffer and the
 * deferred message buffer.
 *
 * @param buffer  Buffer to clear
 * @return SBN_OK
 */
int32 SBN_ClearMsgBuf(SBN_PeerMsgBuf_t* buffer)
{
    if(CFE_PSP_MemSet(buffer, 0, sizeof(SBN_PeerMsgBuf_t)) != CFE_PSP_SUCCESS)
	return SBN_ERROR;

    buffer->AddIndex = 0;
    buffer->MsgCount = 0;
    buffer->OldestIndex = -1;
    return SBN_OK;
} /* end SBN_ClearMsgBuf */

/**
 * Clears all messages acked by the specified ack from the specified message
 * buffer.  Should be used only with the sent message buffer.
 *
 * @param Ack     Ack message being handled
 * @param buffer  Buffer to clear
 * @return SBN_OK
 */
int32 SBN_ClearMsgBufBeforeSeq(int seq, SBN_PeerMsgBuf_t* buffer)
{
    int32   idx = buffer->OldestIndex,
            oldest = buffer->OldestIndex,
            checked = 0,
            count = buffer->MsgCount;

    while(checked < count)
    {
        if(buffer->Msgs[idx].Hdr.SequenceCount <= seq)
        {
            if(CFE_PSP_MemSet(&buffer->Msgs[idx], 0, sizeof(NetDataUnion))
		    != CFE_PSP_SUCCESS)
		return SBN_ERROR;
            buffer->MsgCount--;

            /* if oldest index was cleared, increment it */
            if(idx == oldest)
                oldest = (oldest + 1) % SBN_MSG_BUFFER_SIZE;
        } /* end if */
    
        idx = ((idx + 1) % SBN_MSG_BUFFER_SIZE);

        checked++;
    } /* end while */

    buffer->OldestIndex = oldest;
    return SBN_OK;
} /* end SBN_ClearMsgBufBeforeSeq */

/**
 * Forwards and clears all messages that consecutively follow the specified 
 * sequence number.  Should be used only with the deferred message buffer.
 *
 * @param buffer  Deferred message buffer
 * @param seq     First sequence number
 * @param PeerIdx Index of peer
 * @return Number of deferred messages sent
 */
int32 SBN_SendConsecutiveFromBuf(SBN_PeerMsgBuf_t* buffer, int32 seq, 
        int32 PeerIdx)
{
    int32   idx = buffer->OldestIndex,
            currentSeq = seq + 1,
            oldest = buffer->OldestIndex,
            checked = 0,
            count = buffer->MsgCount,
            sent = 0;
    uint32  NetMsgSize;

    while(checked < count)
    {
        if(buffer->Msgs[idx].Hdr.SequenceCount == currentSeq)
        {
            NetMsgSize = buffer->Msgs[idx].Hdr.MsgSize;
            if(CFE_PSP_MemCpy(&SBN.MsgBuf, 
                    &buffer->Msgs[idx], NetMsgSize) != CFE_PSP_SUCCESS)
		return -1;
        
            SBN_ProcessNetAppMsg(NetMsgSize);
            SBN.Peer[PeerIdx].RcvdCount++;
            sent++;

            if(CFE_PSP_MemSet(&buffer->Msgs[idx], 0, NetMsgSize) != CFE_PSP_SUCCESS) return -1;
            buffer->MsgCount--;

            /* if oldest index was cleared, increment it */
            if(idx == oldest)
                oldest = (oldest + 1) % SBN_MSG_BUFFER_SIZE;
            currentSeq++;
        }
        else
        {
            /* don't leave gaps in the buffer - consecutive messages should
             * follow each other */
            if(sent > 0) break;
        } /* end if */

        idx = ((idx + 1) % SBN_MSG_BUFFER_SIZE);

        checked++;
    } /* end while */

    buffer->OldestIndex = oldest;
    return sent;
} /* end SBN_SendConsecutiveFromBuf */

/**
 * Retransmits the message with the specified sequence number up to a 
 * maximum number of retransmissions.
 *
 * @param buffer  Sent message buffer from which to retransmit
 * @param seq     Sequence number of message to retransmit
 * @param PeerIdx Index of peer
 * @return SBN_OK
 */
int32 SBN_RetransmitSeq(SBN_PeerMsgBuf_t* buffer, int32 seq, int32 PeerIdx)
{
    int32           idx = buffer->OldestIndex,
                    checked = 0,
                    count = buffer->MsgCount;
    uint32          NetMsgSize = 0;
    SBN_SenderId_t  sender;

    while(checked < count)
    {
        if(buffer->Msgs[idx].Hdr.SequenceCount == seq)
        {
            if(buffer->Retransmits[idx] < SBN_MAX_MSG_RETRANSMISSIONS)
            {
                NetMsgSize = buffer->Msgs[idx].Hdr.MsgSize;
                if(CFE_PSP_MemCpy(&SBN.MsgBuf,
			&buffer->Msgs[idx], NetMsgSize) != CFE_PSP_SUCCESS)
		    return SBN_ERROR;
                
                /* set up sender information */
                strncpy(&sender.AppName[0], 
                        (char *) &(SBN.MsgBuf.Hdr.MsgSender.AppName),
                        OS_MAX_API_NAME);
                sender.ProcessorId = SBN.MsgBuf.Hdr.MsgSender.ProcessorId;


                SBN_SendNetMsgNoBuf(SBN_APP_MSG, NetMsgSize, PeerIdx, &sender);
                buffer->Retransmits[idx]++;
            } /* end if */
            break;
        } /* end if */
    
        idx = ((idx + 1) % SBN_MSG_BUFFER_SIZE);

        checked++;
    } /* end while */

    return SBN_OK;
} /* end SBN_RetransmitSeq */
