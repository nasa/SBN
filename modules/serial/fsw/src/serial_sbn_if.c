/**
 * @file
 *
 * This file contains all functions that SBN calls, plus some helper functions.
 *
 * @author Jaclyn Beck, Jonathan Urriste
 * @date 2015/06/24 15:30:00
 */
#include "serial_sbn_if_struct.h"
#include "serial_sbn_if.h"
#include "serial_platform_cfg.h"
#include "serial_queue.h"
#include <string.h>

/**
 * Receives a message from a peer over the appropriate interface.
 * Read pattern: search data for sync code, get message size, get message payload
 *
 * @param Host       Structure of interface data for the host
 * @param DataMsgBuf Pointer to the SBN's protocol message buffer
 *
 * @return Bytes received on success,
 * @return SBN_IF_EMPTY if no data,
 * @return SBN_ERROR on error
 */
int32 Serial_SbnReceiveMsg(SBN_InterfaceData *Host, NetDataUnion *DataMsgBuf) {
    Serial_SBNHostData_t *host;
    int32 status;

    if (Host == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in SerialRcvMsg: Host is NULL.\n");
        return SBN_ERROR;
    }
    if (DataMsgBuf == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in SerialRcvMsg: DataMsgBuf is NULL.\n");
        return SBN_ERROR;
    }

	host = (Serial_SBNHostData_t *)(Host->HostData);
	status = Serial_QueueGetMsg(host->DataQ, host->DataSemId, (uint8 *)DataMsgBuf);

    return status;
}


/**
 * Checks for a single protocol message.
 *
 * @param Peer         Structure of interface data for a peer
 * @param ProtoMsgBuf  Pointer to the SBN's protocol message buffer
 *
 * @return SBN_TRUE for message available
 * @return SBN_NO_MSG for no messages or an error due to link being down or data copy error
 * @return SBN_ERROR on error
 */
int32 Serial_SbnCheckForNetProtoMsg(SBN_InterfaceData *Peer, SBN_NetProtoMsg_t *ProtoMsgBuf) {
	Serial_SBNPeerData_t *peer;
	int32 status;

    if (ProtoMsgBuf == NULL) {
        CFE_EVS_SendEvent(SBN_NET_RCV_PROTO_ERR_EID,CFE_EVS_ERROR,
                          "Serial: Error in CheckForSerialNetProtoMsg: ProtoMsgBuf is NULL.\n");
        return SBN_ERROR;
    }
    if (Peer == NULL) {
        CFE_EVS_SendEvent(SBN_NET_RCV_PROTO_ERR_EID,CFE_EVS_ERROR,
                          "Serial: Error in CheckForSerialNetProtoMsg: Peer is NULL.\n");
        ProtoMsgBuf->Hdr.Type = SBN_NO_MSG;
        return SBN_ERROR;
    }

	peer = (Serial_SBNPeerData_t *)(Peer->PeerData);
	status = Serial_QueueGetMsg(peer->ProtoQ, peer->ProtoSemId, (uint8 *)ProtoMsgBuf);

    if (status > 0) {
        return SBN_TRUE; /* Message available and no errors */
    }

    if ( status == SBN_ERROR ) {
        CFE_EVS_SendEvent(SBN_NET_RCV_PROTO_ERR_EID,CFE_EVS_ERROR,
                "Serial: Error in CheckForSerialNetProtoMsg. QueueGetMsg status=%d",
                status);
        ProtoMsgBuf->Hdr.Type = SBN_NO_MSG;
        return SBN_ERROR;
    }

    /* status = 0, so no messages and no errors */
    ProtoMsgBuf->Hdr.Type = SBN_NO_MSG;
    return SBN_NO_MSG;
} /* end SBN_CheckForNetProtoMsg */


/**
 * Parses the peer data file into SBN_FileEntry_t structures.
 *
 * @param FileEntry  Interface description line as read from file
 * @param LineNum    The line number in the peer file
 * @param EntryAddr  Address in which to return the filled entry struct
 *
 * @return SBN_OK if entry is parsed correctly
 * @return SBN_ERROR on error
 */
