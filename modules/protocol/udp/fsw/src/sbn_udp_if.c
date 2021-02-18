#include "sbn_udp_events.h"
#include "sbn_udp_if.h"
#include "sbn_platform_cfg.h"
#include <string.h>
#include <errno.h>

#include "sbn_interfaces.h"
#include "cfe.h"

CFE_EVS_EventID_t SBN_UDP_FIRST_EID;

#define EXP_VERSION 6

static SBN_ProtocolOutlet_t SBN;

static SBN_Status_t Init(int Version, CFE_EVS_EventID_t BaseEID, SBN_ProtocolOutlet_t *Outlet)
{
    SBN_UDP_FIRST_EID = BaseEID;

    if (Version != EXP_VERSION)
    {
        OS_printf("SBN_UDP version mismatch: expected %d, got %d\n", EXP_VERSION, Version);
        return SBN_ERROR;
    } /* end if */

    if (Outlet == NULL)
    {
        OS_printf("SBN_UDP outlet is NULL\n");
        return SBN_ERROR;
    } /* end if */

    /* copy outlet pointers to a local buffer for later use */
    memcpy(&SBN, Outlet, sizeof(SBN));

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

    OS_SockAddr_t LOCAL_ADDR;
    uint16 Port;
    OS_SocketAddrInit(&LOCAL_ADDR, OS_SocketDomain_INET);
    OS_SocketAddrFromString(&LOCAL_ADDR, "0.0.0.0");
    OS_SocketAddrGetPort(&Port, &NetData->Addr);
    OS_SocketAddrSetPort(&LOCAL_ADDR, Port);

    EVSSendInfo(SBN_UDP_SOCK_EID, "creating socket (NetData=0x%lx)", (long unsigned int)NetData);

    if (OS_SocketOpen(&(NetData->Socket), OS_SocketDomain_INET, OS_SocketType_DATAGRAM) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_SOCK_EID, "socket open call failed");
        return SBN_ERROR;
    } /* end if */

    if (OS_SocketBind(NetData->Socket, &LOCAL_ADDR) != OS_SUCCESS)
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
    AddrHost[ColonLen] = '\0';

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

    EVSSendInfo(SBN_UDP_CONFIG_EID, "addr host: '%s'", AddrHost);
    if ((Status = OS_SocketAddrFromString(Addr, AddrHost)) != OS_SUCCESS)
    {
        EVSSendErr(SBN_UDP_CONFIG_EID, "addr host set failed (AddrHost=%s, Status=%d)", AddrHost, Status);
        return SBN_ERROR;
    } /* end if */

    EVSSendInfo(SBN_UDP_CONFIG_EID, "addr port: '%ld'", Port);
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

    EVSSendInfo(SBN_UDP_CONFIG_EID, "configuring peer (SC=%d, CPU=%d, Address=%s)",
        Peer->SpacecraftID,
        Peer->ProcessorID,
        Address);

    SBN_Status_t Status = ConfAddr(&PeerData->Addr, Address);

    if (Status == SBN_SUCCESS)
    {
      EVSSendInfo(SBN_UDP_CONFIG_EID, "configured peer (SC=%d, CPU=%d, Address=%s)",
          Peer->SpacecraftID,
          Peer->ProcessorID,
          Address);
    } /* end if */

    return Status;
} /* end LoadPeer() */

