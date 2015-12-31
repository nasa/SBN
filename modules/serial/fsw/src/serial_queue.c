/**
 * @file
 * 
 * This file contains all functions relevant adding or removing data from the
 * OS queues. 
 * 
 * @author Jaclyn Beck
 * @date 2015/06/24 15:30:00
 */
#include "cfe.h"
#include "serial_queue.h"
#include "sbn_constants.h"
#include "sbn_interfaces.h"

/**
 * Checks the given queue to see if there is data available. If so, it copies 
 * the data into the message buffer. 
 *
 * @param queue      Queue ID (data or protocol) to check
 * @param semId      Semaphore ID to lock/unlock
 * @param MsgBuf     Pointer to the buffer to copy the message into
 *
 * @return SBN_IF_EMPTY if no data available
 * @return msgSize (number of bytes read from the queue)
 * @return SBN_ERROR on error
 */
int32 Serial_QueueGetMsg(uint32 queue, uint32 semId, uint8 *MsgBuf) {
    uint32 msgSize = 0; 
    NetDataUnion *data; 
    uint32 size; 
    int32 Status; 
 
    if (queue == 0) { 
        return SBN_IF_EMPTY; 
    }

    /* Take the semaphore */
    Status = OS_BinSemTake(semId); 
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error taking semaphore %d. Returned %d\n", semId, Status); 
        return SBN_ERROR; 
    }

    /* Try and get a message from the queue (non-blocking) */
    Status = OS_QueueGet (queue, &data, sizeof(uint32), &size, OS_CHECK); 
    
    if (Status == OS_SUCCESS && data != NULL) {
        msgSize = data->Hdr.MsgSize;
        CFE_PSP_MemCpy(MsgBuf, data, msgSize); 
        free(data);
    }
    else {
        msgSize = SBN_IF_EMPTY;
    }

    /* Give up the semaphore */
    Status = OS_BinSemGive(semId); 
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error giving semaphore %d. Returned %d\n", semId, Status);
        return SBN_ERROR; 
    }

    return msgSize; 
}


/**
 * Adds a message to the OS queue. The message is allocated and then copied
 * from MsgBuf to the allocated character buffer. 
 *
 * @param queue      Queue ID (data or protocol) to add to
 * @param semId      Semaphore ID to lock/unlock
 * @param MsgBuf     Pointer to the buffer to copy the message from
 * 
 * @return SBN_OK on success
 * @return SBN_ERROR on error
 */
int32 Serial_QueueAddNode(uint32 queue, uint32 semId, uint8 *MsgBuf) {
    int32 Status; 
    uint32 msgSize = ((NetDataUnion*)MsgBuf)->Hdr.MsgSize;
    uint8 *message = malloc(msgSize); 

    if (message == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: QueueAddNode: Error allocating message\n"); 
        return SBN_ERROR; 
    }

    CFE_PSP_MemCpy(message, MsgBuf, msgSize); 

    /* Take the semaphore */
    Status = OS_BinSemTake(semId); 
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error taking semaphore %d. Returned %d\n", semId, Status); 
        return SBN_ERROR; 
    }

    /* Try adding the message to the queue */
    Status = OS_QueuePut (queue, &message, sizeof(uint32), 0); 
    if (Status == OS_QUEUE_FULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_INFORMATION,
            "Serial: Queue %d is full. Old messages will be lost.\n", queue); 

        /* Remove the oldest message to make room for the new message and try 
           adding the new message again */
        Serial_QueueRemoveNode(queue); 
        Status = OS_QueuePut (queue, &message, sizeof(uint32), 0); 
    }

    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error adding message to queue %d. Returned %d\n", queue, Status); 
    }

    /* Give up the semaphore */
    Status = OS_BinSemGive(semId); 
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error giving semaphore %d. Returned %d\n", semId, Status); 
        return SBN_ERROR; 
    }

    return SBN_OK; 
}


/**
 * Removes a message from the queue and discards the data. The removed message is
 * then freed to avoid memory leaks. 
 *
 * @param queue     Queue ID (data or protocol) to remove from
 *
 * @return Status   The return value of OS_QueueGet
 */
int32 Serial_QueueRemoveNode(uint32 queue) {
    uint8 *data = NULL; 
    uint32 size; 
    int32 Status;

    Status = OS_QueueGet (queue, &data, sizeof(uint32), &size, OS_CHECK); 
    if (data != NULL) {
        free(data); 
    }
    return Status; 
}

