#ifndef _sbn_tcp_if_h_
#define _sbn_tcp_if_h_

#include "sbn_types.h"
#include "sbn_interfaces.h"
#include "cfe.h"

/* The version #'s this module works with. If this doesn't match the core
 * SBN application version #'s, this module should report and return an error.
 * Note: these DEFINITELY should be hardcoded here, do not map them to
 * the SBN version numbers.
 */
#define SBN_TCP_MAJOR 1
#define SBN_TCP_MINOR 14
#define SBN_TCP_REVISION 0

CFE_Status_t SBN_TCP_LibInit(void);

SBN_Status_t SBN_TCP_Init(int Major, int Minor, int Revision);

SBN_Status_t SBN_TCP_LoadNet(SBN_NetInterface_t *Net, const char *Address);

SBN_Status_t SBN_TCP_LoadPeer(SBN_PeerInterface_t *Peer, const char *Address);

SBN_Status_t SBN_TCP_InitNet(SBN_NetInterface_t *Net);

SBN_Status_t SBN_TCP_InitPeer(SBN_PeerInterface_t *Peer);

SBN_Status_t SBN_TCP_PollPeer(SBN_PeerInterface_t *Peer);

SBN_MsgSz_t SBN_TCP_Send(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
    SBN_MsgSz_t MsgSz, void *Payload);

SBN_Status_t SBN_TCP_Recv(SBN_NetInterface_t *Net,
        SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
        CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer);

SBN_Status_t SBN_TCP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet);

SBN_Status_t SBN_TCP_UnloadNet(SBN_NetInterface_t *Net);

SBN_Status_t SBN_TCP_UnloadPeer(SBN_PeerInterface_t *Peer);

SBN_IfOps_t SBN_TCP_Ops =
{
    SBN_TCP_Init,
    SBN_TCP_LoadNet,
    SBN_TCP_LoadPeer,
    SBN_TCP_InitNet,
    SBN_TCP_InitPeer,
    SBN_TCP_PollPeer,
    SBN_TCP_Send,
    NULL,
    SBN_TCP_Recv,
    SBN_TCP_ReportModuleStatus,
    SBN_TCP_UnloadNet,
    SBN_TCP_UnloadPeer
};

#define SBN_TCP_HEARTBEAT_MSG 0xA0

#endif /* _sbn_tcp_if_h_ */
