/**
 * @file
 * 
 * This file contains all functions relevant to reading/writing the serial 
 * device. 
 * 
 * @author Jaclyn Beck, Jonathan Urriste
 * @date 2015/06/24 15:30:00
 */
#include "serial_io.h"
#include "serial_queue.h"
#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "serial_sbn_if_struct.h"

#ifdef SBN_SERIAL_USE_TERMIOS
#include <errno.h>
#include <termios.h> 
#endif

uint32 HostQueueId = 0; 

/**
 * Opens the serial port as a file descriptor and then sets up some settings 
 * like baud rate. 
 * 
 * @param DevName   The name of the device to open (e.g. "/dev/ttyS0")
 * @param BaudRate  The desired baud rate of the serial port
 *
 * @return Fd        The opened file descriptor
 * @return SBN_ERROR on error
 */
int32 Serial_IoOpenPort(char *DevName, uint32 BaudRate) {
    int32 Fd; 

    /* open serial device and set options */
    Fd = OS_open(DevName, OS_READ_WRITE, 0); 
	if (Fd < 0) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error opening device %s. Returned %d\n",
                          DevName, Fd);
		return Fd;
	}
	if (Serial_IoSetAttrs(Fd, BaudRate) == SBN_ERROR) {
        OS_close(Fd); 
		return SBN_ERROR;
	}

    return Fd; 
}


#ifdef SBN_SERIAL_USE_TERMIOS
/**
 * Specifies the TTY settings for the serial interface. This function requires
 * that the OS supports termios. 
 *
 * @param Fd        The file descriptor for the serial TTY device
 * @param Baud      Desired baud, in terms of constants in termios.h
 * @param Parity    If no parity is desired, set to 0
 *
 * @return SBN_OK if successful
 * @return SBN_ERROR if unsuccessful
 */
int32 Serial_IoSetAttrs(int32 Fd, uint32 BaudRate) {
	struct termios tty;
    int32 termiosBaud; 
    OS_FDTableEntry tblentry;

    OS_FDGetInfo (Fd, &tblentry);

	if (tcgetattr(tblentry.OSfd, &tty) != 0) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error accessing tty settings, errno: 0x%x\n", 
                          errno);
		return SBN_ERROR;
	}

    switch(BaudRate) {
        case 38400:
            termiosBaud = B38400;
            break;

        case 57600:
            termiosBaud = B57600;
            break;

        case 115200:
            termiosBaud = B115200;
            break;
        
        case 230400:
            termiosBaud = B230400;
            break;

        default:
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                    "Serial: Unknown baud rate %d\n", BaudRate); 
            return SBN_ERROR; 
    }

	cfsetospeed(&tty, termiosBaud); /* Set output baud rate */
	cfsetispeed(&tty, termiosBaud); /* Set input baud rate */
	
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8 bit words */
	tty.c_iflag &= ~IGNBRK; /* disable break processing */
	tty.c_lflag = 0;        /* disable signaling characters, echo, canonical processing */
	tty.c_oflag = 0;        /* disable remapping and delays */
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* disable xon/xoff control */
	tty.c_cflag |= (CLOCAL | CREAD);        /* disable modem controls, enable reading */
	tty.c_cflag &= ~(PARENB | PARODD);      /* disable parity */
	tty.c_cflag &= ~CSTOPB;                 /* send 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;                /* no flow control */

    tty.c_cc[VMIN]  = 0;  /* Don't block until a character has been received */
	tty.c_cc[VTIME] = 10; /* read() will timeout after 10 tenths of a second */

    tcflush(tblentry.OSfd, TCIFLUSH);
	if (tcsetattr(tblentry.OSfd, TCSANOW, &tty) != 0) {
		CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                    "Serial: Error setting tty settings, errno: 0x%x\n", 
                    errno);
		return SBN_ERROR;
	}

	return SBN_OK;
}

#else
/**
 * Non-Linux OS / non termios implementation of setting serial settings. 
 */
int32 Serial_IoSetAttrs(int32 Fd, uint32 BaudRate) {
    CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Serial_IoSetAttrs not implemented for this OS\n"); 
    return SBN_ERROR;
}
#endif


