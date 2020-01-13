#include "sbn_tcp_if_struct.h"
#include "sbn_tcp_if.h"
#include "sbn_tcp_events.h"
#include "cfe.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>

int SBN_TCP_Init(int Major, int Minor, int Revision)
{
    if(Major != SBN_TCP_MAJOR || Minor != SBN_TCP_MINOR
        || Revision != SBN_TCP_REVISION)
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_ERROR,
                "mismatching version %d.%d.%d (SBN app reports %d.%d.%d)",
                SBN_TCP_MAJOR, SBN_TCP_MINOR, SBN_TCP_REVISION,
                Major, Minor, Revision);
        return SBN_ERROR;
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_TCP_Init() */

int32 SBN_TCP_LibInit()
{
    OS_printf("SBN_TCP Lib Initialized. Version %d.%d.%d",
        SBN_TCP_MAJOR, SBN_TCP_MINOR, SBN_TCP_REVISION);
    return CFE_SUCCESS;
}/* end SBN_TCP_LibInit() */

static int ConfAddr(OS_SockAddr_t *Addr, const char *Address)
{
    int32 Status = OS_SUCCESS;

    char AddrHost[OS_MAX_API_NAME];
    int AddrLen;
    char *Colon = strchr(Address, ':');

    if(!Colon || (AddrLen = Colon - Address) >= OS_MAX_API_NAME)
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_ERROR, "invalid net address");
        return SBN_ERROR;
    }/* end if */

    strncpy(AddrHost, Address, AddrLen);
    AddrHost[AddrLen] = '\0';
    char *ValidatePtr = NULL;

    uint32 Port = strtol(Colon + 1, &ValidatePtr, 0);

    if(!ValidatePtr || ValidatePtr == Colon + 1)
    {
        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_ERROR, "invalid port");
        return SBN_ERROR;
    }/* end if */

    if((Status = OS_SocketAddrInit(Addr, OS_SocketDomain_INET)) != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "socket addr init failed (Status=%d)", Status);
        return SBN_ERROR;
    }/* end if */

    if((Status = OS_SocketAddrFromString(Addr, AddrHost)) != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "setting address host failed (AddrHost=%s, Status=%d)", AddrHost, Status);
        return SBN_ERROR;
    }/* end if */
 
    if((Status = OS_SocketAddrSetPort(Addr, Port)) != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "setting address port failed (Port=%d, Status=%d)", Port, Status);
        return SBN_ERROR;
    }/* end if */

    return SBN_SUCCESS;
}/* end ConfAddr() */

static uint8 SendBufs[SBN_MAX_NETS][SBN_MAX_PACKED_MSG_SZ];
static int SendBufCnt = 0;

static SBN_TCP_Conn_t *NewConn(SBN_TCP_Net_t *NetData, int Socket)
{
    /* warning -- no protections against flooding */
    /* TODO: do I need a mutex? */

    int ConnID = 0;

    for (ConnID = 0; ConnID < SBN_MAX_PEER_CNT && NetData->Conns[ConnID].InUse; ConnID++);

    if(ConnID == SBN_MAX_PEER_CNT)
    {
        return NULL;
    }/* end if */

    SBN_TCP_Conn_t *Conn = &NetData->Conns[ConnID];

    memset(Conn, 0, sizeof(*Conn));

    Conn->Socket = Socket;

    Conn->InUse = TRUE;

    return Conn;
}/* end NewConn() */

static void Disconnected(SBN_PeerInterface_t *Peer)
{
    SBN_TCP_Peer_t *PeerData = (SBN_TCP_Peer_t *)Peer->ModulePvt;
    SBN_TCP_Conn_t *Conn = PeerData->Conn;

    if(Conn)
    {
        OS_close(Conn->Socket);

        Conn->InUse = FALSE;
        PeerData->Conn = NULL;
    }/* end if */

    SBN_Disconnected(Peer);
}/* end Disconnected() */

int SBN_TCP_LoadNet(SBN_NetInterface_t *Net, const char *Address)
{
    SBN_TCP_Net_t *NetData = (SBN_TCP_Net_t *)Net->ModulePvt;

    CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_INFORMATION,
        "configuring net 0x%lx -> %s", (unsigned long int)NetData, Address);

    int Status = ConfAddr(&NetData->Addr, Address);

    if (Status == SBN_SUCCESS)
    {
        NetData->BufNum = SendBufCnt++;

        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_INFORMATION,
            "net 0x%lx configured", (unsigned long int)NetData);
    }/* end if */

    return Status;
}/* end SBN_TCP_LoadNet */

