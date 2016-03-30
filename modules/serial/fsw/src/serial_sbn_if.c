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
#include "serial_events.h"
#include <string.h>

/**
 * Receives a message from a peer over the appropriate interface.
 * Read pattern: search data for sync code, get message size, get message payload
 *
 * @param Host       Structure of interface data for the host
 * @param MsgBuf     Pointer to the SBN's protocol message buffer
 *
 * @return SB_OK on success
 * @return SBN_ERROR on error
 */
int Serial_SbnReceiveMsg(SBN_InterfaceData *Data, NetDataUnion *MsgBuf)
{
    uint32 MsgSize = 0;
    Serial_SBNHostData_t *host = NULL;

    if(Data == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_RECEIVE_EID, CFE_EVS_ERROR,
            "Serial: Error in SerialRcvMsg: Data is NULL.\n");
        return SBN_ERROR;
    }/* end if */
    if(MsgBuf == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_RECEIVE_EID, CFE_EVS_ERROR,
            "Serial: Error in SerialRcvMsg: MsgBuf is NULL.\n");
        return SBN_ERROR;
    }/* end if */

    host = (Serial_SBNHostData_t *)(Data->InterfacePvt);
    MsgSize = Serial_QueueGetMsg(host->Queue, host->SemId, MsgBuf);
    if(MsgSize <= 0) return SBN_ERROR;
    return SBN_OK;
}/* end Serial_SbnReceiveMsg */

#ifdef _osapi_confloader_

int SBN_LoadSerialEntry(const char **row, int fieldcount, void *entryptr)
{   
    Serial_SBNEntry_t *entry = (Serial_SBNEntry_t *)entryptr;

    if(fieldcount != SBN_SERIAL_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
            "Invalid SBN peer file line, exp %d items,found %d",
            SBN_SERIAL_ITEMS_PER_FILE_LINE, fieldcount);
        return SBN_ERROR;
    }/* end if */

    entry->PairNum = atoi(row[0]);
    strncpy(entry->DevNameHost, row[1], sizeof(entry->DevNameHost));
    entry->BaudRate = atoi(row[2]);

    return SBN_OK;
}/* end SBN_LoadSerialEntry */

#else /* ! _osapi_confloader_ */

/**
 * Parses the peer data file into SBN_FileEntry_t structures.
 *
 * @param FileEntry  Interface description line as read from file
 * @param LineNum    The line number in the peer file
 * @param data       Entry data structure, this loads the InterfacePvt
 *
 * @return SBN_OK if entry is parsed correctly
 * @return SBN_ERROR on error
 */