/**
 * Tries to read a message off the serial wire. If a message is read, it
 * determines which queue to put it in and adds the message to that queue. Each
 * message starts with a 4-byte sync word followed by the message length
 * (which includes the sync word, message length bytes, and the message payload), 
 * and the payload. 
 *
 * @param host       The host data struct containing queues and the file descriptor
 *
 * @return dataRead (number of bytes read off the wire)
 * @return SBN_IF_EMPTY if no message to read
 * @return SBN_ERROR if unsuccessful
 */
int32 Serial_IoReadMsg(Serial_SBNHostData_t *host) {
	int32 dataRead = 0;
	int32 status;
	uint32 messageSize, MsgType;
    uint8 DataMsgBuf[sizeof(NetDataUnion)]; 
	
	/* Synchronize message with magic number a.k.a. sync code */
	status = Serial_IoReadSyncBytes(host->Fd);

	if (status <= 0) {
        if (status == 0 || errno == EAGAIN) {
            return SBN_IF_EMPTY; 
        }
		CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error obtaining message sync code. Returned %d\n", status);
		return SBN_ERROR;
	}
    
	/* Update the number of bytes filling in the buffer */
	messageSize = Serial_IoReadMessageSize(host->Fd);
    dataRead = 2*sizeof(uint32); /* Sync word + message size */

	if (messageSize < 0) {
		CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error obtaining message size. Returned %d\n", messageSize);
		return SBN_ERROR;
	}

    ((NetDataUnion*)DataMsgBuf)->Hdr.MsgSize = messageSize;

	/* Size of data message now obtained, read the rest of the payload */
	if (messageSize == 0) {
		return SBN_IF_EMPTY;
	}

    while (dataRead < messageSize) {
	    status = OS_read(host->Fd, &DataMsgBuf[dataRead], messageSize-dataRead);
	    if (status < 0) {
		    CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                "Serial: Error obtaining message payload. Returned %d\n", status);
		    return SBN_ERROR;
	    }
        dataRead += status;
    }

	if (dataRead != messageSize) {
		CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Data receive mismatch. Expected size %d, received size %d\n", 
            messageSize, dataRead);
	}

    MsgType = ((NetDataUnion*)DataMsgBuf)->Hdr.Type;

    if (SBN_LIB_MsgTypeIsProto(MsgType)) {
        status = Serial_QueueAddNode(host->ProtoQ, host->ProtoSemId, DataMsgBuf); 
    }
    else {
        status = Serial_QueueAddNode(host->DataQ, host->DataSemId, DataMsgBuf); 
    }

    if (status == SBN_OK) {
        return dataRead;
    }

	return status;
}


/**
 * Parses a data stream for individual messages by identifying the synchronizing code,
 * reads one byte at a time to avoid endianness issues when reading multiple bytes at a time
 *
 * @param Fd    File descriptor for reading the serial device
 * 
 * @return 1 if successful, 
 * @return negative number if unsuccessful
 */
int32 Serial_IoReadSyncBytes(int32 Fd) {
	uint8 buffer; 
	int32 msgSize;
	uint8 syncBytesMatched = 0;
	
	while (syncBytesMatched < SBN_SYNC_LENGTH) {
		buffer = 0;
		msgSize = OS_read(Fd, &buffer, 1);

		if (msgSize < 0) { /* error on read() */
			return msgSize;
		}
		if (msgSize == 0) { /* No message available for parsing, possibly due to empty TTY buffer */
            if (syncBytesMatched == 0) {
			    return 0;
            }
            /* Don't return if we've already found a sync byte, instead keep 
               waiting for more data */
		}
		else {
		    /* capture the sync code, testing each byte for a match */
		    /* on mismatch, start searching again */
		    if (buffer == sbn_sync[syncBytesMatched]) {
			    syncBytesMatched++;
		    }
            else if (buffer == sbn_sync[0]) { /* Account for 6A 6A 89 98 B9 */
                syncBytesMatched = 1; 
            }
		    else {
			    syncBytesMatched = 0;
		    }
        }
	}

	return 1;
}


/**
 * Obtains a serial message size given previous sync code alignment. Assumes that
 * once the sync sequence has been found, there is/will be enough data on the 
 * wire to get the full message size without blocking for too long. 
 *
 * @param Fd File descriptor for reading the serial device
 *
 * @return The message payload size
 */
