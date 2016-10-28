#include "sbn_tcp_if_struct.h"
#include "sbn_tcp_if.h"
#include "sbn_tcp_events.h"
#include "cfe.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

#ifdef CFE_ES_CONFLOADER

int SBN_TCP_LoadEntry(const char **Row, int FieldCount, void *EntryBuffer)
{
    SBN_TCP_Entry_t *Entry = (SBN_TCP_Entry_t *)EntryBuffer;
    char *ValidatePtr = NULL;

    if(FieldCount < SBN_TCP_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_ERROR,
                "invalid peer file line (expected %d items, found %d)",
                SBN_TCP_ITEMS_PER_FILE_LINE, FieldCount);
        return SBN_ERROR;
    }/* end if */

    strncpy(Entry->Host, Row[0], sizeof(Entry->Host));
    Entry->Port = strtol(Row[1], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[1])
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_ERROR,
                "invalid port");
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_TCP_LoadEntry */

#else /* ! CFE_ES_CONFLOADER */

#include <ctype.h> /* isspace() */

int SBN_TCP_ParseFileEntry(char *FileEntry, uint32 LineNum, void *EntryPtr)
{
    SBN_TCP_Entry_t *Entry = (SBN_TCP_Entry_t *)EntryPtr;

    char *EndPtr = Entry->Host;

    while(isspace(*FileEntry))
    {
        FileEntry++;
    }/* end while */

    while(EndPtr < Entry->Host + 16 && *FileEntry && !isspace(*FileEntry))
    {
        *EndPtr++ = *FileEntry++;
    }/* end while */

    if(EndPtr == Entry->Host + 16)
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_ERROR,
            "invalid host");
        return SBN_ERROR;
    }/* end if */

    *EndPtr = '\0';

    while(isspace(*FileEntry))
    {
        FileEntry++;
    }/* end while */

    EndPtr = NULL;
    Entry->Port = strtol(FileEntry, &EndPtr, 0);
    if(!EndPtr || EndPtr == FileEntry)
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_ERROR,
            "invalid port");
        return SBN_ERROR;
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_TCP_ParseFileEntry */

#endif /* CFE_ES_CONFLOADER */
/**
 * Initializes an TCP host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
int SBN_TCP_InitHost(SBN_HostInterface_t *HostPtr)
{
    SBN_TCP_Entry_t *Entry
        = (SBN_TCP_Entry_t *)HostPtr->ModulePvt;
    SBN_TCP_Network_t *Network = NULL;

    if(!SBN_TCP_ModuleDataInitialized)
    {
        CFE_PSP_MemSet(&SBN_TCP_ModuleData, 0, sizeof(SBN_TCP_ModuleData));
        SBN_TCP_ModuleDataInitialized = 1;
    }/* end if */

    Network = &SBN_TCP_ModuleData.Networks[Entry->NetworkNumber];

    Network->Host.EntryPtr = Entry;

#ifdef OS_NET_IMPL

    int32 NetID = 0;
    OS_NetAddr_t Addr;
    OS_NetAddrInit(&Addr, OS_NET_DOMAIN_INET4);
    OS_NetAddrSetHost(&Addr, Entry->Host);
    OS_NetAddrSetPort(&Addr, Entry->Port);

    Network->Host.NetID = 0;

    CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_DEBUG,
        "creating network connection for %s:%d", Entry->Host, Entry->Port);

    if((NetID = OS_NetOpen(
        OS_NET_DOMAIN_INET4, OS_NET_TYPE_STREAM)) < 0)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "unable to open socket (errno=%d)", errno);
        return SBN_ERROR;
    }/* end if*/

    if(OS_NetBind(NetID, &Addr) != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "unable to bind socket (%s:%d errno=%d)",
            Entry->Host, Entry->Port, errno);
        return SBN_ERROR;
    }/* end if */

    if(OS_NetListen(Network->Host.NetID, SBN_MAX_NETWORK_PEERS))
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "unable to set listen queue (errno=%d)", errno);
        return SBN_ERROR;
    }/* end if */

    Network->Host.NetID = NetID;