static uint8 RecvBufs[SBN_MAX_PEER_CNT][SBN_MAX_PACKED_MSG_SZ];
static uint8 RecvBufCnt = 0;

int SBN_TCP_LoadPeer(SBN_PeerInterface_t *Peer, const char *Address)
{
    SBN_TCP_Peer_t *PeerData = (SBN_TCP_Peer_t *)Peer->ModulePvt;

    CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_INFORMATION,
        "configuring peer 0x%lx -> %s", (unsigned long int)PeerData, Address);

    int Status = ConfAddr(&PeerData->Addr, Address);

    if (Status == SBN_SUCCESS)
    {
        PeerData->BufNum = RecvBufCnt++;

        CFE_EVS_SendEvent(SBN_TCP_CONFIG_EID, CFE_EVS_INFORMATION,
            "peer 0x%lx configured", (unsigned long int)PeerData);
    }/* end if */

    return Status;
}/* end SBN_TCP_LoadPeer */

/**
 * Initializes an TCP host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
int SBN_TCP_InitNet(SBN_NetInterface_t *Net)
{
    int32 Status = OS_SUCCESS;

    SBN_TCP_Net_t *NetData = (SBN_TCP_Net_t *)Net->ModulePvt;

    uint32 Socket = 0;

    if((Status = OS_SocketOpen(&Socket, OS_SocketDomain_INET, OS_SocketType_STREAM)) != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "unable to create socket (Status=%d)", Status);
        return SBN_ERROR;
    }/* end if */

    if((Status = OS_SocketBind(Socket, &NetData->Addr)) != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
            "bind call failed (0x%lx Socket=%d status=%d)",
            (unsigned long int)NetData, NetData->Socket, Status);
        return SBN_ERROR;
    }

    NetData->Socket = Socket;

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
    SBN_TCP_Peer_t *PeerData
        = (SBN_TCP_Peer_t *)Peer->ModulePvt;

    PeerData->ConnectOut =
        (Peer->ProcessorID > CFE_PSP_GetProcessorId());

    return SBN_SUCCESS;
}/* end SBN_TCP_Init */

static void CheckNet(SBN_NetInterface_t *Net)
{
    int32 Status = OS_SUCCESS;

    SBN_TCP_Net_t *NetData = (SBN_TCP_Net_t *)Net->ModulePvt;
    int PeerIdx = 0;

    OS_time_t LocalTime;
    OS_GetLocalTime(&LocalTime);

    uint32 ClientFd = 0;
    OS_SockAddr_t Addr;
    while((Status = OS_SocketAccept(NetData->Socket, &ClientFd, &Addr, 0)) == OS_SUCCESS)
    {
        NewConn(NetData, ClientFd);
    }/* end while */

    if (Status != OS_ERROR_TIMEOUT)
    {
        CFE_EVS_SendEvent(SBN_TCP_DEBUG_EID, CFE_EVS_ERROR,
            "CPU accept error (status=%d)", Status);
    }/* end if */

    /**
     * For peers I connect out to, and which are not currently connected,
     * try connecting now.
     */
    for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];
        SBN_TCP_Peer_t *PeerData = (SBN_TCP_Peer_t *)Peer->ModulePvt;

        if(PeerData->ConnectOut && !Peer->Connected)
        {
            /* TODO: make a #define */
            if(LocalTime.seconds > PeerData->LastConnectTry.seconds + 5)
            {
                CFE_EVS_SendEvent(SBN_TCP_DEBUG_EID, CFE_EVS_INFORMATION,
                    "connecting to peer (PeerData=0x%lx, ProcessorID=%d)",
                    (unsigned long int)PeerData, Peer->ProcessorID);

                PeerData->LastConnectTry.seconds = LocalTime.seconds;

                uint32 Socket = 0;

                if((Status = OS_SocketOpen(&Socket, OS_SocketDomain_INET, OS_SocketType_STREAM)) != OS_SUCCESS)
                {
                    CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
                        "unable to create socket (Status=%d)",  Status);
                    return;
                }/* end if */

                /* TODO: Make timeout configurable */
                if((Status = OS_SocketConnect(Socket, &PeerData->Addr, 100)) != OS_SUCCESS)
                {
                    CFE_EVS_SendEvent(SBN_TCP_SOCK_EID, CFE_EVS_ERROR,
                        "unable to connect to peer (PeerData=0x%lx, Status=%d)",
                        (unsigned long int)PeerData, Status);

                    OS_close(Socket);

                    return;
                }/* end if */

                CFE_EVS_SendEvent(SBN_TCP_DEBUG_EID, CFE_EVS_INFORMATION, "CPU %d connected", Peer->ProcessorID);

                SBN_TCP_Conn_t *Conn = NewConn(NetData, Socket);
                if(Conn)
                {
                    Conn->PeerInterface = Peer;
                    PeerData->Conn = Conn;

                    SBN_Connected(Peer);
                }/* end if */
            }/* end if */
        }/* end if */
    }/* end for */
}/* end CheckNet */

