#include "sbn_tcp_if_struct.h"
#include "sbn_tcp_if.h"
#include "sbn_tcp_events.h"
#include "cfe.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

#ifdef _osapi_confloader_

int SBN_TCP_LoadEntry(const char **row, int fieldcount, void *entryptr)
{
    SBN_TCP_Entry_t *entry = (SBN_TCP_Entry_t *)entryptr;

    if(fieldcount < SBN_TCP_ITEMS_PER_FILE_LINE)
    {
        return SBN_ERROR;
    }/* end if */

    entry->NetworkNumber = atoi(row[0]);
    if(entry->NetworkNumber < 0
        || entry->NetworkNumber >= SBN_MAX_NETWORK_PEERS)
    {
        return SBN_ERROR;
    }/* end if */

    entry->Addr = inet_addr(row[1]);
    entry->Port = htons(atoi(row[2]));

    return SBN_OK;
}/* end SBN_TCP_LoadEntry */

#else /* ! _osapi_confloader_ */

int SBN_TCP_ParseFileEntry(char *FileEntry, uint32 LineNum, void *entryptr)
{
    SBN_TCP_Entry_t *entry = (SBN_TCP_Entry_t *)entryptr;

    int ScanfStatus = 0;
    char Addr[16];
    int Port = 0, NetworkNumber = 0;

    /*
     * Using sscanf to parse the string.
     * Currently no error handling
     */
    ScanfStatus = sscanf(FileEntry, "%d %s %d", &NetworkNumber, Addr, &Port);

    /*
     * Check to see if the correct number of items were parsed
     */
    if(ScanfStatus != SBN_TCP_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID,CFE_EVS_ERROR,
                "%s:Invalid SBN peer file line,exp %d items,found %d",
                CFE_CPU_NAME, SBN_TCP_ITEMS_PER_FILE_LINE, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    if(NetworkNumber < 0 || NetworkNumber >= SBN_MAX_NETWORK_PEERS)
    {
        return SBN_ERROR;
    }/* end if */

    entry->NetworkNumber = NetworkNumber;
    entry->Addr = inet_addr(Addr);
    entry->Port = htons(Port);

    return SBN_OK;
}/* end SBN_TCP_ParseFileEntry */

#endif /* _osapi_confloader_ */

/**
 * Initializes an TCP host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_OK on success, error code otherwise
 */
int SBN_TCP_Init(SBN_InterfaceData *Data)
{
    SBN_TCP_Entry_t *Entry
        = (SBN_TCP_Entry_t *)Data->InterfacePvt;
    SBN_TCP_Network_t *Network = NULL;

    if(!SBN_TCP_ModuleDataInitialized)
    {
        CFE_PSP_MemSet(&SBN_TCP_ModuleData, 0, sizeof(SBN_TCP_ModuleData));
        SBN_TCP_ModuleDataInitialized = 1;
    }/* end if */

    Network = &SBN_TCP_ModuleData.Networks[Entry->NetworkNumber];

    if(Data->ProcessorId == CFE_CPU_ID)
    {
        Network->Host.EntryPtr = Entry;
        /* CPU id match - this is host data.
         * Create msg interface when we find entry matching its own name
         * because the self entry has port info needed to bind this interface.
         */
        /* create, fill, and store an TCP-specific host data structure */
        struct sockaddr_in my_addr;
        int OptVal = 0;
        Network->Host.Socket = 0;

        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_DEBUG,
            "Creating socket for 0x%04X:%d",
            Entry->Addr,
            ntohs(Entry->Port));

        if((Network->Host.Socket
            = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
                "%s:socket call failed,line %d,rtn val %d,errno=%d",
                CFE_CPU_NAME, __LINE__, Network->Host.Socket, errno);
            return SBN_ERROR;
        }/* end if */

        OptVal = 1;
        setsockopt(Network->Host.Socket, SOL_SOCKET, SO_REUSEADDR,
            (const void *)&OptVal, sizeof(int));

        my_addr.sin_addr.s_addr = Entry->Addr;
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = Entry->Port;

        if(bind(Network->Host.Socket, (struct sockaddr *) &my_addr,
            sizeof(my_addr)) < 0)
        {
            CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
                "%s:bind call failed,line %d,rtn val %d,errno=%d",
                CFE_CPU_NAME, __LINE__, Network->Host.Socket, errno);
            return SBN_ERROR;
        }/* end if */

        if(listen(Network->Host.Socket, SBN_MAX_NETWORK_PEERS) < 0)
        {
            CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
                "%s:listen call failed,line %d,rtn val %d,errno=%d",
                CFE_CPU_NAME, __LINE__, Network->Host.Socket, errno);
            return SBN_ERROR;
        }/* end if */

        return SBN_HOST;
    }

    /* not me, it's a peer */
    Entry->PeerNumber = Network->PeerCount++;
    Network->Peers[Entry->PeerNumber].EntryPtr = Entry;

    Network->Peers[Entry->PeerNumber].ConnectOut = (Data->ProcessorId > CFE_CPU_ID);

    return SBN_PEER;
}/* end SBN_TCP_Init */

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
                == Network->Peers[PeerNumber].EntryPtr->Addr)
            {
                Network->Peers[PeerNumber].Socket = ClientFd;
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

    if(Peer->Socket)
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

                if((Socket = socket(AF_INET, SOCK_STREAM, 0)) > 0)
                {
                    struct sockaddr_in ServerAddr;
                    CFE_PSP_MemSet(&ServerAddr, 0, sizeof(ServerAddr));
                    ServerAddr.sin_family = AF_INET;
                    ServerAddr.sin_addr.s_addr = PeerEntry->Addr;
                    ServerAddr.sin_port = PeerEntry->Port;

                    if((connect(Socket, (struct sockaddr *)&ServerAddr,
                        sizeof(ServerAddr)))
                            >= 0)
                {
                        Peer->Socket = Socket;
                    }
                    else
                    {
                        Peer->LastConnectTry.seconds = LocalTime.seconds;
                    }/* end if */
                }
            }/* end if */
        }/* end if */
        return -1;
    }/* end if */
}/* end GetPeerSocket */

