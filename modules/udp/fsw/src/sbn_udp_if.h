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

int SBN_UDP_Init(SBN_Interface_t* data);

int SBN_UDP_Send(SBN_Interface_t *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t *Msg);

int SBN_UDP_Recv(SBN_Interface_t *PeerInterface, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *MsgBuf);

int SBN_UDP_VerifyPeerInterface(SBN_Interface_t *Peer,
    SBN_Interface_t *HostList[], int HostCount);

int SBN_UDP_VerifyHostInterface(SBN_Interface_t *Host,
    SBN_Peer_t *PeerList, int PeerCount);

int SBN_UDP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet,
    SBN_Interface_t *Peer, SBN_Interface_t *HostList[],
    int HostCount);

int SBN_UDP_ResetPeer(SBN_Interface_t *Peer, SBN_Interface_t *HostList[],
    int HostCount);

SBN_InterfaceOperations_t SBN_UDP_Ops =
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