int32 Serial_SbnParseInterfaceFileEntry(char *FileEntry, uint32 LineNum,
        void** EntryAddr) {

    int     ScanfStatus;
    uint32  BaudRate;
    uint8   PairNum;
    char    DevNameHost[SBN_SERIAL_MAX_CHAR_NAME];
    Serial_SBNEntry_t *entry;

    /*
    ** Using sscanf to parse the string.
    ** Currently no error handling
    */
    ScanfStatus = sscanf(FileEntry, "%u %s %u", &PairNum, DevNameHost, &BaudRate);
    DevNameHost[SBN_SERIAL_MAX_CHAR_NAME-1] = 0;

    /* Check to see if the correct number of items were parsed */
    if (ScanfStatus != SBN_SERIAL_ITEMS_PER_FILE_LINE) {
        CFE_EVS_SendEvent(SBN_INV_LINE_EID,CFE_EVS_ERROR,
                "Invalid SBN peer file line, exp %d items,found %d",
                SBN_SERIAL_ITEMS_PER_FILE_LINE, ScanfStatus);
        return SBN_ERROR;
    }

    entry = malloc(sizeof(Serial_SBNEntry_t));
    if (entry == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                "Serial cannot allocate memory for host/peer file entry.\n");
        return SBN_ERROR;
    }

    *EntryAddr = entry;
    entry->PairNum  = PairNum;
    entry->BaudRate = BaudRate;
    strcpy(entry->DevNameHost, DevNameHost);

    return SBN_OK;
}


/**
 * Initializes a Serial host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 *
 * @return SBN_HOST if this entry is for a host (this CPU)
 * @return SBN_PEER if this entry is for a peer (a different CPU)
 * @return SBN_ERROR on error
 */
int32 Serial_SbnInitPeerInterface(SBN_InterfaceData *Data) {
    Serial_SBNEntry_t *entry;
    int32 Status;
    char name[20];

    if (Data == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Cannot initialize interface! Interface data is null.\n");
        return SBN_ERROR;
    }

	entry = (Serial_SBNEntry_t *)(Data->EntryData);

	/* CPU names match - this is host data. */
	if(strncmp(Data->Name, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH) == 0) {
		/* create, fill, and store Serial-specific host data structure */
		Serial_SBNHostData_t *host = malloc(sizeof(Serial_SBNHostData_t));
		if (host == NULL) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                            "Serial: Cannot allocate host interface data.\n");
			return SBN_ERROR;
		}
		Data->HostData = (int32)host;

		/* open serial device and set options */
		host->Fd = Serial_IoOpenPort(entry->DevNameHost, entry->BaudRate);
		if (host->Fd < 0) {
			return SBN_ERROR;
		}

        strcpy(host->DevName, entry->DevNameHost);
        host->PairNum  = entry->PairNum;
        host->BaudRate = entry->BaudRate;

        /* Create protocol queue semaphor */
        sprintf(name, "ProtoSem%d", host->PairNum);
        Status = OS_BinSemCreate(&host->ProtoSemId,
                             name,
                             OS_SEM_FULL,
                             0);

        if (Status != OS_SUCCESS) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                            "Serial: Error creating protocol semaphor for host %d. Returned %d\n",
                            host->PairNum, Status);
        }

        /* Create data queue semaphor */
        sprintf(name, "DataSem%d", host->PairNum);
        Status = OS_BinSemCreate(&host->DataSemId,
                             name,
                             OS_SEM_FULL,
                             0);

        if (Status != OS_SUCCESS) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                            "Serial: Error creating data semaphor for host %d. Returned %d\n",
                            host->PairNum, Status);
        }

        /* Create protocol queue */
        sprintf(name, "SerialProtoQueue%d", host->PairNum);
        Status = OS_QueueCreate(&host->ProtoQ, name, SBN_SERIAL_QUEUE_DEPTH,
                                sizeof(uint32), 0);

        if (Status != OS_SUCCESS) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                            "Serial: Error creating protocol queue for host %d. Returned %d\n",
                            host->PairNum, Status);
        }

        /* Create data queue */
        sprintf(name, "SerialDataQueue%d", host->PairNum);
        OS_QueueCreate(&host->DataQ, name, SBN_SERIAL_QUEUE_DEPTH,
                       sizeof(uint32), 0);

        if (Status != OS_SUCCESS) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                            "Serial: Error creating data queue for host %d. Returned %d\n",
                            host->PairNum, Status);
        }

		return SBN_HOST;
	}
	/* CPU names do not match - this is peer data. */
	else {
		/* create, fill, and store peer data structure */
		Serial_SBNPeerData_t *peer = malloc(sizeof(Serial_SBNPeerData_t));
        if (peer == NULL) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Cannot allocate peer interface data.\n");
			return SBN_ERROR;
		}
		Data->PeerData = (int32)peer;

        peer->PairNum  = entry->PairNum;
        peer->BaudRate = entry->BaudRate;
        peer->ProtoQ = 0;

		return SBN_PEER;
	}
}