static SBN_Status_t PollPeer(SBN_PeerInterface_t *Peer)
{
    OS_time_t CurrentTime;
    OS_GetLocalTime(&CurrentTime);

    EVSSendDbg(SBN_UDP_DEBUG_EID, "polling peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);

    if (Peer->Connected)
    {
        if (OS_TimeGetTotalSeconds(OS_TimeSubtract(CurrentTime, Peer->LastRecv)) > SBN_UDP_PEER_TIMEOUT)
        {
            EVSSendInfo(SBN_UDP_DEBUG_EID, "disconnected peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);

            SBN.Disconnected(Peer);
            return SBN_SUCCESS;
        } /* end if */

        if (OS_TimeGetTotalSeconds(OS_TimeSubtract(CurrentTime, Peer->LastSend)) > SBN_UDP_PEER_HEARTBEAT)
        {
            OS_GetLocalTime(&Peer->LastSend);
            EVSSendDbg(SBN_UDP_DEBUG_EID, "sending heartbeat to peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
            return SBN.SendNetMsg(SBN_UDP_HEARTBEAT_MSG, 0, NULL, Peer);
        } /* end if */
    }
    else
    {
        if (Peer->ProcessorID != CFE_PSP_GetProcessorId() &&
            Peer->SpacecraftID != CFE_PSP_GetSpacecraftId() &&
            OS_TimeGetTotalSeconds(OS_TimeSubtract(CurrentTime, Peer->LastSend)) > SBN_UDP_ANNOUNCE_TIMEOUT)
        {
            OS_GetLocalTime(&Peer->LastSend);
            EVSSendInfo(SBN_UDP_DEBUG_EID, "announce to peer %d:%d", Peer->SpacecraftID, Peer->ProcessorID);
            return SBN.SendNetMsg(SBN_UDP_ANNOUNCE_MSG, 0, NULL, Peer);
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

    SBN.PackMsg(Buf, MsgSz, MsgType, CFE_PSP_GetProcessorId(), CFE_PSP_GetSpacecraftId(), Payload);

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
                         CFE_ProcessorID_t *ProcessorIDPtr, CFE_SpacecraftID_t *SpacecraftIDPtr, void *Payload)
{
    uint8 RecvBuf[SBN_MAX_PACKED_MSG_SZ];

    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    /* task-based peer connections block on reads, otherwise use select */

    uint32 StateFlags = OS_STREAM_STATE_READABLE;

    /* polling uses select, otherwise drop through to block on read for task */
    if(!(Net->TaskFlags & SBN_TASK_RECV)
        && (OS_SelectSingle(NetData->Socket, &StateFlags, 0) != OS_SUCCESS
        || !(StateFlags & OS_STREAM_STATE_READABLE)))
    {
        /* nothing to receive */
        return SBN_IF_EMPTY;
    }/* end if */

    int Received = OS_SocketRecvFrom(NetData->Socket, (char *)&RecvBuf, CFE_MISSION_SB_MAX_SB_MSG_SIZE, NULL, OS_PEND);

    if (Received < 0)
    {
        EVSSendErr(SBN_UDP_DEBUG_EID, "ERROR: could not receive from socket: status=%d", Received);
        return SBN_ERROR;
    } /* end if */

    /* each UDP packet is a full SBN message */

    if (SBN.UnpackMsg(&RecvBuf, MsgSzPtr, MsgTypePtr, ProcessorIDPtr, SpacecraftIDPtr, Payload) == false)
    {
        EVSSendErr(SBN_UDP_DEBUG_EID, "ERROR: could not unpack message");
        return SBN_ERROR;
    } /* end if */

    SBN_PeerInterface_t *Peer = SBN.GetPeer(Net, *ProcessorIDPtr, *SpacecraftIDPtr);
    if (Peer == NULL)
    {
        EVSSendErr(SBN_UDP_DEBUG_EID, "ERROR: unknown peer %d:%d", *SpacecraftIDPtr, *ProcessorIDPtr);
        return SBN_ERROR;
    } /* end if */

    if (!Peer->Connected)
    {
        EVSSendInfo(SBN_UDP_DEBUG_EID, "connecting to peer %d:%d", *SpacecraftIDPtr, *ProcessorIDPtr);
        SBN.Connected(Peer);
    } else {
        EVSSendDbg(SBN_UDP_DEBUG_EID, "already connected to peer %d:%d", *SpacecraftIDPtr, *ProcessorIDPtr);
    } /* end if */

    if (*MsgTypePtr == SBN_UDP_DISCONN_MSG)
    {
        SBN.Disconnected(Peer);
    }

    return SBN_SUCCESS;
} /* end Recv() */

static SBN_Status_t UnloadPeer(SBN_PeerInterface_t *Peer)
{
    if (Peer->Connected)
    {
        EVSSendInfo(SBN_UDP_DEBUG_EID, "peer %d:%d - sending disconnect", Peer->SpacecraftID, Peer->ProcessorID);
        SBN.SendNetMsg(SBN_UDP_DISCONN_MSG, 0, NULL, Peer);
        SBN.Disconnected(Peer);
    } /* end if */

    return SBN_SUCCESS;
} /* end UnloadPeer() */

static SBN_Status_t UnloadNet(SBN_NetInterface_t *Net)
{
    SBN_Status_t Status = SBN_SUCCESS;

    SBN_PeerIdx_t PeerIdx = 0;
    for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        if(UnloadPeer(&Net->Peers[PeerIdx]) != SBN_SUCCESS) {
          EVSSendInfo(SBN_UDP_DEBUG_EID, "failed to unload peer: %d\n", PeerIdx);
	  Status = SBN_ERROR;
	} else {
          EVSSendInfo(SBN_UDP_DEBUG_EID, "unloaded peer: %d\n", PeerIdx);
        }
    } /* end if */

    return Status;
} /* end UnloadNet() */

SBN_IfOps_t SBN_UDP_Ops = {Init, InitNet, InitPeer, LoadNet,   LoadPeer,  PollPeer,
                           Send, NULL,    Recv,     UnloadNet, UnloadPeer};
