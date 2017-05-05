#ifndef _sbn_dtn_if_h_
#define _sbn_dtn_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

/* sent to all peers, requesting they send their subscriptions */
#define SBN_DTN_SUBREQ_MSG 0xA0

int SBN_DTN_LoadNet(const char **Row, int FieldCount, SBN_NetInterface_t *Net);

int SBN_DTN_LoadPeer(const char **Row, int FieldCount,
    SBN_PeerInterface_t *Peer);

int SBN_DTN_InitNet(SBN_NetInterface_t *NetInterface);

int SBN_DTN_InitPeer(SBN_PeerInterface_t *PeerInterface);

int SBN_DTN_PollPeer(SBN_PeerInterface_t *PeerInterface);

int SBN_DTN_Send(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t Payload);

int SBN_DTN_Recv(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr,
        SBN_MsgSize_t *MsgSizePtr, SBN_CpuID_t *CpuIDPtr,
        SBN_Payload_t PayloadBuffer);

int SBN_DTN_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet);

int SBN_DTN_ResetPeer(SBN_PeerInterface_t *PeerInterface);

SBN_IfOps_t SBN_DTN_Ops =
{
    SBN_DTN_LoadNet,
    SBN_DTN_LoadPeer,
    SBN_DTN_InitNet,
    SBN_DTN_InitPeer,
    SBN_DTN_PollPeer,
    SBN_DTN_Send,
    NULL,
    SBN_DTN_Recv,
    SBN_DTN_ReportModuleStatus,
    SBN_DTN_ResetPeer
};

#endif /* _sbn_dtn_if_h_ */
