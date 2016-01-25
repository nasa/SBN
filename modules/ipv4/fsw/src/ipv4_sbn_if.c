#include "ipv4_sbn_if_struct.h"
#include "ipv4_sbn_if.h"
#include "cfe.h"
#include "sbn_lib_utils.h"
#include <network_includes.h>
#include <string.h>
#include <strings.h> /* for bzero */
#include <errno.h>

/**
 * Displays peer data for the given IPv4 peer.
 *
 * @param idx   Index of IPv4 peer
 */
void SBN_ShowIPv4PeerData(int idx) {

    // TODO - check that peer is IPv4
   /* OS_printf(
            "%s:%s,Element %d,Adr=%s,Pipe=%d,PipeNm=%s,State=%s,SubCnt=%d\n",
            CFE_CPU_NAME,
            SBN.Peer[idx].Name,
            idx,
            SBN.Peer[idx].IFPeerData.IPv4Data.Addr,
            SBN.Peer[idx].Pipe,
            SBN.Peer[idx].PipeName,
            SBN_StateNum2Str(SBN.Peer[idx].State),
            SBN.Peer[idx].SubCnt);*/
}

void SBN_SendSockFailedEvent(uint32 Line, int RtnVal){

  CFE_EVS_SendEvent(SBN_SOCK_FAIL_EID,CFE_EVS_ERROR,
                  "%s:socket call failed,line %d,rtn val %d,errno=%d",
                   CFE_CPU_NAME,Line,RtnVal,errno);

}/* end SBN_SendSockFailedEvent */

void SBN_SendBindFailedEvent(uint32 Line, int RtnVal){

  CFE_EVS_SendEvent(SBN_BIND_FAIL_EID,CFE_EVS_ERROR,
                  "%s:bind call failed,line %d,rtn val %d,errno=%d",
                   CFE_CPU_NAME,Line,RtnVal,errno);

}/* end SBN_SendBindFailedEvent */

/**
 * Creates and configures a socket with the given port.
 *
 * @param Port port number
 * @return socket id
 */