int Serial_SbnParseInterfaceFileEntry(char *FileEntry, uint32 LineNum,
    void *entryptr)
{
    Serial_SBNEntry_t *entry = (Serial_SBNEntry_t *)entryptr;
    int ScanfStatus = 0;
    unsigned int BaudRate = 0, PairNum = 0;
    char DevNameHost[SBN_SERIAL_MAX_CHAR_NAME];

    /*
    ** Using sscanf to parse the string.
    ** Currently no error handling
    */
    ScanfStatus = sscanf(FileEntry, "%u %s %u", &PairNum, DevNameHost,
        &BaudRate);
    DevNameHost[SBN_SERIAL_MAX_CHAR_NAME-1] = 0;

    /* Check to see if the correct number of items were parsed */
    if(ScanfStatus != SBN_SERIAL_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
            "Invalid SBN peer file line, exp %d items,found %d",
            SBN_SERIAL_ITEMS_PER_FILE_LINE, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    entry->PairNum  = PairNum;
    entry->BaudRate = BaudRate;
    strncpy(entry->DevNameHost, DevNameHost,
        sizeof(entry->DevNameHost));

    return SBN_OK;
}/* end Serial_SbnParseInterfaceFileEntry */

#endif /* _osapi_confloader_ */

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
int Serial_SbnInitPeerInterface(SBN_InterfaceData *Data)
{
    Serial_SBNEntry_t *entry = NULL;
    char name[20];
    int32 Status = 0;

    if(Data == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
            "Serial: Cannot initialize interface! Interface data is null.\n");
        return SBN_ERROR;
    }/* end if */

    entry = (Serial_SBNEntry_t *)(Data->InterfacePvt);

    /* CPU names match - this is host data. */
    if(strncmp(Data->Name, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH) == 0)
    {
        /* create, fill, and store Serial-specific host data structure */
        Serial_SBNHostData_t host;

        memset(&host, 0, sizeof(host));

        /* open serial device and set options */
        Status = Serial_IoOpenPort(entry->DevNameHost, entry->BaudRate,
            &host.Fd);
        if(Status != SBN_OK)
        {
            return Status;
        }/* end if */

        strncpy(host.DevName, entry->DevNameHost, sizeof(host.DevName));
        host.PairNum  = entry->PairNum;
        host.BaudRate = entry->BaudRate;

        /* Create data queue semaphor */
        snprintf(name, sizeof(name), "Sem%d", host.PairNum);
        if(OS_BinSemCreate(&host.SemId, name, OS_SEM_FULL, 0) != OS_SUCCESS)
        {
            CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
                "Serial: Error creating data semaphore for host %d.\n",
                host.PairNum);
        }/* end if */

        /* Create data queue */
        snprintf(name, sizeof(name), "SerialQueue%d", host.PairNum);
        if(OS_QueueCreate(&host.Queue, name, SBN_SERIAL_QUEUE_DEPTH,
            sizeof(uint32), 0) != OS_SUCCESS)
        {
            CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
                "Serial: Error creating data queue for host %d.\n",
                host.PairNum);
        }/* end if */

        memcpy(Data->InterfacePvt, &host, sizeof(host));

        return SBN_HOST;
    }
    else /* CPU names do not match - this is peer data. */
    {
        /* create, fill, and store peer data structure */
        Serial_SBNPeerData_t peer;

        memset(&peer, 0, sizeof(peer));

        peer.PairNum  = entry->PairNum;
        peer.BaudRate = entry->BaudRate;

        memcpy(Data->InterfacePvt, &peer, sizeof(peer));

        return SBN_PEER;
    }/* end if */
}/* end Serial_SbnInitPeerInterface */


/**
 * Sends a message to a peer over a Serial interface.
 *
 * @param HostList     The array of SBN_InterfaceData structs that describes
 *                     the host.
 * @param SenderPtr    Sender information
 * @param IfData       The SBN_InterfaceData struct describing this peer
 * @param MsgBuf       Data message
 *
 * @return number of bytes written on success
 * @return SBN_ERROR on error
 */
