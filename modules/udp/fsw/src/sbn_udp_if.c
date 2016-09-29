#include "sbn_udp_if_struct.h"
#include "sbn_udp_if.h"
#include "sbn_udp_events.h"
#include "cfe.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>

#ifdef OS_NET_IMPL
static void ClearSocket(int32 NetID)
#else /* !OS_NET_IMPL */
static void ClearSocket(int SockId)
#endif /* OS_NET_IMPL */
{
    struct sockaddr_in  s_addr;
    socklen_t           addr_len = 0;
    int                 i = 0;
    int                 status = 0;
    char                DiscardData[SBN_MAX_MSG_SIZE];

    addr_len = sizeof(s_addr);
    CFE_PSP_MemSet(&s_addr, 0, sizeof(s_addr));

#ifdef OS_NET_IMPL

    CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_DEBUG,
        "clearing socket (NetID=%d)", NetID);

#else /* !OS_NET_IMPL */

    CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_DEBUG,
        "clearing socket (SockId=%d)", SockId);

#endif /* OS_NET_IMPL */

    /* change to while loop */
    for(i = 0; i <= 50; i++)
    {
#ifdef OS_NET_IMPL

        size_t Size = sizeof(DiscardData);
        status = OS_NetRecv(NetID, DiscardData, &Size);
        if(status < 0)
        {
            break;
        }/* end if */

#else /* !OS_NET_IMPL */

        status = recvfrom(SockId, DiscardData, sizeof(DiscardData),
            MSG_DONTWAIT,(struct sockaddr *) &s_addr, &addr_len);
        if((status < 0) && (errno == EWOULDBLOCK)) // TODO: add EAGAIN?
        {
            break; /* no (more) messages */
        }/* end if */

#endif /* OS_NET_IMPL */

    }/* end for */
}/* end ClearSocket */

#ifdef _osapi_confloader_

int SBN_UDP_LoadEntry(const char **Row, int FieldCount, void *EntryBuffer)
{
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)EntryBuffer;
    if(FieldCount < SBN_UDP_ITEMS_PER_FILE_LINE)
    {
        return SBN_ERROR;
    }/* end if */

    strncpy(Entry->Host, Row[0], sizeof(Entry->Host));
    Entry->Port = atoi(Row[1]);

    return SBN_SUCCESS;
}/* end SBN_UDP_LoadEntry */

#else /* ! _osapi_confloader_ */

int SBN_UDP_ParseFileEntry(char *FileEntry, uint32 LineNum, void *EntryPtr)
{
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)EntryPtr;

    char Host[16];
    int ScanfStatus = 0, Port = 0;

    /*
     * Using sscanf to parse the string.
     * Currently no error handling
     */
    ScanfStatus = sscanf(FileEntry, "%16s %d", Host, &Port);

    /*
     * Check to see if the correct number of items were parsed
     */
    if(ScanfStatus != SBN_UDP_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_UDP_CONFIG_EID,CFE_EVS_ERROR,
                "invalid peer file line (expected %d items, found %d)",
                SBN_UDP_ITEMS_PER_FILE_LINE, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    strncpy(Entry->Host, Host, sizeof(Entry->Host));
    Entry->Port = Port;

    return SBN_SUCCESS;
}/* end SBN_UDP_ParseFileEntry */

#endif /* _osapi_confloader_ */

/**
 * Initializes an UDP host.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
int SBN_UDP_InitHost(SBN_HostInterface_t *HostInterface)
{
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)HostInterface->InterfacePvt;
    SBN_UDP_Network_t *Network
        = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];

    /**
     * Create msg interface when we find entry matching its own name
     * because the self entry has port info needed to bind this interface.
     */

    Network->Host.EntryPtr = Entry;

    CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_DEBUG,
        "creating socket (%s:%d)", Entry->Host, Entry->Port);

#ifdef OS_NET_IMPL

    if((Network->Host.NetID
        = OS_NetOpen(OS_NET_DOMAIN_INET4, OS_NET_TYPE_DATAGRAM)) < 0)
    {
        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
            "unable to open network (NetID=%d)", Network->Host.NetID);
        return SBN_ERROR;
    }/* end if */

    OS_NetSetNonBlocking(Network->Host.NetID, TRUE);
    
    OS_NetAddr_t Addr;
    OS_NetAddrInit(&Addr, OS_NET_DOMAIN_INET4);
    OS_NetAddrSetHost(&Addr, Entry->Host);
    OS_NetAddrSetPort(&Addr, Entry->Port);
    if(OS_NetBind(Network->Host.NetID, &Addr) < 0)
    {
        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
            "unable to bind network (%s:%d NetID=%d)",
            Entry->Host, Entry->Port, Network->Host.NetID);
        return SBN_ERROR;
    }/* end if */

    ClearSocket(Network->Host.NetID);