uint32 Serial_IoReadMessageSize(int32 Fd) {
    int32  status;
    uint8  msgBuf[4], bytesRead = 0;
	uint32 messageSize = 0;

    while (bytesRead < 4) {
        status = OS_read(Fd, &msgBuf[bytesRead], 4-bytesRead);
        if (status < 0) { /* missing data, or errors on read() */
		    return 0;
	    }
        bytesRead += status;
    }

    CFE_PSP_MemCpy(&messageSize, msgBuf, 4);
	return messageSize;
}


/**
 * Prepares and sends message for sending  over the serial channel. Message prepended with sync code and payload byte size
 * @param Fd            File descriptor for writing to the serial device
 * @param DataMsgBuf    Buffer containing  payload
 * @param MsgSize       Size of the message in bytes (includes sync and message size bytes)
 *
 * @return number of bytes sent if successful
 * @return SBN_ERROR if unsuccessful 
 */
int32 Serial_IoWriteMsg(int32 Fd, void *DataMsgBuf, uint32 MsgSize) {
	int32 status;
	int16 bytesSent = 0;

	status = OS_write (Fd, DataMsgBuf, MsgSize);
    bytesSent += status; 

	if (status < 0) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Error writing Payload. Returned %d\n", status); 
		return status;
	}

	return bytesSent;
}


/**
 * Thread that continuously reads the serial device and puts the read messages
 * in either the protocol or data queues. 
 */
void Serial_IoReadTaskMain() {
    int32 dataRead = 0;
    Serial_SBNHostData_t *host;
    uint32 size; 

    CFE_ES_RegisterChildTask();

    /* Pull the host off the queue so its proto/data queues can be used */
    OS_QueueGet (HostQueueId, &host, sizeof(uint32), &size, OS_PEND); 

    if (size == 0 || host == NULL || host->Fd < 0) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
            "Serial: Cannot start read task. Host is null.\n"); 
        CFE_ES_ExitChildTask();
        return;
    }

    /* Keep reading forever unless there's an error */
    while(dataRead == SBN_IF_EMPTY || dataRead >= 0) { 
        dataRead = Serial_IoReadMsg(host); 
    }

    CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_INFORMATION,
            "Serial: Serial Read Task exiting for host number %d\n", host->PairNum);
    CFE_ES_ExitChildTask();
}


/**
 * You cannot pass arguments to threads in CFE so this function puts the host in
 * an OS queue accessible across threads, starts a task, and the task pulls the 
 * host off the queue when it starts up. This function returns SBN_VALID or 
 * SBN_NOT_VALID because called during host validation. An error here means the
 * host is not valid and shouldn't be used.
 *
 * @param host  The host to start reading from
 *
 * @return SBN_VALID on success
 * @return SBN_NOT_VALID on error
 */
int32 Serial_IoStartReadTask(Serial_SBNHostData_t *host) {
    char name[20]; 
    int32 Status; 

    /* If the queue doesn't already exist, create it */
    if (HostQueueId == 0) {
        Status = OS_QueueCreate(&HostQueueId, "SerialHostQueue", SBN_SERIAL_QUEUE_DEPTH,
                                sizeof(uint32), 0); 

        if (Status != OS_SUCCESS) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_INFORMATION,
                "Serial: Error creating host queue. Returned %d\n", Status); 
            return SBN_NOT_VALID; 
        }
    }

    /* Add this host to the queue */
    Status = OS_QueuePut(HostQueueId, &host, sizeof(uint32), 0); 
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_INFORMATION,
            "Serial: Error adding host to queue. Returned %d\n", Status); 
        return SBN_NOT_VALID; 
    }

    /* Start the child task that will read this host's fd */
    sprintf(name, "SerialReadTask%d", host->PairNum); 
    Status = CFE_ES_CreateChildTask(&host->TaskHandle,
                                    name,
                                    Serial_IoReadTaskMain,
                                    NULL,
                                    SBN_SERIAL_CHILD_STACK_SIZE,
                                    SBN_SERIAL_CHILD_TASK_PRIORITY,
                                    0);
    if (Status != CFE_SUCCESS) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_INFORMATION,
            "Serial: Error creating read task for host %d. Returned %d\n", 
            host->PairNum, Status); 
        return SBN_NOT_VALID; 
    }

    return SBN_VALID; 
}