#else /* !OS_NET_IMPL */

    struct sockaddr_in my_addr;
    int OptVal = 0;

    Network->Host.Socket = 0;
    int Socket = 0;

    CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_DEBUG,
        "creating socket for %s:%d", Entry->Host, Entry->Port);

    if((Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "unable to create socket (errno=%d)", errno);
        return SBN_ERROR;
    }/* end if */

    OptVal = 1;
    setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR,
        (const void *)&OptVal, sizeof(int));

    my_addr.sin_addr.s_addr = inet_addr(Entry->Host);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(Entry->Port);

    if(bind(Socket, (struct sockaddr *) &my_addr,
        sizeof(my_addr)) < 0)
    {
        close(Socket);
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "bind call failed (%s:%d Socket=%d errno=%d)",
            Entry->Host, Entry->Port, Socket, errno);
        return SBN_ERROR;
    }/* end if */

    if(listen(Socket, SBN_MAX_NETWORK_PEERS) < 0)
    {
        close(Socket);
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "listen call failed (%s:%d Socket=%d errno=%d)",
            Entry->Host, Entry->Port, Socket, errno);
        return SBN_ERROR;
    }/* end if */

    Network->Host.Socket = Socket;
#endif /* OS_NET_IMPL */

    return SBN_SUCCESS;
}/* end SBN_TCP_Init */

/**
 * Initializes an TCP peer.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
int SBN_TCP_InitPeer(SBN_PeerInterface_t *Peer)
{
    SBN_TCP_Entry_t *Entry
        = (SBN_TCP_Entry_t *)Peer->ModulePvt;
    SBN_TCP_Network_t *Network = NULL;

    if(!SBN_TCP_ModuleDataInitialized)
    {
        CFE_PSP_MemSet(&SBN_TCP_ModuleData, 0, sizeof(SBN_TCP_ModuleData));
        SBN_TCP_ModuleDataInitialized = 1;
    }/* end if */

    Network = &SBN_TCP_ModuleData.Networks[Entry->NetworkNumber];

    Entry->PeerNumber = Network->PeerCount++;
    Network->Peers[Entry->PeerNumber].EntryPtr = Entry;

    Network->Peers[Entry->PeerNumber].ConnectOut =
        (Peer->Status->ProcessorId > CFE_PSP_GetProcessorId());

    return SBN_PEER;
}/* end SBN_TCP_Init */

#ifdef OS_NET_IMPL

static void CheckServer(SBN_TCP_Network_t *Network)
{
    if(OS_NetDataReady(Network->Host.NetID) != OS_SUCCESS)
    {
        return;
    }/* end if */

    int ClientNetID = 0;

    OS_NetAddr_t ClientAddr;
    OS_NetAddrInit(&ClientAddr, OS_NET_DOMAIN_INET4);

    if(OS_NetAccept(Network->Host.NetID, &ClientNetID, &ClientAddr, 0)
        != OS_SUCCESS)
    {
        return;
    }/* end if */

    int PeerNumber = 0;
    for(PeerNumber = 0; PeerNumber < Network->PeerCount; PeerNumber++)
    {
        if(((struct sockaddr_in *)&ClientAddr.Addr)->sin_addr.s_addr
            == inet_addr(Network->Peers[PeerNumber].EntryPtr->Host))
        {
            Network->Peers[PeerNumber].NetID = ClientNetID;
            Network->Peers[PeerNumber].Connected = TRUE;
            return;
        }/* end if */
    }/* end for */
        
    /* invalid peer */
    OS_NetClose(ClientNetID);
}/* end CheckServer */

