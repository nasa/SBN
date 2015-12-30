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
int32 SBN_InitMsgBuf(SBN_PeerMsgBuf_t* buffer) {

    return SBN_ClearMsgBuf(buffer);
}

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
int32 SBN_AddMsgToMsgBufOverwrite(NetDataUnion* Msg, SBN_PeerMsgBuf_t* buffer) {
    int32 status;
    int32 addIdx = buffer->AddIndex;
 

    status = CFE_PSP_MemCpy(&buffer->Msgs[addIdx], Msg, sizeof(NetDataUnion));

    /* if oldest message was overwritten, update OldestIndex */
    if((addIdx == buffer->OldestIndex) || (buffer->OldestIndex == -1)) {
        buffer->OldestIndex = (buffer->OldestIndex + 1) % SBN_MSG_BUFFER_SIZE;
    }
    
    buffer->AddIndex = ((addIdx + 1) % SBN_MSG_BUFFER_SIZE);

    if(buffer->MsgCount != SBN_MSG_BUFFER_SIZE) {
        buffer->MsgCount++;
    }

    return status;
}

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
        int32 PeerIdx) {

    int32 status;
    int32 addIdx = buffer->AddIndex;
    uint32 NetMsgSize;

    /* if oldest message wil be overwritten, send it and update OldestIndex */
    if(addIdx == buffer->OldestIndex) {
        NetMsgSize = buffer->Msgs[buffer->OldestIndex].Hdr.MsgSize;
        status = CFE_PSP_MemCpy(&SBN.DataMsgBuf, 
                    &buffer->Msgs[buffer->OldestIndex], NetMsgSize);
        
        SBN_ProcessNetAppMsg(NetMsgSize);
        SBN.Peer[PeerIdx].RcvdCount++;

        buffer->OldestIndex = (buffer->OldestIndex + 1) % SBN_MSG_BUFFER_SIZE;
    }
    
    NetMsgSize = Msg->Hdr.MsgSize;
    status = CFE_PSP_MemCpy(&buffer->Msgs[addIdx], Msg, NetMsgSize);

    printf("SBN_AddMsgToMsgBufSend: Added Seq = %d to Idx = %d\n", Msg->Hdr.SequenceCount, addIdx);

    if(buffer->OldestIndex == -1) {
        buffer->OldestIndex = (buffer->OldestIndex + 1) % SBN_MSG_BUFFER_SIZE;
    }
    
    buffer->AddIndex = ((addIdx + 1) % SBN_MSG_BUFFER_SIZE);

    if(buffer->MsgCount != SBN_MSG_BUFFER_SIZE) {
        buffer->MsgCount++;
    }

    return status;
}
/**
 * Clears all messages out of the specified message buffer.  Sets each entry in
 * the buffer to NULL.  Can be used with both the sent message buffer and the
 * deferred message buffer.
 *
 * @param buffer  Buffer to clear
 * @return SBN_OK
 */
int32 SBN_ClearMsgBuf(SBN_PeerMsgBuf_t* buffer) {
    int32 status;

    status = CFE_PSP_MemSet(buffer, 0, sizeof(SBN_PeerMsgBuf_t));

    buffer->AddIndex = 0;
    buffer->MsgCount = 0;
    buffer->OldestIndex = -1;
    return status;
}

/**
 * Clears all messages acked by the specified ack from the specified message
 * buffer.  Should be used only with the sent message buffer.
 *
 * @param Ack     Ack message being handled
 * @param buffer  Buffer to clear
 * @return SBN_OK
 */
