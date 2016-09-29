#ifndef _sbn_tcp_if_h_
#define _sbn_tcp_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

#ifdef _osapi_confloader_

int SBN_TCP_LoadEntry(const char **Row, int RowCount, void *EntryBuffer);

#else /* ! _osapi_confloader_ */

int SBN_TCP_ParseFileEntry(char *Line, uint32 LineNum, void *EntryBuffer);

#endif /* _osapi_confloader_ */

int SBN_TCP_InitHost(SBN_HostInterface_t *Host);

int SBN_TCP_InitPeer(SBN_PeerInterface_t *Peer);

int SBN_TCP_Send(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t *Msg);

int SBN_TCP_Recv(SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr, 
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *MsgBuf);

int SBN_TCP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet);

int SBN_TCP_ResetPeer(SBN_PeerInterface_t *Peer);

SBN_InterfaceOperations_t SBN_TCP_Ops =
{
#ifdef _osapi_confloader_
    SBN_TCP_LoadEntry,
#else /* ! _osapi_confloader_ */
    SBN_TCP_ParseFileEntry,
#endif /* _osapi_confloader_ */
    SBN_TCP_InitHost,
    SBN_TCP_InitPeer,
    SBN_TCP_Send,
    SBN_TCP_Recv,
    SBN_TCP_ReportModuleStatus,
    SBN_TCP_ResetPeer
};

#endif /* _sbn_tcp_if_h_ */
