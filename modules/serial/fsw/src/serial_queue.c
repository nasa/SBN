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
 * @return MsgSize (number of bytes read from the queue)
 * @return SBN_ERROR on error
 */
int Serial_QueueGetMsg(uint32 queue, uint32 semId, NetDataUnion *MsgBuf) {
    NetDataUnion *data = NULL;
    int32 status = 0;
    uint32 size = 0;
 
    if (queue == 0) { 
        return SBN_IF_EMPTY; 
    }

    /* Take the semaphore */
    status = OS_BinSemTake(semId); 
    if (status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error taking semaphore %d. Returned %d\n", semId, status); 
        return SBN_ERROR; 
    }

    /* Try and get a message from the queue (non-blocking) */
    status = OS_QueueGet(queue, &data, sizeof(uint32), &size, OS_CHECK); 
    
    if (status == OS_SUCCESS && data != NULL) {
        size = data->Hdr.MsgSize;
        CFE_PSP_MemCpy(MsgBuf, data, size);
        free(data);
    }

    /* Give up the semaphore */
    status = OS_BinSemGive(semId); 
    if (status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error giving semaphore %d. Returned %d\n", semId, status);
        return SBN_ERROR; 
    }

    return SBN_OK; 
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
int Serial_QueueAddNode(uint32 queue, uint32 semId,  NetDataUnion *MsgBuf) {
    int32 status = 0;
    uint32 MsgSize = MsgBuf->Hdr.MsgSize;
    uint8 *message = malloc(MsgSize); 

    if (message == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: QueueAddNode: Error allocating message\n"); 
        return SBN_ERROR; 
    }

    CFE_PSP_MemCpy(message, MsgBuf, MsgSize); 

    /* Take the semaphore */
    status = OS_BinSemTake(semId); 
    if (status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error taking semaphore %d. Returned %d\n", semId, status); 
        return SBN_ERROR; 
    }

    /* Try adding the message to the queue */
    status = OS_QueuePut (queue, &message, sizeof(uint32), 0); 
    if (status == OS_QUEUE_FULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_INFORMATION,
            "Serial: Queue %d is full. Old messages will be lost.\n", queue); 

        /* Remove the oldest message to make room for the new message and try 
           adding the new message again */
        Serial_QueueRemoveNode(queue); 
        status = OS_QueuePut (queue, &message, sizeof(uint32), 0); 
    }

    if (status != OS_SUCCESS) {
        /* TODO: should we free message? */
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error adding message to queue %d. Returned %d\n", queue, status); 
    }

    /* Give up the semaphore */
    status = OS_BinSemGive(semId); 
    if (status != OS_SUCCESS) {
        /* TODO: should we free message? */
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error giving semaphore %d. Returned %d\n", semId, status); 
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
 * @return SBN_OK on success or SBN_ERROR if there's an issue.
 */
int32 Serial_QueueRemoveNode(uint32 queue) {
    uint8 *data = NULL; 
    uint32 size = 0;

    if (OS_QueueGet (queue, &data, sizeof(uint32), &size, OS_CHECK) != OS_SUCCESS)
        return SBN_ERROR;
    if (data != NULL) {
        free(data); 
    }
    return SBN_OK;
}