/**
 * Sends a message to a peer over a Serial interface.
 *
 * @param MsgType      Type of Message
 * @param MsgSize      Size of Message
 * @param HostList     The array of SBN_InterfaceData structs that describes the host
 * @param SenderPtr    Sender information
 * @param IfData       The SBN_InterfaceData struct describing this peer
 * @param ProtoMsgBuf  Protocol message
 * @param DataMsgBuf   Data message
 *
 * @return number of bytes written on success
 * @return SBN_ERROR on error
 */
int32 Serial_SbnSendNetMsg(uint32 MsgType, uint32 MsgSize, SBN_InterfaceData *HostList[], int32 NumHosts, CFE_SB_SenderId_t *SenderPtr, SBN_InterfaceData *IfData, SBN_NetProtoMsg_t *ProtoMsgBuf, NetDataUnion *DataMsgBuf) {
	int32 status = 0;
	int32 found = 0;
    Serial_SBNEntry_t *peer;
    Serial_SBNEntry_t *host_tmp;
	Serial_SBNHostData_t *host;
	uint32 HostIdx;

    /* Check pointer arguments used for all cases for null */
    if (HostList == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in SendSerialNetMsg: HostList is NULL.\n");
        return SBN_ERROR;
    }
    if (IfData == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in SendSerialNetMsg: IfData is NULL.\n");
        return SBN_ERROR;
    }

	/* Find the host that goes with this peer. */
	peer = (Serial_SBNEntry_t *)(IfData->EntryData);
	for(HostIdx = 0; HostIdx < NumHosts; HostIdx++) {
		if(HostList[HostIdx]->ProtocolId == SBN_SERIAL) {
            host_tmp = (Serial_SBNEntry_t *)(HostList[HostIdx]->EntryData);
            if ( Serial_IsHostPeerMatch(host_tmp, peer) ) {
                found = 1;
			    host = (Serial_SBNHostData_t *)(HostList[HostIdx]->HostData);
                break;
            }
		}
	}
	if(found != 1) {
		CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: No Serial Host Found!\n");
		return SBN_ERROR;
	}

	switch(MsgType) {

        /* Data messages */
		case SBN_APP_MSG:
            if (SenderPtr == NULL) {
                CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                                  "Serial: Error in SendSerialNetMsg: SenderPtr is NULL.\n");
                return SBN_ERROR;
            }

            /* If my peer sent this message, don't send it back to them, avoids loops */
			if (CFE_PSP_GetProcessorId() != SenderPtr->ProcessorId)
				break;

			/* Then no break, so fill in the sender application information */
            if (DataMsgBuf == NULL) {
                CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                                  "Serial: Error in SendSerialNetMsg: DataMsgBuf is NULL.\n");
                return SBN_ERROR;
            }

			strncpy((char *)&(DataMsgBuf->Hdr.MsgSender.AppName), &(SenderPtr->AppName[0]), OS_MAX_API_NAME);
			DataMsgBuf->Hdr.MsgSender.ProcessorId = SenderPtr->ProcessorId;
            /* Fall through to the next case */

		case SBN_SUBSCRIBE_MSG:
		case SBN_UN_SUBSCRIBE_MSG:
            if (DataMsgBuf == NULL) {
                CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                                  "Serial: Error in SendSerialNetMsg: DataMsgBuf is NULL.\n");
                return SBN_ERROR;
            }

			/* Initialize the SBN hdr of the outgoing network message */
            CFE_PSP_MemCpy(DataMsgBuf->Hdr.Sync, sbn_sync, 4);
            DataMsgBuf->Hdr.MsgSize = MsgSize;
			strncpy((char *)&DataMsgBuf->Hdr.SrcCpuName,CFE_CPU_NAME,SBN_MAX_PEERNAME_LENGTH);

			DataMsgBuf->Hdr.Type = MsgType;
			status = Serial_IoWriteMsg(host->Fd, (void *)DataMsgBuf, MsgSize);

			break;

        /* Protocol messages */
		case SBN_ANNOUNCE_MSG:
		case SBN_ANNOUNCE_ACK_MSG:
		case SBN_HEARTBEAT_MSG:
		case SBN_HEARTBEAT_ACK_MSG:
            if (ProtoMsgBuf == NULL) {
                CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                                  "Serial: Error in SendSerialNetMsg: ProtoMsgBuf is NULL.\n");
                  return SBN_ERROR;
            }

            CFE_PSP_MemCpy(ProtoMsgBuf->Hdr.Sync, sbn_sync, 4);
            ProtoMsgBuf->Hdr.MsgSize = MsgSize;
			ProtoMsgBuf->Hdr.Type = MsgType;
			strncpy(ProtoMsgBuf->Hdr.SrcCpuName, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH);

			status = Serial_IoWriteMsg(host->Fd, (void *)ProtoMsgBuf, MsgSize);
			break;

		default:
			CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Unexpected message type!\n");
			/* send event to indicate unexpected msgtype */
			status = SBN_ERROR;
			break;
	} /* end switch */

	return (status);
} /* end SBN_SendNetMsg */