int32 SBN_ClearMsgBufBeforeSeq(SBN_NetProtoMsg_t* Ack, SBN_PeerMsgBuf_t* buffer) {
    int32 idx, seq, oldest, checked = 0, count;

    seq = Ack->Hdr.SequenceCount;
    count = buffer->MsgCount;
    oldest = buffer->OldestIndex;
    idx = oldest;

    //printf("SBN_ClearMsgBufBeforeSeq\n");
    while(checked < count) {
      //  printf("Checking index %d\n", idx);
        if(buffer->Msgs[idx].Hdr.SequenceCount <= seq) {
            CFE_PSP_MemSet(&buffer->Msgs[idx], 0, sizeof(NetDataUnion));
            buffer->MsgCount--;

            /* if oldest index was cleared, increment it */
            if(idx == oldest) {
                oldest = (oldest + 1) % SBN_MSG_BUFFER_SIZE;
            }
        }
    
        idx = ((idx + 1) % SBN_MSG_BUFFER_SIZE);

        checked++;
    }

    buffer->OldestIndex = oldest;
    printf("Removed Msgs From Send Buffer.  MsgCount = %d, OldestIdx = %d\n", 
            buffer->MsgCount, buffer->OldestIndex);
    return SBN_OK;
}

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
        int32 PeerIdx) {

    int32 idx, currentSeq, oldest, checked = 0, count, sent, status;
    uint32 NetMsgSize;
    count = buffer->MsgCount;
    oldest = buffer->OldestIndex;
    idx = oldest;
    currentSeq = seq + 1;
    sent = 0;

    while(checked < count) {
        printf("SBN_SendConsecutiveFromBuf: Checking idx %d, Seq = %d, ExpSeq = %d\n", idx, buffer->Msgs[idx].Hdr.SequenceCount, currentSeq);
        if(buffer->Msgs[idx].Hdr.SequenceCount == currentSeq) {
            printf("SBN_SendConsecutiveFromBuf: sending message with seq = %d\n", buffer->Msgs[idx].Hdr.SequenceCount);
            NetMsgSize = buffer->Msgs[idx].Hdr.MsgSize;
            status = CFE_PSP_MemCpy(&SBN.DataMsgBuf, 
                    &buffer->Msgs[idx], NetMsgSize);
        
            SBN_ProcessNetAppMsg(NetMsgSize);
            SBN.Peer[PeerIdx].RcvdCount++;
            sent++;

            CFE_PSP_MemSet(&buffer->Msgs[idx], 0, NetMsgSize);
            buffer->MsgCount--;

            /* if oldest index was cleared, increment it */
            if(idx == oldest) {
                oldest = (oldest + 1) % SBN_MSG_BUFFER_SIZE;
            }
            currentSeq++;
        }
        else {
            /* don't leave gaps in the buffer - consecutive messages should
             * follow each other */
            if(sent > 0) {
                break;
            }
        }

        idx = ((idx + 1) % SBN_MSG_BUFFER_SIZE);

        checked++;
    }

    buffer->OldestIndex = oldest;
    printf("Removed Msgs From Send Buffer.  MsgCount = %d, OldestIdx = %d\n", 
            buffer->MsgCount, buffer->OldestIndex);
    return sent;
}

/**
 * Retransmits the message with the specified sequence number up to a 
 * maximum number of retransmissions.
 *
 * @param buffer  Sent message buffer from which to retransmit
 * @param seq     Sequence number of message to retransmit
 * @param PeerIdx Index of peer
 * @return SBN_OK
 */
int32 SBN_RetransmitSeq(SBN_PeerMsgBuf_t* buffer, int32 seq, int32 PeerIdx) {
    int32 idx, checked = 0, count, status;
    uint32 NetMsgSize;
    CFE_SB_SenderId_t sender;

    count = buffer->MsgCount;
    idx = buffer->OldestIndex;
    
    printf("Retransmitting Message %d\n", seq);
    while(checked < count) {

 //       printf("Checking idx %d, seq = %d\n", idx, buffer->Msgs[idx].Hdr.SequenceCount);
        if(buffer->Msgs[idx].Hdr.SequenceCount == seq) {
   //         printf("#retransmis = %d\n", buffer->Retransmits[idx]);
            if(buffer->Retransmits[idx] < SBN_MAX_MSG_RETRANSMISSIONS) {
                NetMsgSize = buffer->Msgs[idx].Hdr.MsgSize;
                status = CFE_PSP_MemCpy(&SBN.DataMsgBuf, 
                            &buffer->Msgs[idx], NetMsgSize);
                
                /* set up sender information */
                strncpy((char *) &sender.AppName[0], 
                        &(SBN.DataMsgBuf.Hdr.MsgSender.AppName),
                        OS_MAX_API_NAME);
                sender.ProcessorId = SBN.DataMsgBuf.Hdr.MsgSender.ProcessorId;


                SBN_SendNetMsg(SBN_APP_MSG, NetMsgSize, PeerIdx, &sender, SBN_TRUE);
                buffer->Retransmits[idx]++;
                printf("Sent Retransmitted Message!\n");
            }
            break;
        }
    
        idx = ((idx + 1) % SBN_MSG_BUFFER_SIZE);

        checked++;
    }

    return SBN_OK;

}
