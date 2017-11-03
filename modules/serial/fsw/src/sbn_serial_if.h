#ifndef _sbn_serial_if_h_
#define _sbn_serial_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

int SBN_SERIAL_LoadNet(const char **Row, int FieldCount,
        SBN_NetInterface_t *Net);

int SBN_SERIAL_LoadPeer(const char **Row, int FieldCount,
        SBN_PeerInterface_t *Peer);

int SBN_SERIAL_InitNet(SBN_NetInterface_t *Net);

int SBN_SERIAL_InitPeer(SBN_PeerInterface_t *Peer);

int SBN_SERIAL_PollPeer(SBN_PeerInterface_t *Peer);

int SBN_SERIAL_Send(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
    SBN_MsgSz_t MsgSz, void *Payload);

int SBN_SERIAL_Recv(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer,
        SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
        SBN_CpuID_t *CpuIDPtr, void *PayloadBuffer);

int SBN_SERIAL_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet);

int SBN_SERIAL_ResetPeer(SBN_PeerInterface_t *Peer);

int SBN_SERIAL_UnloadNet(SBN_NetInterface_t *Net);

int SBN_SERIAL_UnloadPeer(SBN_PeerInterface_t *Peer);

SBN_IfOps_t SBN_SERIAL_Ops =
{
    SBN_SERIAL_LoadNet,
    SBN_SERIAL_LoadPeer,
    SBN_SERIAL_InitNet,
    SBN_SERIAL_InitPeer,
    SBN_SERIAL_PollPeer,
    SBN_SERIAL_Send,
    SBN_SERIAL_Recv,
    NULL,
    SBN_SERIAL_ReportModuleStatus,
    SBN_SERIAL_ResetPeer,
    SBN_SERIAL_UnloadNet,
    SBN_SERIAL_UnloadPeer
};

/**
 * SBN message type for heartbeat messages.
 */
#define SBN_SERIAL_HEARTBEAT_MSG 0xA0

/**
 * If unable to connect to the serial device, try again in
 * SBN_SERIAL_CONNTRY_TIME seconds.
 */
#define SBN_SERIAL_CONNTRY_TIME 10

#endif /* _sbn_serial_if_h_ */