/**
 * Iterates through the list of all host interfaces to see if there is a
 * match for the specified peer interface. For the serial interface, there
 * should be exactly one host per peer.
 *
 * @param SBN_InterfaceData *Peer    Peer to verify
 * @param SBN_InterfaceData *Hosts[] List of hosts to check against the peer
 * @param int32 NumHosts             Number of hosts in the SBN
 *
 * @return SBN_VALID     if the required match exists, else
 * @return SBN_NOT_VALID if not
 */
int32 Serial_SbnVerifyPeerInterface(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts) {
	int32 HostIdx;
    Serial_SBNEntry_t *HostEntry;
    Serial_SBNEntry_t *PeerEntry;

    if (Peer == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in Serial_VerifyPeerInterface: Peer is NULL.\n");
        return SBN_NOT_VALID;
    }
    if (HostList == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in Serial_VerifyPeerInterface: Hosts is NULL.\n");
        return SBN_NOT_VALID;
    }

    PeerEntry = (Serial_SBNEntry_t *)Peer->EntryData;

    /* Find the host that goes with this peer. */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++) {
        if(HostList[HostIdx]->ProtocolId == SBN_SERIAL) {
            HostEntry = (Serial_SBNEntry_t *)HostList[HostIdx]->EntryData;

            if ( Serial_IsHostPeerMatch(HostEntry, PeerEntry) ) {
                return SBN_VALID;
            }
        }
    }

    return SBN_NOT_VALID;
}


/**
 * Iterates through the list of all peer interfaces to see if there is a
 * match for the specified host interface. For the serial interface, there should
 * be exactly one peer per host. Once a valid host/peer match is found, the
 * task to read from the serial port is started for that host.
 *
 * @param SBN_InterfaceData *Host    Host to verify
 * @param SBN_PeerData_t *Peers      List of peers to check against the host
 * @param int32 NumPeers             Number of peers in the SBN
 *
 * @return SBN_VALID     if the required match exists, else
 * @return SBN_NOT_VALID if not
 */