int Serial_SbnSendNetMsg(SBN_InterfaceData *HostList[], int32 NumHosts,
    SBN_SenderId_t *SenderPtr, SBN_InterfaceData *IfData, NetDataUnion *MsgBuf)
{
    Serial_SBNEntry_t *peer = NULL;
    Serial_SBNEntry_t *host_tmp = NULL;
    Serial_SBNHostData_t *host = NULL;
    uint32 HostIdx;

    /* Check pointer arguments used for all cases for null */
    if(HostList == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_SEND_EID, CFE_EVS_ERROR,
            "Serial: Error in SendSerialNetMsg: HostList is NULL.\n");
        return SBN_ERROR;
    }/* end if */

    if(IfData == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_SEND_EID, CFE_EVS_ERROR,
            "Serial: Error in SendSerialNetMsg: IfData is NULL.\n");
        return SBN_ERROR;
    }/* end if */

    if(MsgBuf == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_SEND_EID, CFE_EVS_ERROR,
            "Serial: Error in SendSerialNetMsg: MsgBuf is NULL.\n");
        return SBN_ERROR;
    }/* end if */

    /* Find the host that goes with this peer. */
    peer = (Serial_SBNEntry_t *)(IfData->InterfacePvt);
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++)
    {
        if(HostList[HostIdx]->ProtocolId == SBN_SERIAL)
        {
            host_tmp = (Serial_SBNEntry_t *)(HostList[HostIdx]->InterfacePvt);
            if(Serial_IsHostPeerMatch(host_tmp, peer))
            {
                host = (Serial_SBNHostData_t *)HostList[HostIdx]->InterfacePvt;
                break;
            }/* end if */
        }/* end if */
    }/* end for */

    if(!host)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_SEND_EID, CFE_EVS_ERROR,
            "Serial: No Serial Host Found!\n");
        return SBN_ERROR;
    }/* end if */

    switch(MsgBuf->Hdr.Type)
    {
        /* Data messages */
        case SBN_APP_MSG:
            /* TODO: Unsure if we need this logic, may never happen */
            if(SenderPtr == NULL)
            {
                CFE_EVS_SendEvent(SBN_SERIAL_SEND_EID, CFE_EVS_ERROR,
                    "Serial: Error in SendSerialNetMsg: SenderPtr is NULL.\n");
                return SBN_ERROR;
            }/* end if */

            /* If my peer sent this message, don't send it back to them,
             * avoids loops
             */
            if(CFE_PSP_GetProcessorId() != SenderPtr->ProcessorId) break;

            strncpy((char *)&(MsgBuf->Hdr.MsgSender.AppName),
                &(SenderPtr->AppName[0]), OS_MAX_API_NAME);
            MsgBuf->Hdr.MsgSender.ProcessorId = SenderPtr->ProcessorId;
            /* Fall through to the next case */

        case SBN_SUBSCRIBE_MSG:
        case SBN_UN_SUBSCRIBE_MSG:
        case SBN_ANNOUNCE_MSG:
        case SBN_ANNOUNCE_ACK_MSG:
        case SBN_HEARTBEAT_MSG:
        case SBN_HEARTBEAT_ACK_MSG:
            return Serial_IoWriteMsg(host->Fd, MsgBuf);

        default:
            CFE_EVS_SendEvent(SBN_SERIAL_SEND_EID, CFE_EVS_ERROR,
                "Serial: Unexpected message type!\n");
    }/* end switch */
    return SBN_ERROR;
}/* end SBN_SendNetMsg */


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
int Serial_SbnVerifyPeerInterface(SBN_InterfaceData *Peer,
        SBN_InterfaceData *HostList[], int NumHosts)
{
    int HostIdx = 0;
    Serial_SBNEntry_t *HostEntry = NULL;
    Serial_SBNEntry_t *PeerEntry = NULL;

    if(Peer == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
            "Serial: Error in Serial_VerifyPeerInterface: Peer is NULL.\n");
        return SBN_NOT_VALID;
    }/* end if */

    if(HostList == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
            "Serial: Error in Serial_VerifyPeerInterface: Hosts is NULL.\n");
        return SBN_NOT_VALID;
    }/* end if */

    PeerEntry = (Serial_SBNEntry_t *)Peer->InterfacePvt;

    /* Find the host that goes with this peer. */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++)
    {
        if(HostList[HostIdx]->ProtocolId == SBN_SERIAL)
        {
            HostEntry = (Serial_SBNEntry_t *)HostList[HostIdx]->InterfacePvt;

            if(Serial_IsHostPeerMatch(HostEntry, PeerEntry))
            {
                return SBN_VALID;
            }/* end if */
        }/* end if */
    }/* end for */

    return SBN_NOT_VALID;
}/* end Serial_SbnVerifyPeerInterface */


/**
 * Iterates through the list of all peer interfaces to see if there is a
 * match for the specified host interface. For the serial interface, there
 * should be exactly one peer per host. Once a valid host/peer match is found,
 * the task to read from the serial port is started for that host.
 *
 * @param SBN_InterfaceData *Host    Host to verify
 * @param SBN_PeerData_t *Peers      List of peers to check against the host
 * @param int32 NumPeers             Number of peers in the SBN
 *
 * @return SBN_VALID     if the required match exists, else
 * @return SBN_NOT_VALID if not
 */