int SBN_TCP_Send(SBN_InterfaceData *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, void *Msg)
{
    int PeerSocket = 0;
    SBN_TCP_Entry_t *PeerEntry = (SBN_TCP_Entry_t *)PeerInterface->InterfacePvt;
    SBN_TCP_Network_t *Network
        = &SBN_TCP_ModuleData.Networks[PeerEntry->NetworkNumber];

    if ((PeerSocket = GetPeerSocket(Network, PeerEntry)) < 0)
    {
        /* fail silently as the peer is not connected (yet) */
        return SBN_OK;
    }/* end if */

    /* send the SBN header as individual fields so that there's no alignment */
    SBN_MsgSize_t MsgSizeEndian = htons(MsgSize);
    send(PeerSocket, &MsgSizeEndian, sizeof(MsgSizeEndian), 0);
    send(PeerSocket, &MsgType, sizeof(MsgType), 0);
    SBN_CpuId_t CpuIdEndian = htons(CFE_CPU_ID);
    send(PeerSocket, &CpuIdEndian, sizeof(CpuIdEndian), 0);

    if(Msg && MsgSize)
    {
        send(PeerSocket, Msg, MsgSize, 0);
    }/* end if */

    return SBN_OK;
}/* end SBN_TCP_Send */

int SBN_TCP_Recv(SBN_InterfaceData *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, void *MsgBuf)
{
    SBN_TCP_Entry_t *PeerEntry = (SBN_TCP_Entry_t *)PeerInterface->InterfacePvt;
    SBN_TCP_Network_t *Network
        = &SBN_TCP_ModuleData.Networks[PeerEntry->NetworkNumber];
    SBN_TCP_Peer_t *Peer = &Network->Peers[PeerEntry->PeerNumber];
    int PeerSocket = GetPeerSocket(Network, PeerEntry);

    if (PeerSocket < 0)
    {
        /* fail silently as the peer is not connected (yet) */
        return SBN_IF_EMPTY;
    }/* end if */

    while(1)
    {
        ssize_t Received = 0;
        fd_set ReadFDs;
        struct timeval timeout;
        SBN_MsgSize_t MsgSize = 0;

        CFE_PSP_MemSet(&timeout, 0, sizeof(timeout));
        timeout.tv_usec = 100;

        FD_ZERO(&ReadFDs);
        FD_SET(PeerSocket, &ReadFDs);

        if(select(PeerSocket + 1, &ReadFDs, 0, 0, &timeout) < 0)
        {
            return SBN_ERROR;
        }/* end if */
        if (FD_ISSET(PeerSocket, &ReadFDs))
        {
            Received = recv(PeerSocket, Peer->RecvBuf + Peer->RecvSize,
                SBN_MAX_MSG_SIZE - Peer->RecvSize, 0);
            if(Received < 0)
            {
                return SBN_ERROR;
            }/* end if */

            Peer->RecvSize += Received;
        }/* end if */

        if (Peer->RecvSize >= SBN_PACKED_HDR_SIZE)
        {
            MsgSize = ntohs(*((SBN_MsgSize_t *)Peer->RecvBuf));

            if(Peer->RecvSize < MsgSize + SBN_PACKED_HDR_SIZE)
            {
                if(Received == 0)
                {
                    /* nothing received this time through and the buffer
                     * does not have enough to make a message...we're done
                     * for now.
                     */
                    return SBN_IF_EMPTY;
                }/* end if */

                /* we received bytes, continue to accumulate the message */
                continue;
            }/* end if */

            /* Ok we have the message! read off the SBN header */
            char *BufPtr = (char *)Peer->RecvBuf;

            *MsgSizePtr = MsgSize;
            BufPtr += sizeof(MsgSize);

            *MsgTypePtr = *((SBN_MsgType_t *)BufPtr);
            BufPtr += sizeof(*MsgTypePtr);

            *CpuIdPtr = ntohs(*((SBN_CpuId_t *)BufPtr));
            BufPtr += sizeof(*CpuIdPtr);

            /* and read the body */
            CFE_PSP_MemCpy(MsgBuf, BufPtr, MsgSize);
            BufPtr += MsgSize;

            /* shift back the received bytes not part of this message,
             * we may have part of the next message
             */
            int Overage = Peer->RecvSize - MsgSize - SBN_PACKED_HDR_SIZE;
            Peer->RecvSize = Overage;

            /* move the overage to the start of the buffer */
            if(Overage < MsgSize + SBN_PACKED_HDR_SIZE)
            {
                CFE_PSP_MemCpy(Peer->RecvBuf, BufPtr, Overage);
            }
            else
            {
                char *Start = Peer->RecvBuf;

                /* the overage is larger than the message, so the memcpy
                 * would overlap, which may be undefined */
                while(Overage)
                {
                    *Start++ = *BufPtr++;
                    Overage--;
                }/* end while */
            }/* end if */

            return SBN_OK;
        }/* end if */
        
        if(Received == 0)
        {
            return SBN_IF_EMPTY;
        }/* end if */
    }/* end while */
    /* will never get here */
}/* end SBN_TCP_Recv */

int SBN_TCP_VerifyPeerInterface(SBN_InterfaceData *Peer,
        SBN_InterfaceData *HostList[], int NumHosts)
{
    return TRUE;
}/* end SBN_TCP_VerifyPeerInterface */

int SBN_TCP_VerifyHostInterface(SBN_InterfaceData *Host,
        SBN_PeerData_t *PeerList, int NumPeers)
{
    return TRUE;
}/* end SBN_TCP_VerifyHostInterface */

int SBN_TCP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet,
        SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int NumHosts)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_TCP_ReportModuleStatus */

int SBN_TCP_ResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
        int NumHosts)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_TCP_ResetPeer */
