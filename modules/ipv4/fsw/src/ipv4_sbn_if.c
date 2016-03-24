#include "ipv4_sbn_if_struct.h"
#include "ipv4_sbn_if.h"
#include "ipv4_events.h"
#include "cfe.h"
#include "sbn_lib_utils.h"
#include <network_includes.h>
#include <string.h>
#include <strings.h> /* for bzero */
#include <errno.h>

/**
 * Creates and configures a socket with the given port.
 *
 * @param Port port number
 * @return socket id
 */
int SBN_CreateSocket(char *Addr, int Port)
{
    static struct sockaddr_in   my_addr;
    int                         SockId = 0;

    CFE_EVS_SendEvent(SBN_IPV4_SOCK_EID, CFE_EVS_DEBUG,
        "Creating socket for %s:%d", Addr, Port);

    if((SockId = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        CFE_EVS_SendEvent(SBN_IPV4_SOCK_EID, CFE_EVS_ERROR,
            "%s:socket call failed,line %d,rtn val %d,errno=%d",
            CFE_CPU_NAME, __LINE__, SockId, errno);
        return SockId;
    }/* end if */

    my_addr.sin_addr.s_addr = inet_addr(Addr);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(Port);

    if(bind(SockId, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0 )
    {
        CFE_EVS_SendEvent(SBN_IPV4_SOCK_EID, CFE_EVS_ERROR,
            "%s:bind call failed,line %d,rtn val %d,errno=%d",
            CFE_CPU_NAME, __LINE__, SockId, errno);
        return SockId;
    }/* end if */

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
void SBN_ClearSocket(int SockId)
{
    struct sockaddr_in  s_addr;
    socklen_t           addr_len = 0;
    int                 i = 0;
    int                 status = 0;
    NetDataUnion        DiscardData;

    addr_len = sizeof(s_addr);
    bzero((char *) &s_addr, sizeof(s_addr));

    CFE_EVS_SendEvent(SBN_IPV4_SOCK_EID, CFE_EVS_DEBUG,
        "Clearing socket %d", SockId);

    /* change to while loop */
    for (i = 0; i <= 50; i++)
    {
        status = recvfrom(SockId, (char *)&DiscardData, sizeof(SBN_NetPkt_t),
            MSG_DONTWAIT,(struct sockaddr *) &s_addr, &addr_len);
        if ((status < 0) && (errno == EWOULDBLOCK)) // TODO: add EAGAIN?
            break; /* no (more) messages */
    }/* end for */
}/* end SBN_ClearSocket */

/**
 * Receives a message from a peer over the appropriate interface.
 *
 * @param Data        Structure of interface data for a peer
 * @param MsgBuf  Pointer to the SBN's protocol message buffer
 * @return Bytes received on success, SBN_IF_EMPTY if empty, -1 on error
 */
int SBN_IPv4RcvMsg(SBN_InterfaceData *Data, NetDataUnion *MsgBuf)
{
    ssize_t             Received = 0, TotalReceived = 0;
    struct sockaddr_in  s_addr;
    socklen_t           addr_len = 0;
    IPv4_SBNHostData_t  *host = (IPv4_SBNHostData_t *)Data->InterfacePvt;

    bzero((char *) &s_addr, sizeof(s_addr));

    while(1)
    {
        addr_len = sizeof(s_addr);
        Received = recvfrom(host->SockId, (char *)MsgBuf + TotalReceived,
            SBN_MAX_MSG_SIZE - TotalReceived,
            MSG_DONTWAIT, (struct sockaddr *) &s_addr, &addr_len);

        if (Received == 0) return SBN_IF_EMPTY;

        if ((Received < 0) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)))
            return SBN_ERROR;

        TotalReceived += Received;
        if (TotalReceived < sizeof(SBN_Hdr_t)) continue;

        /* SBN over the wire uses network (big-endian) byte order */
        MsgBuf->Hdr.MsgSize = ntohl(MsgBuf->Hdr.MsgSize);
        MsgBuf->Hdr.MsgSender.ProcessorId =
            ntohl(MsgBuf->Hdr.MsgSender.ProcessorId);
        MsgBuf->Hdr.Type = ntohl(MsgBuf->Hdr.Type);
        MsgBuf->Hdr.SequenceCount = ntohs(MsgBuf->Hdr.SequenceCount);

        if (TotalReceived >= MsgBuf->Hdr.MsgSize) return SBN_OK;
    }
    return SBN_OK;
}/* end SBN_IPv4RcvMsg */

#ifdef _osapi_confloader_

int SBN_LoadIPv4Entry(const char **row, int fieldcount, void *entryptr)
{
    IPv4_SBNEntry_t *entry = (IPv4_SBNEntry_t *)entryptr;
    if (fieldcount < IPV4_ITEMS_PER_FILE_LINE)
    {
        return SBN_ERROR;
    }/* end if */

    strncpy(entry->Addr, row[0], 16);
    entry->Port = atoi(row[1]);

    return SBN_OK;
}/* end SBN_LoadIPv4Entry */

#else /* ! _osapi_confloader_ */

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
int SBN_ParseIPv4FileEntry(char *FileEntry, uint32 LineNum, void *entryptr)
{
    IPv4_SBNEntry_t *entry = (IPv4_SBNEntry_t *)entryptr;

    int     ScanfStatus = 0;
    char    Addr[16];
    int     Port = 0;

    /*
     * Using sscanf to parse the string.
     * Currently no error handling
     */
    ScanfStatus = sscanf(FileEntry, "%s %d", Addr, &Port);

    /*
     * Check to see if the correct number of items were parsed
     */
    if (ScanfStatus != IPV4_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_IPV4_CONFIG_EID,CFE_EVS_ERROR,
                "%s:Invalid SBN peer file line,exp %d items,found %d",
                CFE_CPU_NAME, IPV4_ITEMS_PER_FILE_LINE, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    strncpy(entry->Addr, Addr, 16);
    entry->Port = Port;

    return SBN_OK;
}/* end SBN_ParseIPv4FileEntry */

#endif /* _osapi_confloader_ */

/**
 * Initializes an IPv4 host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_OK on success, error code otherwise
 */
int SBN_InitIPv4IF(SBN_InterfaceData *Data)
{
    IPv4_SBNEntry_t *entry = (IPv4_SBNEntry_t *)Data->InterfacePvt;

    if(strncmp(Data->Name, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH) == 0)
    {
        /* CPU names match - this is host data.
         * Create msg interface when we find entry matching its own name
         * because the self entry has port info needed to bind this interface.
         */
        /* create, fill, and store an IPv4-specific host data structure */
        IPv4_SBNHostData_t host;

        memset(&host, 0, sizeof(host));

        strncpy(host.Addr, entry->Addr, sizeof(entry->Addr));
        host.Port = entry->Port;
        host.SockId = SBN_CreateSocket(host.Addr, host.Port);
        if(host.SockId == SBN_ERROR){
            return SBN_ERROR;
        }/* end if */

        memcpy(Data->InterfacePvt, &host, sizeof(host));

        return SBN_HOST;
    }
    else
    {
        /* CPU names do not match - this is peer data. */
        /* create, fill, and store an IPv4-specific host data structure */
        IPv4_SBNPeerData_t peer;

        memset(&peer, 0, sizeof(peer));

        strncpy(peer.Addr, entry->Addr, sizeof(entry->Addr));
        peer.Port = entry->Port;

        memcpy(Data->InterfacePvt, &peer, sizeof(peer));

        return SBN_PEER;
    }/* end if */
}/* end SBN_InitIPv4IF */

/**
 * Sends a message to a peer over an Ethernet IPv4 interface.
 *
 * @param MsgType      Type of Message
 * @param MsgSize      Size of Message
 * @param HostList     The array of SBN_InterfaceData structs that describes the host
 * @param SenderPtr    Sender information
 * @param IfData       The SBN_InterfaceData struct describing this peer
 * @param MsgBuf   Data message
 */

int SBN_SendIPv4NetMsg(uint32 MsgType, uint32 MsgSize,
        SBN_InterfaceData *HostList[], int NumHosts,
        SBN_SenderId_t *SenderPtr, SBN_InterfaceData *IfData,
        NetDataUnion *MsgBuf)
{
    static struct sockaddr_in   s_addr;
    IPv4_SBNPeerData_t          *peer = NULL;
    IPv4_SBNHostData_t          *host = NULL;
    uint32                      HostIdx = 0;


    /* Find the host that goes with this peer.  There should only be one
       ethernet host */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++)
    {
        if(HostList[HostIdx]->ProtocolId == SBN_IPv4)
        {
            host = (IPv4_SBNHostData_t *)HostList[HostIdx]->InterfacePvt;
        }/* end if */
    }/* end for */

    if(!host)
    {
        OS_printf("No IPv4 Host Found!\n");
        return SBN_ERROR;
    }/* end if */

    peer = (IPv4_SBNPeerData_t *)IfData->InterfacePvt;
    bzero((char *) &s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(peer->Addr);

    SBN_Hdr_t orig_hdr;

    s_addr.sin_port = htons(peer->Port);

    /* Initialize the SBN hdr of the outgoing network message */
    strncpy((char *)&MsgBuf->Hdr.SrcCpuName, CFE_CPU_NAME,
        SBN_MAX_PEERNAME_LENGTH);

    MsgBuf->Hdr.Type = MsgType;

    memcpy(&orig_hdr, &(MsgBuf->Hdr), sizeof(orig_hdr));
    MsgBuf->Hdr.MsgSize = htonl(orig_hdr.MsgSize);
    MsgBuf->Hdr.Type = htonl(orig_hdr.Type);
    MsgBuf->Hdr.SequenceCount = htons(orig_hdr.SequenceCount);
    MsgBuf->Hdr.MsgSender.ProcessorId = ntohl(orig_hdr.MsgSender.ProcessorId);
    MsgBuf->Hdr.SequenceCount = ntohs(orig_hdr.SequenceCount);
    MsgBuf->Hdr.GapAfter = ntohs(orig_hdr.GapAfter);
    MsgBuf->Hdr.GapTo = ntohs(orig_hdr.GapTo);

    sendto(host->SockId, (char *)MsgBuf, MsgSize, 0,
        (struct sockaddr *) &s_addr, sizeof(s_addr));

    memcpy(&(MsgBuf->Hdr), &orig_hdr, sizeof(orig_hdr));

    return SBN_OK;
}/* end SBN_SendNetMsg */

/**
 * Verifies that there is, in fact, and ethernet host.
 *
 * @param Peer      The peer to verify
 * @param HostList  The list of hosts in the SBN
 * @param NumHosts  The number of hosts in the SBN
 * @return SBN_VALID if there is an ethernet host, SBN_NOT_VALID otherwise
 */
int IPv4_VerifyPeerInterface(SBN_InterfaceData *Peer,
        SBN_InterfaceData *HostList[], int NumHosts)
{
    int     HostIdx = 0;

    /* Find the host that goes with this peer.  There should only be one
       ethernet host */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++)
    {
        if(HostList[HostIdx]->ProtocolId == SBN_IPv4)
        {
            return SBN_VALID;
        }/* end if */
    }/* end for */

    return SBN_NOT_VALID;
}/* end IPv4_VerifyPeerInterface */

/**
 * An IPv4 host doesn't necessarily need a peer, so this always returns true.
 *
 * @param Host      The host to verify
 * @param PeerList  The list of peers in the SBN
 * @param NumPeers  The number of peers in the SBN
 * @return SBN_VALID
 */
int IPv4_VerifyHostInterface(SBN_InterfaceData *Host,
        SBN_PeerData_t *PeerList, int NumPeers)
{
    return SBN_VALID;
}/* end IPv4_VerifyHostInterface */

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
int IPv4_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet,
        SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int NumHosts)
{
    return SBN_NOT_IMPLEMENTED;
}/* end IPv4_ReportModuleStatus */

/**
 * Resets the specified ethernet peer.
 * Not yet implemented.
 *
 * @param Peer      The peer to reset
 * @param HostList  The list of hosts in the SBN
 * @param NumHosts  The number of hosts in the SBN
 * @return SBN_NOT_IMPLEMENTED
 */
int IPv4_ResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
        int NumHosts)
{
    return SBN_NOT_IMPLEMENTED;
}/* end IPv4_ResetPeer */