static int GetPeerNetID(SBN_TCP_Network_t *Network, SBN_TCP_Entry_t *PeerEntry)
{
    SBN_TCP_Peer_t *Peer = &Network->Peers[PeerEntry->PeerNumber];
    CheckServer(Network);

    if(Peer->Connected)
    {
        return Peer->NetID;
    }
    else
    {
        if(Peer->ConnectOut)
        {
            OS_time_t LocalTime;
            OS_GetLocalTime(&LocalTime);
            /* TODO: make a #define */
            if(LocalTime.seconds > Peer->LastConnectTry.seconds + 5)
            {
                int NetID = 0;

                if((NetID = OS_NetOpen(
                    OS_NET_DOMAIN_INET4, OS_NET_TYPE_STREAM)) > 0)
                {
                    OS_NetAddr_t ClientAddr;
                    OS_NetAddrInit(&ClientAddr, OS_NET_DOMAIN_INET4);
                    OS_NetAddrSetHost(&ClientAddr, PeerEntry->Host);
                    OS_NetAddrSetPort(&ClientAddr, PeerEntry->Port);

                    if (OS_NetConnect(NetID, &ClientAddr, 0) == OS_SUCCESS)
                    {
                        Peer->NetID = NetID;
                        Peer->Connected = TRUE;
                        return NetID;
                    }
                    else
                    {
                        OS_NetClose(NetID);
                        Peer->LastConnectTry.seconds = LocalTime.seconds;
                    }/* end if */
                }/* end if */
            }/* end if */
        }/* end if */
        return -1;
    }/* end if */
}/* end GetPeerNetID */

