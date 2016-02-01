#ifndef _sbn_ipv4_if_h_
#define _sbn_ipv4_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"

void  SBN_ShowIPv4PeerData(int i);
void  SBN_SendSockFailedEvent(uint32 Line, int RtnVal);
void  SBN_SendBindFailedEvent(uint32 Line, int RtnVal);
int   SBN_CreateSocket(char *Addr, int Port);
void  SBN_ClearSocket(int SockID);
int   SBN_IPv4RcvMsg(SBN_InterfaceData *Host, NetDataUnion *DataMsgBuf);
int32 SBN_ParseIPv4FileEntry(char *FileEntry, uint32 LineNum, void** EntryAddr);
int32 SBN_InitIPv4IF(SBN_InterfaceData* data);
int32 SBN_SendIPv4NetMsg(uint32 MsgType, uint32 MsgSize, SBN_InterfaceData *HostList[], int32 NumHosts, SBN_SenderId_t *SenderPtr, SBN_InterfaceData *IfData, NetDataUnion *DataMsgBuf);
int32 IPv4_VerifyPeerInterface(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts);
int32 IPv4_VerifyHostInterface(SBN_InterfaceData *Host, SBN_PeerData_t *PeerList, int32 NumPeers);
int32 IPv4_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet, SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts);
int32 IPv4_ResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts);

SBN_InterfaceOperations IPv4Ops = {
    SBN_ParseIPv4FileEntry,
    SBN_InitIPv4IF,
    SBN_SendIPv4NetMsg,
    SBN_IPv4RcvMsg,
    IPv4_VerifyPeerInterface,
    IPv4_VerifyHostInterface,
    IPv4_ReportModuleStatus,
    IPv4_ResetPeer
};


#endif /* _sbn_ipv4_if_h_ */