int SBN_TCP_PollPeer(SBN_PeerInterface_t *Peer)
{
    CheckNet(Peer->Net);

    if(!Peer->Connected)
    {
        return 0;
    }/* end if */

    OS_time_t CurrentTime;
    OS_GetLocalTime(&CurrentTime);

    if(SBN_TCP_PEER_HEARTBEAT > 0 &&
        CurrentTime.seconds - Peer->LastSend.seconds
            > SBN_TCP_PEER_HEARTBEAT)
    {
        SBN_TCP_Send(Peer, SBN_TCP_HEARTBEAT_MSG, 0, NULL);
    }/* end if */

    if(SBN_TCP_PEER_TIMEOUT > 0 &&
        CurrentTime.seconds - Peer->LastRecv.seconds
            > SBN_TCP_PEER_TIMEOUT)
    {
        CFE_EVS_SendEvent(SBN_TCP_DEBUG_EID, CFE_EVS_INFORMATION,
            "CPU %d timeout, disconnected", Peer->ProcessorID);

        Disconnected(Peer);
    }/* end if */

    return 0;
}/* end SBN_TCP_PollPeer() */

int SBN_TCP_Send(SBN_PeerInterface_t *Peer,
    SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg)
{
    SBN_TCP_Peer_t *PeerData = (SBN_TCP_Peer_t *)Peer->ModulePvt;
    SBN_NetInterface_t *Net = Peer->Net;
    SBN_TCP_Net_t *NetData = (SBN_TCP_Net_t *)Net->ModulePvt;

    if (PeerData->Conn == NULL)
    {
        /* fail silently as the peer is not connected (yet) */
        return SBN_SUCCESS;
    }/* end if */

    SBN_PackMsg(&SendBufs[NetData->BufNum], MsgSz, MsgType, CFE_PSP_GetProcessorId(), Msg);
    size_t sent_size = OS_write(PeerData->Conn->Socket, &SendBufs[NetData->BufNum], MsgSz + SBN_PACKED_HDR_SZ);
    if(sent_size < MsgSz + SBN_PACKED_HDR_SZ)
    {
        CFE_EVS_SendEvent(SBN_TCP_DEBUG_EID, CFE_EVS_INFORMATION,
            "CPU %d failed to write, disconnected", Peer->ProcessorID);

        Disconnected(Peer);
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_TCP_Send */

int SBN_TCP_Recv(SBN_NetInterface_t *Net,
    SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
    SBN_CpuID_t *CpuIDPtr, void *MsgBuf)
{
    OS_FdSet FdSet;
    int32 timeout = 0;
    int ConnID = 0;
    #ifdef SBN_RECV_TASK
    timeout = 1000; /* if we are using tasks, block on select */
    #endif /* SBN_RECV_TASK */

    SBN_TCP_Net_t *NetData = (SBN_TCP_Net_t *)Net->ModulePvt;

    OS_SelectFdZero(&FdSet);

    for(ConnID = 0; ConnID < SBN_MAX_PEER_CNT; ConnID++)
    {
        if (NetData->Conns[ConnID].InUse)
        {
            OS_SelectFdAdd(&FdSet, NetData->Conns[ConnID].Socket);
        }/* end if */
    }/* end for */

    if(OS_SelectMultiple(&FdSet, NULL, timeout) != OS_SUCCESS)
    {
        return SBN_IF_EMPTY;
    }/* end if */

    for(ConnID = 0; ConnID < SBN_MAX_PEER_CNT; ConnID++)
    {
        SBN_TCP_Conn_t *Conn = &NetData->Conns[ConnID];

        if (!Conn->InUse)
        {
            continue;
        }/* end if */

        if(OS_SelectFdIsSet(&FdSet, Conn->Socket))
        {
            ssize_t Received = 0;

            int ToRead = 0;

            if(!Conn->ReceivingBody)
            {
                /* recv the header first */
                ToRead = SBN_PACKED_HDR_SZ - Conn->RecvSz;

                Received = OS_read(Conn->Socket,
                    (char *)&RecvBufs[Conn->BufNum] + Conn->RecvSz,
                    ToRead);

                if(Received <= 0)
                {
                    CFE_EVS_SendEvent(SBN_TCP_DEBUG_EID, CFE_EVS_INFORMATION,
                        "Connection %d head recv failed, disconnected",ConnID);

                    OS_close(Conn->Socket);

                    Disconnected(Conn->PeerInterface);

                    return SBN_IF_EMPTY;
                }/* end if */

                Conn->RecvSz += Received;

                if(Received >= ToRead)
                {
                    Conn->ReceivingBody = TRUE; /* and continue on to recv body */
                }
                else
                {
                    return SBN_IF_EMPTY; /* wait for the complete header */
                }/* end if */
            }/* end if */

            /* only get here if we're recv'd the header and ready for the body */

            ToRead = CFE_MAKE_BIG16(
                    *((SBN_MsgSz_t *)&RecvBufs[Conn->BufNum])
                ) + SBN_PACKED_HDR_SZ - Conn->RecvSz;
            if(ToRead)
            {
                Received = OS_read(Conn->Socket,
                    (char *)&RecvBufs[Conn->BufNum] + Conn->RecvSz,
                    ToRead);

                if(Received <= 0)
                {
                    uint32 ProcessorID = -1;
                    if(Conn->PeerInterface != NULL)
                    {
                        ProcessorID = Conn->PeerInterface->ProcessorID;
                    }/* end if */

                    CFE_EVS_SendEvent(SBN_TCP_DEBUG_EID, CFE_EVS_INFORMATION,
                        "CPUID %d body recv failed, disconnected", ProcessorID);

                    return SBN_ERROR;
                }/* end if */

                Conn->RecvSz += Received;

                if(Received < ToRead)
                {
                    return SBN_IF_EMPTY; /* wait for the complete body */
                }/* end if */
            }/* end if */

            /* we have the complete body, decode! */
            if(SBN_UnpackMsg(&RecvBufs[Conn->BufNum], MsgSzPtr, MsgTypePtr,
                CpuIDPtr, MsgBuf) == FALSE)
            {
                return SBN_ERROR;
            }/* end if */

            if(!Conn->PeerInterface)
            {
                /* New peer, link it to the connection */
                int PeerInterfaceID = 0;

                for (PeerInterfaceID = 0; PeerInterfaceID < Net->PeerCnt; PeerInterfaceID++)
                {
                    SBN_PeerInterface_t *PeerInterface = &Net->Peers[PeerInterfaceID];

                    if(PeerInterface->ProcessorID == *CpuIDPtr)
                    {
                        SBN_TCP_Peer_t *PeerData = (SBN_TCP_Peer_t *)PeerInterface->ModulePvt;

                        PeerData->Conn = Conn;

                        Conn->PeerInterface = PeerInterface;

                        SBN_Connected(PeerInterface);

                        break;
                    }/* end if */
                }/* end for */
            }/* end if */

            Conn->ReceivingBody = FALSE;
            Conn->RecvSz = 0;
        }/* end if */
    }/* end for */
    return SBN_SUCCESS;
}/* end SBN_TCP_Recv */

int SBN_TCP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_TCP_ReportModuleStatus */

int SBN_TCP_UnloadNet(SBN_NetInterface_t *Net)
{
    SBN_TCP_Net_t *NetData = (SBN_TCP_Net_t *)Net->ModulePvt;

    if(NetData->Socket)
    {
        close(NetData->Socket);
    }/* end if */

    int PeerIdx = 0;
    for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        SBN_TCP_UnloadPeer(&Net->Peers[PeerIdx]);
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_TCP_ResetPeer */

int SBN_TCP_UnloadPeer(SBN_PeerInterface_t *Peer)
{
    Disconnected(Peer);

    return SBN_SUCCESS;
}/* end SBN_TCP_ResetPeer */