int SBN_CreateSocket(int Port) {

    static struct sockaddr_in   my_addr;
    int    SockId;

    if((SockId = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        SBN_SendSockFailedEvent(__LINE__, SockId);
        return SockId;
    }

    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(Port);

    if(bind(SockId, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0 )
    {
        SBN_SendBindFailedEvent(__LINE__,SockId);
        return SockId;
    }

    #ifdef _HAVE_FCNTL_
        /*
        ** Set the socket to non-blocking
        ** This is not available to vxWorks, so it has to be
        ** Conditionally compiled in
        */
        fcntl(SockId, F_SETFL, O_NONBLOCK);
    #endif

    SBN_ClearSocket(SockId);
    return SockId;

}/* end SBN_CreateSocket */


/**
 * Clears a socket by reading out all data in it.
 * Data is discarded.
 *
 * @param SockID  ID of socket to clear
 */
void SBN_ClearSocket(int SockID) {
    struct sockaddr_in  s_addr;
    socklen_t addr_len;
    int                 i;
    int                 status;

    addr_len = sizeof(s_addr);
    bzero((char *) &s_addr, sizeof(s_addr));

    NetDataUnion DiscardData;

    /* change to while loop */
    for (i=0; i<=50; i++)
    {
        status = recvfrom(SockID, (char *)&DiscardData, sizeof(SBN_NetPkt_t),
                               MSG_DONTWAIT,(struct sockaddr *) &s_addr, &addr_len);

    if ( (status < 0) && (errno == EWOULDBLOCK) ) // TODO: add EAGAIN?
        break; /* no (more) messages */
    } /* end for */

} /* end SBN_ClearSocket */

/**
 * Checks for a single protocol message.
 *
 * @param Peer         Structure of interface data for a peer
 * @param ProtoMsgBuf  Pointer to the SBN's protocol message buffer
 * @return 1 for message available and 0 for no messages or an error
 */
int32 SBN_CheckForIPv4NetProtoMsg(SBN_InterfaceData *Peer, SBN_NetProtoMsg_t *ProtoMsgBuf) {
    struct sockaddr_in s_addr;
    int                status;
    socklen_t addr_len;
    IPv4_SBNPeerData_t *peer = Peer->PeerData;
    bzero((char *) &s_addr, sizeof(s_addr));

    addr_len = sizeof(s_addr);

    status = recvfrom(peer->ProtoSockId,
                      (char *)ProtoMsgBuf,
                      sizeof(SBN_NetProtoMsg_t),
                      MSG_DONTWAIT,
                      (struct sockaddr *) &s_addr,
                      &addr_len);

    if (status > 0) /* Positive number indicates byte length of message */
    {
        ProtoMsgBuf->Hdr.MsgSize = ntohl(ProtoMsgBuf->Hdr.MsgSize);
        ProtoMsgBuf->Hdr.Type = ntohl(ProtoMsgBuf->Hdr.Type);
        ProtoMsgBuf->Hdr.MsgSender.ProcessorId = ntohs(ProtoMsgBuf->Hdr.MsgSender.ProcessorId);
        ProtoMsgBuf->Hdr.SequenceCount = ntohs(ProtoMsgBuf->Hdr.SequenceCount);
        ProtoMsgBuf->Hdr.GapAfter = ntohs(ProtoMsgBuf->Hdr.GapAfter);
        ProtoMsgBuf->Hdr.GapTo = ntohs(ProtoMsgBuf->Hdr.GapTo);

        return SBN_TRUE; /* Message available and no errors */
    }
    else
        if ( (status <=0) && (errno != EWOULDBLOCK) && (errno != EAGAIN)) {
            CFE_EVS_SendEvent(SBN_NET_RCV_PROTO_ERR_EID,CFE_EVS_ERROR,
                              "%s:Socket recv err in CheckForNetProtoMsgs stat=%d,err=%d",
                              CFE_CPU_NAME, status, errno);
            ProtoMsgBuf->Hdr.Type = SBN_NO_MSG;
            return SBN_ERROR;
        }

    /* status = 0, so no messages and no errors */
    ProtoMsgBuf->Hdr.Type = SBN_NO_MSG;
    return SBN_NO_MSG;
}/* end SBN_CheckForNetProtoMsg */

/**
 * Receives a message from a peer over the appropriate interface.
 *
 * @param Host        Structure of interface data for a peer
 * @param DataMsgBuf  Pointer to the SBN's protocol message buffer
 * @return Bytes received on success, SBN_IF_EMPTY if empty, -1 on error
 */
int SBN_IPv4RcvMsg(SBN_InterfaceData *Host, NetDataUnion *DataMsgBuf) {

    struct sockaddr_in s_addr;
    int                status;
    socklen_t addr_len;

    bzero((char *) &s_addr, sizeof(s_addr));

    IPv4_SBNHostData_t *host;

    host = Host->HostData;
    addr_len = sizeof(s_addr);
    status = recvfrom(host->DataSockId,
                      (char *)DataMsgBuf,
                      SBN_MAX_MSG_SIZE,
                      MSG_DONTWAIT,
                      (struct sockaddr *) &s_addr,
                      &addr_len);

    if ( (status < 0) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)) )
        return SBN_IF_EMPTY;

    DataMsgBuf->Hdr.MsgSize = ntohl(DataMsgBuf->Hdr.MsgSize);
    DataMsgBuf->Hdr.Type = ntohl(DataMsgBuf->Hdr.Type);
    DataMsgBuf->Hdr.SequenceCount = ntohs(DataMsgBuf->Hdr.SequenceCount);
        return status;
}

/**
 * Parses the peer data file into SBN_FileEntry_t structures.
 * Parses information that is common to all interface types and
 * allows individual interface modules to parse out interface-
 * specfic information.
 *
 * @param FileEntry  Interface description line as read from file
 * @param LineNum    The line number in the peer file
 * @param EntryAddr  Address in which to return the filled entry struct
 * @return SBN_OK if entry is parsed correctly, SBN_ERROR otherwise
 *
 */
int32 SBN_ParseIPv4FileEntry(char *FileEntry, uint32 LineNum, void **EntryAddr) {
    int     ScanfStatus;
    char    Addr[16];
    int     DataPort;
    int     ProtoPort;

    /*
    ** Using sscanf to parse the string.
    ** Currently no error handling
    */
    ScanfStatus = sscanf(FileEntry, "%s %d %d", Addr, &DataPort, &ProtoPort);

    /*
    ** Check to see if the correct number of items were parsed
    */
    if (ScanfStatus != IPV4_ITEMS_PER_FILE_LINE) {
        CFE_EVS_SendEvent(SBN_INV_LINE_EID,CFE_EVS_ERROR,
                "%s:Invalid SBN peer file line,exp %d items,found %d",
                CFE_CPU_NAME, SBN_IPv4_ITEMS_PER_FILE_LINE, ScanfStatus);
        return SBN_ERROR;
    }

    IPv4_SBNEntry_t *entry = malloc(sizeof(IPv4_SBNEntry_t));
    *EntryAddr = entry;

    strncpy(entry->Addr, Addr, 16);
    entry->DataPort = DataPort;
    entry->ProtoPort = ProtoPort;

    return SBN_OK;
}


/**
 * Initializes an IPv4 host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_OK on success, error code otherwise
 */
