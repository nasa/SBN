#ifndef _sbn_udp_if_h_
#define _sbn_udp_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

#ifdef CFE_ES_CONFLOADER

int SBN_UDP_LoadEntry(const char **Row, int RowCount, void *EntryBuffer);

#else /* ! CFE_ES_CONFLOADER */

int SBN_UDP_ParseFileEntry(char *Line, uint32 LineNum, void *EntryBuffer);

#endif /* CFE_ES_CONFLOADER */

int SBN_UDP_InitHost(SBN_HostInterface_t *HostInterface);

int SBN_UDP_InitPeer(SBN_PeerInterface_t *PeerInterface);

int SBN_UDP_Send(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t *Msg);

int SBN_UDP_Recv(SBN_PeerInterface_t *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *MsgBuf);

int SBN_UDP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet);

int SBN_UDP_ResetPeer(SBN_PeerInterface_t *PeerInterface);

SBN_InterfaceOperations_t SBN_UDP_Ops =
{
#ifdef CFE_ES_CONFLOADER
    SBN_UDP_LoadEntry,
#else /* ! CFE_ES_CONFLOADER */
    SBN_UDP_ParseFileEntry,
#endif /* CFE_ES_CONFLOADER */
    SBN_UDP_InitHost,
    SBN_UDP_InitPeer,
    SBN_UDP_Send,
    SBN_UDP_Recv,
    SBN_UDP_ReportModuleStatus,
    SBN_UDP_ResetPeer
};

#endif /* _sbn_udp_if_h_ */
