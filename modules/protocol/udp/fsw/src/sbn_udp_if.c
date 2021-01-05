#include "sbn_udp_events.h"
#include "sbn_udp_if.h"
#include "sbn_platform_cfg.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>

#include "sbn_interfaces.h"
#include "cfe.h"

CFE_EVS_EventID_t SBN_UDP_FIRST_EID;

#define EXP_VERSION 5

static SBN_Status_t Init(int Version, CFE_EVS_EventID_t BaseEID)
{
    SBN_UDP_FIRST_EID = BaseEID;

    if (Version != EXP_VERSION)
    {
        OS_printf("SBN_UDP version mismatch: expected %d, got %d\n", EXP_VERSION, Version);
        return SBN_ERROR;
    } /* end if */

    OS_printf("SBN_UDP Lib Initialized.\n");
    return SBN_SUCCESS;
} /* end Init() */

/**
 * Initializes an UDP host.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
static SBN_Status_t InitNet(SBN_NetInterface_t *Net)
{
    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    EVSSendDbg(SBN_UDP_SOCK_EID, "creating socket (NetData=0x%lx)", (long unsigned int)NetData);

    if (OS_SocketOpen(&(NetData->Socket), OS_SocketDomain_INET, OS_SocketType_DATAGRAM) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_SOCK_EID, "socket call failed");
        return SBN_ERROR;
    } /* end if */

    if (OS_SocketBind(NetData->Socket, &NetData->Addr) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_SOCK_EID, "bind call failed (NetData=0x%lx, Socket=%d)", (long unsigned int)NetData,
                   NetData->Socket);
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
} /* end InitNet() */

/**
 * Initializes an UDP host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
static SBN_Status_t InitPeer(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
} /* end InitPeer() */

static SBN_Status_t ConfAddr(OS_SockAddr_t *Addr, const char *Address)
{
    SBN_Status_t Status = SBN_SUCCESS;

    char AddrHost[OS_MAX_API_NAME];

    char *Colon    = strchr(Address, ':');
    int   ColonLen = 0;

    if (!Colon || (ColonLen = Colon - Address) >= OS_MAX_API_NAME)
    {
        EVSSendErr(SBN_UDP_CONFIG_EID, "invalid address (Address=%s)", Address);
        return SBN_ERROR;
    } /* end if */

    strncpy(AddrHost, Address, ColonLen);

    char *ValidatePtr = NULL;
    long  Port        = strtol(Colon + 1, &ValidatePtr, 0);
    if (!ValidatePtr || ValidatePtr == Colon + 1)
    {
        EVSSendErr(SBN_UDP_CONFIG_EID, "invalid port (Address=%s)", Address);
        return SBN_ERROR;
    } /* end if */

    if ((Status = OS_SocketAddrInit(Addr, OS_SocketDomain_INET)) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_CONFIG_EID, "addr init failed (Status=%d)", Status);
        return SBN_ERROR;
    } /* end if */

    if ((Status = OS_SocketAddrFromString(Addr, AddrHost)) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_CONFIG_EID, "addr host set failed (AddrHost=%s, Status=%d)", AddrHost, Status);
        return SBN_ERROR;
    } /* end if */

    if ((Status = OS_SocketAddrSetPort(Addr, Port)) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_CONFIG_EID, "addr port set failed (Port=%ld, Status=%d)", Port, Status);
        return SBN_ERROR;
    } /* end if */

    return SBN_SUCCESS;
} /* end ConfAddr() */

static SBN_Status_t LoadNet(SBN_NetInterface_t *Net, const char *Address)
{
    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    EVSSendInfo(SBN_UDP_CONFIG_EID, "configuring net (NetData=0x%lx, Address=%s)", (long unsigned int)NetData, Address);

    SBN_Status_t Status = ConfAddr(&NetData->Addr, Address);

    if (Status == SBN_SUCCESS)
    {
        EVSSendInfo(SBN_UDP_CONFIG_EID, "configured (NetData=0x%lx)", (long unsigned int)NetData);
    } /* end if */

    return Status;
} /* end LoadNet */

static SBN_Status_t LoadPeer(SBN_PeerInterface_t *Peer, const char *Address)
{
    SBN_UDP_Peer_t *PeerData = (SBN_UDP_Peer_t *)Peer->ModulePvt;

    EVSSendInfo(SBN_UDP_CONFIG_EID, "configuring peer (PeerData=0x%lx, Address=%s)", (long unsigned int)PeerData,
                Address);

    SBN_Status_t Status = ConfAddr(&PeerData->Addr, Address);

    if (Status == SBN_SUCCESS)
    {
        EVSSendInfo(SBN_UDP_CONFIG_EID, "configured (PeerData=0x%lx)", (long unsigned int)PeerData);
    } /* end if */

    return Status;
} /* end LoadPeer() */