int Serial_SbnVerifyHostInterface(SBN_InterfaceData *Data,
    SBN_PeerData_t *PeerList, int NumPeers)
{
    int PeerIdx;
    Serial_SBNEntry_t *PeerEntry = NULL;
    Serial_SBNEntry_t *HostEntry = NULL;

    if(Data == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
            "Serial: Error in Serial_VerifyHostInterface: Data is NULL.\n");
        return SBN_NOT_VALID;
    }/* end if */

    if(PeerList == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_CONFIG_EID, CFE_EVS_ERROR,
            "Serial: Error in Serial_VerifyHostInterface: PeerList is NULL.\n");
        return SBN_NOT_VALID;
    }/* end if */

    HostEntry = (Serial_SBNEntry_t *)Data->InterfacePvt;

    /* Find the peer that goes with this host. */
    for(PeerIdx = 0; PeerIdx < NumPeers; PeerIdx++)
    {
        if(PeerList[PeerIdx].ProtocolId == SBN_SERIAL)
        {
            PeerEntry = (Serial_SBNEntry_t *)(PeerList[PeerIdx].IfData)
                ->InterfacePvt;

            if(Serial_IsHostPeerMatch(HostEntry, PeerEntry))
            {
                /* Start serial read task. */
                return Serial_IoStartReadTask(
                    (Serial_SBNHostData_t *)Data->InterfacePvt);
            }/* end if */
        }/* end if */
    }/* end for */

    return SBN_NOT_VALID;
}/* end Serial_SbnVerifyHostInterface */


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
int Serial_SbnReportModuleStatus(SBN_ModuleStatusPacket_t *StatusPkt,
    SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int NumHosts)
{
    int Status = 0;
    Serial_SBNModuleStatus_t *ModuleStatus = NULL;
    Serial_SBNPeerData_t *PeerData = NULL;
    Serial_SBNHostData_t *HostData = NULL;

    /* Error check */
    if(StatusPkt == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not report module status: StatusPkt is null.\n");
        return SBN_ERROR;
    }/* end if */

    if(Peer == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not report module status: Peer is null.\n");
        return SBN_ERROR;
    }/* end if */

    if(HostList == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not report module status: HostList is null.\n");
        return SBN_ERROR;
    }/* end if */

    /* Cast to the serial module's packet format to make it clearer
     * where things are going
     */
    ModuleStatus = (Serial_SBNModuleStatus_t*)&StatusPkt->ModuleStatus;

    /* Find the matching host for this peer */
    Status = Serial_GetHostPeerMatchData(Peer, HostList, &HostData, &PeerData, NumHosts);
    if(Status != SBN_OK)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not report module status for peer.\n");
        return Status;
    }/* end if */

    /* Copy data into the module status packet */
    CFE_PSP_MemCpy(&ModuleStatus->HostData, HostData,
        sizeof(Serial_SBNHostData_t));
    CFE_PSP_MemCpy(&ModuleStatus->PeerData, PeerData,
        sizeof(Serial_SBNPeerData_t));

    return SBN_OK;
}/* end Serial_SbnReportModuleStatus */


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
int Serial_SbnResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
    int NumHosts)
{
    Serial_SBNPeerData_t *PeerData = NULL;
    Serial_SBNHostData_t *HostData = NULL;
    int Status = 0;

    /* Error check */
    if(Peer == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not reset peer: Peer is null.\n");
        return SBN_ERROR; 
    }/* end if */

    if(HostList == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not reset peer: HostList is null.\n");
        return SBN_ERROR; 
    }/* end if */

    /* Find the matching host for this peer */
    Status = Serial_GetHostPeerMatchData(Peer, HostList, &HostData, &PeerData,
        NumHosts);
    if(Status != SBN_OK)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not reset peer because no matching host "
            "was found.\n");
        return Status; 
    }/* end if */

    /* Stop the read task if it's running */
    if(HostData->TaskHandle > 0)
    {
        Status = CFE_ES_DeleteChildTask(HostData->TaskHandle);
        if(Status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
                "Serial: Could not stop read task for host/peer %d.\n",
                HostData->PairNum);
            return SBN_ERROR;
        }/* end if */
    }/* end if */

    /* Close serial port */
    OS_close(HostData->Fd);

    /* Empty the queues. These are just while loops that loop until the method
     * returns OS_QUEUE_EMPTY
     */
    while(Serial_QueueRemoveNode(HostData->Queue) == OS_SUCCESS)
    {
        /* do nothing */
    }/* end while */

    /* Re-open serial port */
    Status = Serial_IoOpenPort(HostData->DevName, HostData->BaudRate,
        &HostData->Fd);
    if(Status != SBN_OK)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not re-open host %d's serial port.\n",
            HostData->PairNum);
        return Status;
    }/* end if */

    /* Re-start read task */
    Status = Serial_IoStartReadTask(HostData);
    if(Status != SBN_VALID)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Could not restart the read task for host %d.\n",
            HostData->PairNum);
        return Status;
    }/* end if */

    return SBN_OK;
}/* end Serial_SbnResetPeer */


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
int Serial_GetHostPeerMatchData(SBN_InterfaceData *Data,
    SBN_InterfaceData *HostList[], Serial_SBNHostData_t **HostData,
    Serial_SBNPeerData_t **PeerData, int NumHosts)
{
    Serial_SBNEntry_t *HostEntry = NULL;
    int HostIdx = 0;

    if(Data->InterfacePvt == 0)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: InterfaceData entry has no interface private.\n");
        return SBN_ERROR;
    }/* end if */

    *PeerData = (Serial_SBNPeerData_t *)Data->InterfacePvt;

    /* Find the host that goes with this peer. */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++)
    {
        if(HostList[HostIdx]->ProtocolId == SBN_SERIAL)
        {
            HostEntry = (Serial_SBNEntry_t *)HostList[HostIdx]->InterfacePvt;

            if(Serial_IsHostPeerMatch(HostEntry,
                (Serial_SBNEntry_t *)Data->InterfacePvt))
            {
                if(HostList[HostIdx]->InterfacePvt != 0)
                {
                    *HostData = (Serial_SBNHostData_t *)HostList[HostIdx]
                        ->InterfacePvt;
                    return SBN_OK;
                }/* end if */
            }/* end if */
        }/* end if */
    }/* end for */

    /* If the code reaches here, no match was found */
    CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_INFORMATION,
        "Serial: Could not find matching host for peer %d.\n",
        (*PeerData)->PairNum);

    return SBN_ERROR;
}/* end Serial_GetHostPeerMatchData */


/**
 * Compares pair number and baud rates of the host/peer to verify that they
 * match.
 *
 * @param Serial_SBNEntry_t *Host    Host to verify
 * @param Serial_SBNEntry_t *Peer    Peer to verify
 *
 * @return 1 if the two match, else
 * @return 0 if not
 */
int Serial_IsHostPeerMatch(Serial_SBNEntry_t *Host, Serial_SBNEntry_t *Peer)
{
    if(Host == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Error in IsHostPeerMatch: Host is NULL.\n");
        return 0;
    }/* end if */

    if(Peer == NULL)
    {
        CFE_EVS_SendEvent(SBN_SERIAL_EID, CFE_EVS_ERROR,
            "Serial: Error in IsHostPeerMatch: Peer is NULL.\n");
        return 0;
    }/* end if */

    return (Host->PairNum == Peer->PairNum)
        && (Host->BaudRate == Peer->BaudRate);
}/* end Serial_IsHostPeerMatch */
