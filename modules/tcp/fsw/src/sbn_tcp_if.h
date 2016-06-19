#ifndef _sbn_tcp_if_h_
#define _sbn_tcp_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

#ifdef _osapi_confloader_

int SBN_TCP_LoadEntry(const char **, int, void *);

#else /* ! _osapi_confloader_ */

int SBN_TCP_ParseFileEntry(char *, uint32, void *);

#endif /* _osapi_confloader_ */

int SBN_TCP_Init(SBN_InterfaceData* data);

int SBN_TCP_Send(SBN_InterfaceData *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, void *Msg);

int SBN_TCP_Recv(SBN_InterfaceData *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, void *MsgBuf);

int SBN_TCP_VerifyPeerInterface(SBN_InterfaceData *Peer,
    SBN_InterfaceData *HostList[], int NumHosts);

int SBN_TCP_VerifyHostInterface(SBN_InterfaceData *Host,
    SBN_PeerData_t *PeerList, int NumPeers);

int SBN_TCP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet,
    SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
    int NumHosts);

int SBN_TCP_ResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
    int NumHosts);

SBN_InterfaceOperations SBN_TCP_Ops =
{
#ifdef _osapi_confloader_
    SBN_TCP_LoadEntry,
#else /* ! _osapi_confloader_ */
    SBN_TCP_ParseFileEntry,
#endif /* _osapi_confloader_ */
    SBN_TCP_Init,
    SBN_TCP_Send,
    SBN_TCP_Recv,
    SBN_TCP_VerifyPeerInterface,
    SBN_TCP_VerifyHostInterface,
    SBN_TCP_ReportModuleStatus,
    SBN_TCP_ResetPeer
};

#endif /* _sbn_tcp_if_h_ */