int SBN_TCP_Send(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t *Msg)
{
    SBN_TCP_Entry_t *PeerEntry = (SBN_TCP_Entry_t *)PeerInterface->ModulePvt;
    SBN_TCP_Network_t *Network
        = &SBN_TCP_ModuleData.Networks[PeerEntry->NetworkNumber];
    SBN_TCP_Peer_t *Peer = &Network->Peers[PeerEntry->PeerNumber];
    int PeerNetID = GetPeerNetID(Network, PeerEntry);

    if (PeerNetID < 0)
    {
        /* fail silently as the peer is not connected (yet) */
        return SBN_SUCCESS;
    }/* end if */

    SBN_PackMsg(&Network->SendBuf, MsgSize, MsgType, CFE_CPU_ID, Msg);
    if(OS_NetSend(PeerNetID, &Network->SendBuf, MsgSize + SBN_PACKED_HDR_SIZE)
        != OS_SUCCESS)
    {
        Peer->Connected = FALSE;
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "send failed, peer #%d reset", PeerEntry->PeerNumber);
        return SBN_ERROR;
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_TCP_Send */

int SBN_TCP_Recv(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *MsgBuf)
{
    SBN_TCP_Entry_t *PeerEntry = (SBN_TCP_Entry_t *)PeerInterface->ModulePvt;
    SBN_TCP_Network_t *Network
        = &SBN_TCP_ModuleData.Networks[PeerEntry->NetworkNumber];
    SBN_TCP_Peer_t *Peer = &Network->Peers[PeerEntry->PeerNumber];
    int PeerNetID = GetPeerNetID(Network, PeerEntry);
    int32 Status = 0;

    if(PeerNetID < 0 || OS_NetDataReady(PeerNetID) != OS_SUCCESS)
    {
        /* fail silently as the peer is not connected (yet) */
        return SBN_IF_EMPTY;
    }/* end if */
    
    size_t Len = 0, ToRead = 0;

    if(!Peer->ReceivingBody)
    {
        /* recv the header first */
        ToRead = Len = SBN_PACKED_HDR_SIZE - Peer->RecvSize;

        Status = OS_NetRecv(PeerNetID,
            (char *)&Peer->RecvBuf + Peer->RecvSize, &Len);

        if(Status != OS_SUCCESS)
        {
            return SBN_ERROR;
        }/* end if */

        Peer->RecvSize += Len;

        if(Len >= ToRead)
        {
            Peer->ReceivingBody = 1; /* and continue on to recv body */
        }
        else
        {
            return SBN_IF_EMPTY; /* wait for the complete header */
        }/* end if */
    }/* end if */

    /* only get here if we're recv'd the header and ready for the body */

    Len = ToRead =
        CFE_MAKE_BIG16(*((SBN_MsgSize_t *)Peer->RecvBuf.Hdr.MsgSizeBuf))
        + SBN_PACKED_HDR_SIZE - Peer->RecvSize;
    if(ToRead)
    {
        Status = OS_NetRecv(PeerNetID,
            (char *)&Peer->RecvBuf + Peer->RecvSize, &Len);

        if(Status != OS_SUCCESS)
        {
            return SBN_ERROR;
        }/* end if */

        Peer->RecvSize += Len;

        if(Len < ToRead)
        {
            return SBN_IF_EMPTY; /* wait for the complete body */
        }/* end if */
    }/* end if */

    /* we have the complete body, decode! */
    SBN_UnpackMsg(&Peer->RecvBuf, MsgSizePtr, MsgTypePtr, CpuIdPtr,
        MsgBuf);

    Peer->ReceivingBody = 0;
    Peer->RecvSize = 0;

    return SBN_SUCCESS;
}/* end SBN_TCP_Recv */

#else /* !OS_NET_IMPL */

static void CheckServer(SBN_TCP_Network_t *Network)
{
    fd_set ReadFDs;
    struct timeval timeout;
    struct sockaddr_in ClientAddr;
    socklen_t ClientLen = sizeof(ClientAddr);

    CFE_PSP_MemSet(&timeout, 0, sizeof(timeout));
    timeout.tv_usec = 100;

    FD_ZERO(&ReadFDs);
    FD_SET(Network->Host.Socket, &ReadFDs);

    if(select(Network->Host.Socket + 1, &ReadFDs, 0, 0, &timeout) < 0)
    {
        return;
    }/* end if */

    if(FD_ISSET(Network->Host.Socket, &ReadFDs))
    {
        int PeerNumber = 0, ClientFd = 0;
        if ((ClientFd
            = accept(Network->Host.Socket,
                (struct sockaddr *)&ClientAddr, &ClientLen)) < 0)
        {
            return;
        }/* end if */
        
        for(PeerNumber = 0; PeerNumber < Network->PeerCount; PeerNumber++)
        {
            if(ClientAddr.sin_addr.s_addr
                == inet_addr(Network->Peers[PeerNumber].EntryPtr->Host))
            {
                Network->Peers[PeerNumber].Socket = ClientFd;
                Network->Peers[PeerNumber].Connected = TRUE;
                return;
            }/* end if */
        }/* end for */
        
        /* invalid peer */
        close(ClientFd);
    }/* end if */
}/* end CheckServer */

static int GetPeerSocket(SBN_TCP_Network_t *Network, SBN_TCP_Entry_t *PeerEntry)
{
    SBN_TCP_Peer_t *Peer = &Network->Peers[PeerEntry->PeerNumber];
    CheckServer(Network);

    if(Peer->Connected)
    {
        return Peer->Socket;
    }
    else
    {
        if(Peer->ConnectOut)
        {
            OS_time_t LocalTime;
            OS_GetLocalTime(&LocalTime);
            /* TODO: make a #define */
            if(LocalTime.seconds > Peer->LastConnectTry.seconds + 5)
            {
                int Socket = 0;

                if((Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
                        "unable to create socket (errno=%d)", errno);
                    return -1;
                }/* end if */

                struct sockaddr_in ServerAddr;
                CFE_PSP_MemSet(&ServerAddr, 0, sizeof(ServerAddr));
                ServerAddr.sin_family = AF_INET;
                ServerAddr.sin_addr.s_addr = inet_addr(PeerEntry->Host);
                ServerAddr.sin_port = htons(PeerEntry->Port);

                if((connect(Socket, (struct sockaddr *)&ServerAddr,
                    sizeof(ServerAddr)))
                        >= 0)
                {
                    Peer->Socket = Socket;
                    Peer->Connected = TRUE;
                    return Socket;
                }
                else
                {
                    close(Socket);
                    Peer->LastConnectTry.seconds = LocalTime.seconds;
                }/* end if */
            }/* end if */
        }/* end if */
        return -1;
    }/* end if */
}/* end GetPeerSocket */

int SBN_TCP_Send(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t *Msg)
{
    SBN_TCP_Entry_t *PeerEntry = (SBN_TCP_Entry_t *)PeerInterface->ModulePvt;
    SBN_TCP_Network_t *Network
        = &SBN_TCP_ModuleData.Networks[PeerEntry->NetworkNumber];
    int Socket = GetPeerSocket(Network, PeerEntry);

    if (Socket < 0)
    {
        /* fail silently as the peer is not connected (yet) */
        return SBN_SUCCESS;
    }/* end if */

    SBN_PackMsg(&Network->SendBuf, MsgSize, MsgType, CFE_CPU_ID, Msg);
    send(Socket, &Network->SendBuf, MsgSize + SBN_PACKED_HDR_SIZE, 0);

    return SBN_SUCCESS;
}/* end SBN_TCP_Send */

int SBN_TCP_Recv(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *MsgBuf)
{
    SBN_TCP_Entry_t *PeerEntry = (SBN_TCP_Entry_t *)PeerInterface->ModulePvt;
    SBN_TCP_Network_t *Network
        = &SBN_TCP_ModuleData.Networks[PeerEntry->NetworkNumber];
    SBN_TCP_Peer_t *Peer = &Network->Peers[PeerEntry->PeerNumber];
    int PeerSocket = GetPeerSocket(Network, PeerEntry);

    if (PeerSocket < 0)
    {
        /* fail silently as the peer is not connected (yet) */
        return SBN_IF_EMPTY;
    }/* end if */

    ssize_t Received = 0;

    int ToRead = 0;

    if(!Peer->ReceivingBody)
    {
        /* recv the header first */
        ToRead = SBN_PACKED_HDR_SIZE - Peer->RecvSize;

        Received = recv(PeerSocket,
            (char *)&Peer->RecvBuf + Peer->RecvSize, ToRead, 0);

        if(Received < 0)
        {
            return SBN_ERROR;
        }/* end if */

        Peer->RecvSize += Received;

        if(Received >= ToRead)
        {
            Peer->ReceivingBody = 1; /* and continue on to recv body */
        }
        else
        {
            return SBN_IF_EMPTY; /* wait for the complete header */
        }/* end if */
    }/* end if */

    /* only get here if we're recv'd the header and ready for the body */

    ToRead =
        CFE_MAKE_BIG16(*((SBN_MsgSize_t *)Peer->RecvBuf.Hdr.MsgSizeBuf))
        + SBN_PACKED_HDR_SIZE - Peer->RecvSize;
    if(ToRead)
    {
        Received = recv(PeerSocket,
            (char *)&Peer->RecvBuf + Peer->RecvSize, ToRead, 0);
        if(Received < 0)
        {
            return SBN_ERROR;
        }/* end if */

        Peer->RecvSize += Received;

        if(Received < ToRead)
        {
            return SBN_IF_EMPTY; /* wait for the complete body */
        }/* end if */
    }/* end if */

    /* we have the complete body, decode! */
    SBN_UnpackMsg(&Peer->RecvBuf, MsgSizePtr, MsgTypePtr, CpuIdPtr,
        MsgBuf);

    Peer->ReceivingBody = 0;
    Peer->RecvSize = 0;

    return SBN_SUCCESS;
}/* end SBN_TCP_Recv */

#endif /* OS_NET_IMPL */

int SBN_TCP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_TCP_ReportModuleStatus */

int SBN_TCP_ResetPeer(SBN_PeerInterface_t *Peer)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_TCP_ResetPeer */