int32 SBN_InitIPv4IF(SBN_InterfaceData *Data) {

    IPv4_SBNEntry_t *entry = Data->EntryData;

    /* CPU names match - this is host data.
       Create msg interface when we find entry matching its own name
       because the self entry has port info needed to bind this interface. */
    if(strncmp(Data->Name, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH) == 0){

        /* create, fill, and store an IPv4-specific host data structure */
        IPv4_SBNHostData_t *host = malloc(sizeof(IPv4_SBNHostData_t));

        host->DataRcvPort = entry->DataPort;
        host->DataSockId = SBN_CreateSocket(host->DataRcvPort);
        if(host->DataSockId == SBN_ERROR){
            return SBN_ERROR;
        }

        host->ProtoXmtPort = entry->ProtoPort;
        host->ProtoSockId = SBN_CreateSocket(host->ProtoXmtPort);

        if(host->ProtoSockId == SBN_ERROR){
            return SBN_ERROR;
        }

        Data->HostData = host;
        return SBN_HOST;
    }
    /* CPU names do not match - this is peer data. */
    else {
        /* create, fill, and store an IPv4-specific host data structure */
        IPv4_SBNPeerData_t *peer = malloc(sizeof(IPv4_SBNPeerData_t));

        strncpy(peer->Addr, entry->Addr, sizeof(entry->Addr));
        peer->DataPort = entry->DataPort;
        peer->ProtoRcvPort = entry->ProtoPort;
        peer->ProtoSockId  = SBN_CreateSocket(peer->ProtoRcvPort);
        if(peer->ProtoSockId == SBN_ERROR){
            return SBN_ERROR;
        }
        Data->PeerData = peer;
        return SBN_PEER;
    }
}

/**
 * Sends a message to a peer over an Ethernet IPv4 interface.
 *
 * @param MsgType      Type of Message
 * @param MsgSize      Size of Message
 * @param HostList     The array of SBN_InterfaceData structs that describes the host
 * @param SenderPtr    Sender information
 * @param IfData       The SBN_InterfaceData struct describing this peer
 * @param ProtoMsgBuf  Protocol message
 * @param DataMsgBuf   Data message
 */