int32 Serial_SbnVerifyHostInterface(SBN_InterfaceData *Host, SBN_PeerData_t *PeerList, int32 NumPeers) {
	int PeerIdx;
    Serial_SBNEntry_t *PeerEntry;
    Serial_SBNEntry_t *HostEntry;
    Serial_SBNHostData_t *HostData;
    Serial_SBNPeerData_t *PeerData;

    if (Host == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in Serial_VerifyHostInterface: Host is NULL.\n");
        return SBN_NOT_VALID;
    }
    if (PeerList == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in Serial_VerifyHostInterface: PeerList is NULL.\n");
        return SBN_NOT_VALID;
    }

    HostEntry = (Serial_SBNEntry_t *)Host->EntryData;

    /* Find the peer that goes with this host. */
    for(PeerIdx = 0; PeerIdx < NumPeers; PeerIdx++) {
        if(PeerList[PeerIdx].ProtocolId == SBN_SERIAL) {
            PeerEntry = (Serial_SBNEntry_t *)(PeerList[PeerIdx].IfData)->EntryData;

            if ( Serial_IsHostPeerMatch(HostEntry, PeerEntry) ) {
                HostData = (Serial_SBNHostData_t *) Host->HostData;
                PeerData = (Serial_SBNPeerData_t *) (PeerList[PeerIdx].IfData)->PeerData;

                /* Connect the protocol queue to the peer's data so the peer can access it */
                PeerData->ProtoQ = HostData->ProtoQ;
                PeerData->ProtoSemId = HostData->ProtoSemId;

                /* Start serial read task. */
                return Serial_IoStartReadTask(HostData);
            }
        }
    }

    return SBN_NOT_VALID;
}


/**
 * Reports the status of the module. The status packet is passed in
 * initialized (with message ID and size), the module fills it, and upon
 * return the SBN application sends the message over the software bus.
 * For the serial module, the status packet contains the peer data (a
 * Serial_SBNPeerData_t struct) and its matching host's data (a
 * Serial_SBNHostData_t struct). This provides basic information about the
 * serial port (file descriptor, queue numbers, semaphore numbers).
 *
 * @param StatusPkt Status packet to fill
 * @param Peer      Peer to report status
 * @param HostList  List of hosts that may match with peer
 * @param NumHosts  Number of hosts in the SBN
 *
 * @return SBN_OK on success
 * @return SBN_ERROR if the necessary data can't be found
 */
int32 Serial_SbnReportModuleStatus(SBN_ModuleStatusPacket_t *StatusPkt, SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts) {
    Serial_SBNModuleStatus_t *ModuleStatus;
    Serial_SBNPeerData_t *PeerData;
    Serial_SBNHostData_t *HostData;
    int32 Status;

    /* Error check */
    if (StatusPkt == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not report module status: StatusPkt is null.\n");
        return SBN_ERROR;
    }

    if (Peer == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not report module status: Peer is null.\n");
        return SBN_ERROR;
    }

    if (HostList == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not report module status: HostList is null.\n");
        return SBN_ERROR;
    }

    /* Cast to the serial module's packet format to make it clearer where things are going */
    ModuleStatus = (Serial_SBNModuleStatus_t*)&StatusPkt->ModuleStatus;

    /* Find the matching host for this peer */
    Status = Serial_GetHostPeerMatchData(Peer, HostList, &HostData, &PeerData, NumHosts);
    if (Status != SBN_OK) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not report module status for peer.\n");
        return Status;
    }

    /* Copy data into the module status packet */
    CFE_PSP_MemCpy(&ModuleStatus->HostData, HostData, sizeof(Serial_SBNHostData_t));
    CFE_PSP_MemCpy(&ModuleStatus->PeerData, PeerData, sizeof(Serial_SBNPeerData_t));

    return SBN_OK;
}


/**
 * Resets a specific peer.
 * Because the peer doesn't do anything but read from a queue, we actually need
 * to find the matching host and do resets using the host's data.
 * A reset includes:
 *  - Stopping the serial port read task
 *  - Close the serial port
 *  - Emptying both data and protocol queues
 *  - Re-opening the serial port
 *  - Restarting the serial port read task
 *
 * @param Peer      Peer to reset
 * @param HostList  List of hosts that may match with peer
 * @param NumHosts  Number of hosts in the SBN
 *
 * @return SBN_OK when the peer is reset correcly
 * @return SBN_ERROR if the peer cannot be reset
 */
