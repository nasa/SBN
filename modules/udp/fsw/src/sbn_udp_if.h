#ifndef _sbn_udp_if_h_
#define _sbn_udp_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

int SBN_UDP_LoadNet(const char **Row, int FieldCount,
    SBN_NetInterface_t *Net);

int SBN_UDP_LoadPeer(const char **Row, int FieldCount,
    SBN_PeerInterface_t *Peer);

int SBN_UDP_InitNet(SBN_NetInterface_t *NetInterface);

int SBN_UDP_InitPeer(SBN_PeerInterface_t *PeerInterface);

int SBN_UDP_Send(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t Payload);

int SBN_UDP_Recv(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr,
        SBN_MsgSize_t *MsgSizePtr, SBN_CpuID_t *CpuIDPtr,
        SBN_Payload_t PayloadBuffer);

int SBN_UDP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet);

int SBN_UDP_ResetPeer(SBN_PeerInterface_t *PeerInterface);

SBN_IfOps_t SBN_UDP_Ops =
{
    SBN_UDP_LoadNet,
    SBN_UDP_LoadPeer,
    SBN_UDP_InitNet,
    SBN_UDP_InitPeer,
    SBN_UDP_Send,
    NULL,
    SBN_UDP_Recv,
    SBN_UDP_ReportModuleStatus,
    SBN_UDP_ResetPeer
};

#endif /* _sbn_udp_if_h_ */