static SBN_Status_t PollPeer(SBN_PeerInterface_t *Peer)
{
    OS_time_t CurrentTime;
    OS_GetLocalTime(&CurrentTime);

    if (Peer->Connected)
    {
        if (CurrentTime.seconds - Peer->LastRecv.seconds > SBN_UDP_PEER_TIMEOUT)
        {
            EVSSendInfo(SBN_UDP_DEBUG_EID, "disconnected CPU %d", Peer->ProcessorID);

            SBN_Disconnected(Peer);
            return SBN_SUCCESS;
        } /* end if */

        if (CurrentTime.seconds - Peer->LastSend.seconds > SBN_UDP_PEER_HEARTBEAT)
        {
            EVSSendInfo(SBN_UDP_DEBUG_EID, "heartbeat CPU %d", Peer->ProcessorID);
            return SBN_SendNetMsg(SBN_UDP_HEARTBEAT_MSG, 0, NULL, Peer);
        } /* end if */
    }
    else
    {
        if (Peer->ProcessorID < CFE_PSP_GetProcessorId() &&
            CurrentTime.seconds - Peer->LastSend.seconds > SBN_UDP_ANNOUNCE_TIMEOUT)
        {
            EVSSendInfo(SBN_UDP_DEBUG_EID, "announce CPU %d", Peer->ProcessorID);
            return SBN_SendNetMsg(SBN_UDP_ANNOUNCE_MSG, 0, NULL, Peer);
        } /* end if */
    }     /* end if */

    return SBN_SUCCESS;
} /* end PollPeer() */

static SBN_Status_t Send(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload)
{
    int32 BufSz = MsgSz + SBN_PACKED_HDR_SZ, SentSz = 0;
    uint8 Buf[BufSz];

    SBN_UDP_Peer_t *    PeerData = (SBN_UDP_Peer_t *)Peer->ModulePvt;
    SBN_NetInterface_t *Net      = Peer->Net;
    SBN_UDP_Net_t *     NetData  = (SBN_UDP_Net_t *)Net->ModulePvt;

    SBN_PackMsg(Buf, MsgSz, MsgType, CFE_PSP_GetProcessorId(), Payload);

    OS_SockAddr_t Addr;
    if (OS_SocketAddrInit(&Addr, OS_SocketDomain_INET) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_SOCK_EID, "socket addr init failed");
        return SBN_ERROR;
    } /* end if */

    SentSz = OS_SocketSendTo(NetData->Socket, Buf, BufSz, &PeerData->Addr);

    if (SentSz < BufSz)
    {
        EVSSendErr(SBN_UDP_SOCK_EID, "incomplete socket send, tried to send %d bytes, returned %d", (int)BufSz,
                   (int)SentSz);
        return SBN_ERROR;
    }
    else
    {
        return SBN_SUCCESS;
    } /* end if */
} /* end Send() */

/* Note that this Recv function is indescriminate, packets will be received
 * from all peers but that's ok, I just inject them into the SB and all is
 * good!
 */
static SBN_Status_t Recv(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
                         CFE_ProcessorID_t *ProcessorIDPtr, void *Payload)
{
    uint8 RecvBuf[SBN_MAX_PACKED_MSG_SZ];

    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    /* task-based peer connections block on reads, otherwise use select */

    uint32 StateFlags = OS_STREAM_STATE_READABLE;

    /* timeout returns an OS error */
    if (OS_SelectSingle(NetData->Socket, &StateFlags, 0) != OS_SUCCESS || !(StateFlags & OS_STREAM_STATE_READABLE))
    {
        /* nothing to receive */
        return SBN_IF_EMPTY;
    } /* end if */

    int Received = OS_SocketRecvFrom(NetData->Socket, (char *)&RecvBuf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, NULL, OS_PEND);

    if (Received < 0)
    {
        return SBN_ERROR;
    } /* end if */

    /* each UDP packet is a full SBN message */

    if (SBN_UnpackMsg(&RecvBuf, MsgSzPtr, MsgTypePtr, ProcessorIDPtr, Payload) == false)
    {
        return SBN_ERROR;
    } /* end if */

    SBN_PeerInterface_t *Peer = SBN_GetPeer(Net, *ProcessorIDPtr);
    if (Peer == NULL)
    {
        return SBN_ERROR;
    } /* end if */

    if (!Peer->Connected)
    {
        SBN_Connected(Peer);
    } /* end if */

    if (*MsgTypePtr == SBN_UDP_DISCONN_MSG)
    {
        SBN_Disconnected(Peer);
    }

    return SBN_SUCCESS;
} /* end Recv() */

static SBN_Status_t UnloadPeer(SBN_PeerInterface_t *Peer)
{
    if (Peer->Connected)
    {
        EVSSendInfo(SBN_UDP_DEBUG_EID, "peer%d - sending disconnect", Peer->ProcessorID);
        SBN_SendNetMsg(SBN_UDP_DISCONN_MSG, 0, NULL, Peer);
        SBN_Disconnected(Peer);
    } /* end if */

    return SBN_SUCCESS;
} /* end UnloadPeer() */

static SBN_Status_t UnloadNet(SBN_NetInterface_t *Net)
{
    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    OS_close(NetData->Socket);

    SBN_PeerIdx_t PeerIdx = 0;
    for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        UnloadPeer(&Net->Peers[PeerIdx]);
    } /* end if */

    return SBN_SUCCESS;
} /* end UnloadNet() */

SBN_IfOps_t SBN_UDP_Ops = {Init, InitNet, InitPeer, LoadNet,   LoadPeer,  PollPeer,
                           Send, NULL,    Recv,     UnloadNet, UnloadPeer};