int32 Serial_SbnResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts) {
    Serial_SBNPeerData_t *PeerData;
    Serial_SBNHostData_t *HostData;
    int32 Status;

    /* Error check */
    if (Peer == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not reset peer: Peer is null.\n");
        return SBN_ERROR; 
    }

    if (HostList == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not reset peer: HostList is null.\n");
        return SBN_ERROR; 
    }

    /* Find the matching host for this peer */
    Status = Serial_GetHostPeerMatchData(Peer, HostList, &HostData, &PeerData, NumHosts);
    if (Status != SBN_OK) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not reset peer because no matching host was found.\n");
        return Status; 
    }

    /* Stop the read task if it's running */
    if (HostData->TaskHandle > 0) {
        Status = CFE_ES_DeleteChildTask(HostData->TaskHandle);
        if (Status != CFE_SUCCESS) {
            CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                              "Serial: Could not stop read task for host/peer %d.\n",
                              HostData->PairNum);
            return SBN_ERROR;
        }
    }

    /* Close serial port */
    OS_close(HostData->Fd);

    /* Empty the queues. These are just while loops that loop until the method returns OS_QUEUE_EMPTY */
    while (Serial_QueueRemoveNode(HostData->ProtoQ) == OS_SUCCESS) {}
    while (Serial_QueueRemoveNode(HostData->DataQ) == OS_SUCCESS) {}

    /* Re-open serial port */
    HostData->Fd = Serial_IoOpenPort(HostData->DevName, HostData->BaudRate);
    if (HostData->Fd < 0) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not re-open host %d's serial port.\n",
                          HostData->PairNum);
        return SBN_ERROR;
    }

    /* Re-start read task */
    Status = Serial_IoStartReadTask(HostData);
    if (Status != SBN_VALID) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Could not restart the read task for host %d.\n",
                          HostData->PairNum);
        return SBN_ERROR;
    }

    return SBN_OK;
}


/**
 * Searches through a list of hosts to find a matching host to the peer. If a
 * match is found, it will assign the HostData and PeerData pointers.
 * HostData and PeerData are double pointers (pointer to a struct pointer)
 * so that the struct address is assigned correctly in the calling functions.
 *
 * @param Peer      Peer to search for matches for
 * @param HostList  List of possible hosts to search
 * @param NumHosts  Number of hosts in the list
 * @param HostData  Pointer to the struct where matching host data will go
 * @param PeerData  Pointer to the struct where matching peer data will go
 *
 * @return SBN_OK if a match was found
 * @return SBN_ERROR if no match found
 */
int32 Serial_GetHostPeerMatchData(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], Serial_SBNHostData_t **HostData, Serial_SBNPeerData_t **PeerData, int32 NumHosts) {
    Serial_SBNEntry_t *HostEntry = NULL;
    int32 HostIdx;

    if (Peer->PeerData == 0) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Peer entry has no PeerData.\n");
        return SBN_ERROR;
    }

    *PeerData = (Serial_SBNPeerData_t *)Peer->PeerData;

    /* Find the host that goes with this peer. */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++) {
        if(HostList[HostIdx]->ProtocolId == SBN_SERIAL) {
            HostEntry = (Serial_SBNEntry_t *)HostList[HostIdx]->EntryData;

            if ( Serial_IsHostPeerMatch(HostEntry, (Serial_SBNEntry_t *)Peer->EntryData) ) {
                if (HostList[HostIdx]->HostData != 0) {
                    *HostData = (Serial_SBNHostData_t *)HostList[HostIdx]->HostData;
                    return SBN_OK;
                }
            }
        }
    }

    /* If the code reaches here, no match was found */
    CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_INFORMATION,
                      "Serial: Could not find matching host for peer %d.\n",
                      (*PeerData)->PairNum);

    return SBN_ERROR;
}


/**
 * Compares pair number and baud rates of the host/peer to verify that they match.
 *
 * @param Serial_SBNEntry_t *Host    Host to verify
 * @param Serial_SBNEntry_t *Peer    Peer to verify
 *
 * @return 1 if the two match, else
 * @return 0 if not
 */
int32 Serial_IsHostPeerMatch(Serial_SBNEntry_t *Host, Serial_SBNEntry_t *Peer) {
    if (Host == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in IsHostPeerMatch: Host is NULL.\n");
        return 0;
    }
    if (Peer == NULL) {
        CFE_EVS_SendEvent(SBN_INIT_EID,CFE_EVS_ERROR,
                          "Serial: Error in IsHostPeerMatch: Peer is NULL.\n");
        return 0;
    }

    return (Host->PairNum == Peer->PairNum) && (Host->BaudRate == Peer->BaudRate);
}