int32 SBN_SendIPv4NetMsg(uint32 MsgType, uint32 MsgSize, SBN_InterfaceData *HostList[], int32 NumHosts, CFE_SB_SenderId_t *SenderPtr, SBN_InterfaceData *IfData, SBN_NetProtoMsg_t *ProtoMsgBuf, NetDataUnion *DataMsgBuf) {
    static struct sockaddr_in s_addr;
    int    status, found = 0;
    IPv4_SBNPeerData_t *peer;
    IPv4_SBNHostData_t *host;
    uint32 HostIdx;


    /* Find the host that goes with this peer.  There should only be one
       ethernet host */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++) {
        if(HostList[HostIdx]->ProtocolId == SBN_IPv4) {
            found = 1;
            host = HostList[HostIdx]->HostData;
            break;
        }
    }
    if(found != 1) {
        OS_printf("No IPv4 Host Found!\n");
        return SBN_ERROR;
    }

    peer = IfData->PeerData;
    bzero((char *) &s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(peer->Addr);

 //   if(MsgType == SBN_APP_MSG) {

        /* If my peer sent this message, don't send it back to them, avoids loops */
 //       if (CFE_PSP_GetProcessorId() == SenderPtr->ProcessorId) {

            /* Then no break, so fill in the sender application infomation */
 //           strncpy((char *)&(DataMsgBuf->Hdr.MsgSender.AppName), &SenderPtr->AppName[0], OS_MAX_API_NAME);
 //           DataMsgBuf->Hdr.MsgSender.ProcessorId = SenderPtr->ProcessorId;
 //       }
 //       else {
 //           return SBN_OK;
 //       }
 //   }
    if(SBN_LIB_MsgTypeIsData(MsgType)) {
	SBN_Hdr_t orig_hdr;


        s_addr.sin_port = htons(peer->DataPort);

        /* Initialize the SBN hdr of the outgoing network message */
        strncpy((char *)&DataMsgBuf->Hdr.SrcCpuName,CFE_CPU_NAME,SBN_MAX_PEERNAME_LENGTH);

        DataMsgBuf->Hdr.Type = MsgType;

        memcpy(&orig_hdr, &(DataMsgBuf->Hdr), sizeof(orig_hdr));
        DataMsgBuf->Hdr.MsgSize = htonl(orig_hdr.MsgSize);
        DataMsgBuf->Hdr.Type = htonl(orig_hdr.Type);
        DataMsgBuf->Hdr.SequenceCount = htons(orig_hdr.SequenceCount);
        DataMsgBuf->Hdr.MsgSender.ProcessorId = ntohs(orig_hdr.MsgSender.ProcessorId);
        DataMsgBuf->Hdr.SequenceCount = ntohs(orig_hdr.SequenceCount);
        DataMsgBuf->Hdr.GapAfter = ntohs(orig_hdr.GapAfter);
        DataMsgBuf->Hdr.GapTo = ntohs(orig_hdr.GapTo);

        status = sendto(host->DataSockId,
                        (char *)DataMsgBuf,
                        MsgSize,
                        0,
                        (struct sockaddr *) &s_addr,
                        sizeof(s_addr) );

        memcpy(&(DataMsgBuf->Hdr), &orig_hdr, sizeof(orig_hdr));
 //       printf("IPv4 Sending Msg with Seq = %d, MsgSize = %d, Status = %d\n", DataMsgBuf->Hdr.SequenceCount, MsgSize, status);
    }
    else if(SBN_LIB_MsgTypeIsProto(MsgType)) {
	SBN_Hdr_t orig_hdr;

        s_addr.sin_port = htons(host->ProtoXmtPort); /* dest port is always the same for each IP addr */

        ProtoMsgBuf->Hdr.Type = MsgType;
        strncpy(ProtoMsgBuf->Hdr.SrcCpuName, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH);

        memcpy(&orig_hdr, &(ProtoMsgBuf->Hdr), sizeof(orig_hdr));
        ProtoMsgBuf->Hdr.MsgSize = htonl(ProtoMsgBuf->Hdr.MsgSize);
        ProtoMsgBuf->Hdr.Type = htonl(ProtoMsgBuf->Hdr.Type);
        ProtoMsgBuf->Hdr.SequenceCount = htons(ProtoMsgBuf->Hdr.SequenceCount);
        ProtoMsgBuf->Hdr.MsgSender.ProcessorId = ntohs(orig_hdr.MsgSender.ProcessorId);
        ProtoMsgBuf->Hdr.SequenceCount = ntohs(orig_hdr.SequenceCount);
        ProtoMsgBuf->Hdr.GapAfter = ntohs(orig_hdr.GapAfter);
        ProtoMsgBuf->Hdr.GapTo = ntohs(orig_hdr.GapTo);

        status = sendto(host->ProtoSockId,
                        (char *)ProtoMsgBuf,
                        MsgSize,
                        0,
                        (struct sockaddr *) &s_addr,
                        sizeof(s_addr) );

        memcpy(&(ProtoMsgBuf->Hdr), &orig_hdr, sizeof(orig_hdr));
    }
    else {
        OS_printf("Unexpected msg type\n");
        /*  TODO send event to indicate unexpected msgtype */
        status = SBN_ERROR;
    } /* end switch */

    return (status);
}/* end SBN_SendNetMsg */

/**
 * Verifies that there is, in fact, and ethernet host.
 *
 * @param Peer      The peer to verify
 * @param HostList  The list of hosts in the SBN
 * @param NumHosts  The number of hosts in the SBN
 * @return SBN_VALID if there is an ethernet host, SBN_NOT_VALID otherwise
 */
int32 IPv4_VerifyPeerInterface(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts) {
    int32 HostIdx;
    int32 found;

    /* Find the host that goes with this peer.  There should only be one
       ethernet host */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++) {
        if(HostList[HostIdx]->ProtocolId == SBN_IPv4) {
            found = 1;
            break;
        }
    }

    if(found == 1) {
        return SBN_VALID;
    }
    else {
        return SBN_NOT_VALID;
    }
}

/**
 * An IPv4 host doesn't necessarily need a peer, so this always returns true.
 *
 * @param Host      The host to verify
 * @param PeerList  The list of peers in the SBN
 * @param NumPeers  The number of peers in the SBN
 * @return SBN_VALID
 */
int32 IPv4_VerifyHostInterface(SBN_InterfaceData *Host, SBN_PeerData_t *PeerList, int32 NumPeers) {
    return SBN_VALID;
}

/**
 * Reports the status of the ethernet peer.
 * Not yet implemented.
 *
 * @param Packet    Status packet to fill
 * @param Peer      The peer for which to report status
 * @param HostList  The list of hosts in the SBN
 * @param NumHosts  The number of hosts in the SBN
 * @return SBN_NOT_IMPLEMENTED
 */
int32 IPv4_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet, SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts) {
    return SBN_NOT_IMPLEMENTED;
}

/**
 * Resets the specified ethernet peer.
 * Not yet implemented.
 *
 * @param Peer      The peer to reset
 * @param HostList  The list of hosts in the SBN
 * @param NumHosts  The number of hosts in the SBN
 * @return SBN_NOT_IMPLEMENTED
 */
int32 IPv4_ResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts) {
    return SBN_NOT_IMPLEMENTED;
}