#else /* !OS_NET_IMPL */

    if((Network->Host.Socket
        = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
            "socket call failed (errno=%d)", errno);
        return SBN_ERROR;
    }/* end if */

    static struct sockaddr_in my_addr;

    my_addr.sin_addr.s_addr = inet_addr(Entry->Host);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(Entry->Port);

    if(bind(Network->Host.Socket, (struct sockaddr *) &my_addr,
        sizeof(my_addr)) < 0)
    {
        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
            "bind call failed (%s:%d Socket=%d errno=%d)",
            Entry->Host, Entry->Port, Network->Host.Socket, errno);
        return SBN_ERROR;
    }/* end if */

    #ifdef _HAVE_FCNTL_
        /*
        ** Set the socket to non-blocking
        ** This is not available to vxWorks, so it has to be
        ** Conditionally compiled in
        */
        fcntl(Network->Host.Socket, F_SETFL, O_NONBLOCK);
    #endif

    ClearSocket(Network->Host.Socket);

#endif /* OS_NET_IMPL */

    return SBN_SUCCESS;
}/* end SBN_UDP_InitHost */

/**
 * Initializes an UDP host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
int SBN_UDP_InitPeer(SBN_PeerInterface_t *PeerInterface)
{
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)PeerInterface->InterfacePvt;
    SBN_UDP_Network_t *Network
        = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];

    Entry->PeerNumber = Network->PeerCount++;
    Network->Peers[Entry->PeerNumber].EntryPtr = Entry;

    return SBN_SUCCESS;
}/* end SBN_UDP_InitPeer */

int SBN_UDP_Send(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t *Msg)
{
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)PeerInterface->InterfacePvt;
    SBN_UDP_Network_t *Network
        = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];

    SBN_PackMsg(&Network->SendBuf, MsgSize, MsgType, CFE_CPU_ID, Msg);

#ifdef OS_NET_IMPL

    OS_NetAddr_t Addr;
    OS_NetAddrInit(&Addr, OS_NET_DOMAIN_INET4);
    OS_NetAddrSetHost(&Addr, Entry->Host);
    OS_NetAddrSetPort(&Addr, Entry->Port);

    OS_NetSendTo(Network->Host.NetID, &Network->SendBuf,
        MsgSize + SBN_PACKED_HDR_SIZE, &Addr);

#else /* !OS_NET_IMPL */
    SBN_UDP_Peer_t *Peer = &Network->Peers[Entry->PeerNumber];

    static struct sockaddr_in s_addr;

    CFE_PSP_MemSet(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(Peer->EntryPtr->Host);
    s_addr.sin_port = htons(Peer->EntryPtr->Port);

    sendto(Network->Host.Socket, &Network->SendBuf,
        MsgSize + SBN_PACKED_HDR_SIZE, 0,
        (struct sockaddr *) &s_addr, sizeof(s_addr));

#endif /* OS_NET_IMPL */

    return SBN_SUCCESS;
}/* end SBN_UDP_Send */

/* Note that this Recv function is indescriminate, packets will be received
 * from all peers but that's ok, I just inject them into the SB and all is
 * good!
 */
int SBN_UDP_Recv(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *MsgBuf)
{
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)PeerInterface->InterfacePvt;
    SBN_UDP_Network_t *Network
        = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];

#ifdef OS_NET_IMPL

    size_t Received = SBN_MAX_MSG_SIZE;
    int32 Status = OS_NetRecv(Network->Host.NetID,
        (char *)&Network->RecvBuf, &Received);
    if(Status < 0)
    {
        return SBN_ERROR;
    }/* end if */

    if(Received == 0)
    {
        return SBN_IF_EMPTY;
    }/* end if */

#else /* !OS_NET_IMPL */

    int Received = recv(Network->Host.Socket, (char *)&Network->RecvBuf,
        SBN_MAX_MSG_SIZE, MSG_DONTWAIT);

    if(Received == 0)
    {
        return SBN_IF_EMPTY;
    }/* end if */

    if(Received < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            /* the socket is set to O_NONBLOCK, so these are valid return
             *  values when there are no packets to recv.
             */
            return SBN_IF_EMPTY;
        }/* end if */

        return SBN_ERROR;
    }/* end if */

    /* each UDP packet is a full SBN message */

#endif /* OS_NET_IMPL */

    SBN_UnpackMsg(&Network->RecvBuf, MsgSizePtr, MsgTypePtr, CpuIdPtr, MsgBuf);

    return SBN_SUCCESS;
}/* end SBN_UDP_Recv */

int SBN_UDP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_UDP_ReportModuleStatus */

int SBN_UDP_ResetPeer(SBN_PeerInterface_t *Peer)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_UDP_ResetPeer */
