#ifndef _sbn_serial_if_h_
#define _sbn_serial_if_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "cfe.h"
#include "serial_sbn_if_struct.h"
#include "serial_io.h"

/* Interface-specific functions */
void  SBN_ShowSerialPeerData(int i);

/* SBN_InterfaceOperations functions */
int32 Serial_SbnParseInterfaceFileEntry(char *FileEntry, uint32 LineNum, void** EntryAddr);
int32 Serial_SbnInitPeerInterface(SBN_InterfaceData* data);
int32 Serial_SbnSendNetMsg(uint32 MsgType, uint32 MsgSize, SBN_InterfaceData *HostList[], int32 NumHosts, CFE_SB_SenderId_t *SenderPtr, SBN_InterfaceData *IfData, SBN_NetProtoMsg_t *ProtoMsgBuf, NetDataUnion *DataMsgBuf);
int32 Serial_SbnCheckForNetProtoMsg(SBN_InterfaceData *Peer, SBN_NetProtoMsg_t *ProtoMsgBuf);
int32 Serial_SbnReceiveMsg(SBN_InterfaceData *Host, NetDataUnion *DataMsgBuf);
int32 Serial_SbnSendNetMsg(uint32 MsgType, uint32 MsgSize, SBN_InterfaceData *HostList[], int32 NumHosts, CFE_SB_SenderId_t *SenderPtr, SBN_InterfaceData *IfData, SBN_NetProtoMsg_t *ProtoMsgBuf, NetDataUnion *DataMsgBuf);
int32 Serial_SbnVerifyPeerInterface(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts);
int32 Serial_SbnVerifyHostInterface(SBN_InterfaceData *Host, SBN_PeerData_t *PeerList, int32 NumPeers);
int32 Serial_SbnReportModuleStatus(SBN_ModuleStatusPacket_t *StatusPkt, SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts);
int32 Serial_SbnResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int32 NumHosts);

/* Utility functions */
int32 Serial_GetHostPeerMatchData(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], Serial_SBNHostData_t **HostData, Serial_SBNPeerData_t **PeerData, int32 NumHosts); 
int32 Serial_IsHostPeerMatch(Serial_SBNEntry_t *Host, Serial_SBNEntry_t *Peer);


/* Interface operations called by SBN */
SBN_InterfaceOperations SerialOps = {
    Serial_SbnParseInterfaceFileEntry,
    Serial_SbnInitPeerInterface,
    Serial_SbnSendNetMsg,
    Serial_SbnCheckForNetProtoMsg,
    Serial_SbnReceiveMsg,
    Serial_SbnVerifyPeerInterface,
    Serial_SbnVerifyHostInterface,
    Serial_SbnReportModuleStatus,
    Serial_SbnResetPeer
};


#endif /* _sbn_serial_if_h_ */
