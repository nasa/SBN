#ifndef _sbn_udp_if_h_
#define _sbn_udp_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

#ifdef _osapi_confloader_

int SBN_UDP_LoadEntry(const char **, int, void *);

#else /* ! _osapi_confloader_ */

int SBN_UDP_ParseFileEntry(char *, uint32, void *);

#endif /* _osapi_confloader_ */

int SBN_UDP_Init(SBN_InterfaceData* data);

int SBN_UDP_Send(SBN_InterfaceData *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t *Msg);

int SBN_UDP_Recv(SBN_InterfaceData *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *MsgBuf);

int SBN_UDP_VerifyPeerInterface(SBN_InterfaceData *Peer,
    SBN_InterfaceData *HostList[], int NumHosts);

int SBN_UDP_VerifyHostInterface(SBN_InterfaceData *Host,
    SBN_PeerData_t *PeerList, int NumPeers);

int SBN_UDP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet,
    SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
    int NumHosts);

int SBN_UDP_ResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
    int NumHosts);

SBN_InterfaceOperations SBN_UDP_Ops =
{
#ifdef _osapi_confloader_
    SBN_UDP_LoadEntry,
#else /* ! _osapi_confloader_ */
    SBN_UDP_ParseFileEntry,
#endif /* _osapi_confloader_ */
    SBN_UDP_Init,
    SBN_UDP_Send,
    SBN_UDP_Recv,
    SBN_UDP_VerifyPeerInterface,
    SBN_UDP_VerifyHostInterface,
    SBN_UDP_ReportModuleStatus,
    SBN_UDP_ResetPeer
};

#endif /* _sbn_udp_if_h_ */
