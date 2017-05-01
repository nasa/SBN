#include "sbn_udp_if_struct.h"
#include "sbn_udp_if.h"
#include "sbn_udp_events.h"
#include "cfe.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

int SBN_UDP_LoadNet(const char **Row, int FieldCount,
    SBN_NetInterface_t *Net)
{
    char *ValidatePtr = NULL;
    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    if(FieldCount < SBN_UDP_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_UDP_CONFIG_EID, CFE_EVS_ERROR,
                "invalid host entry (expected %d items, found %d)",
                SBN_UDP_ITEMS_PER_FILE_LINE, FieldCount);
        return SBN_ERROR;
    }/* end if */

    strncpy(NetData->Host, Row[0], sizeof(NetData->Host));
    NetData->Port = strtol(Row[1], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[1])
    {
        CFE_EVS_SendEvent(SBN_UDP_CONFIG_EID, CFE_EVS_ERROR,
                "invalid port");
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_UDP_LoadNet */

int SBN_UDP_LoadPeer(const char **Row, int FieldCount,
    SBN_PeerInterface_t *Peer)
{
    char *ValidatePtr = NULL;
    SBN_UDP_Peer_t *PeerData = (SBN_UDP_Peer_t *)Peer->ModulePvt;

    if(FieldCount < SBN_UDP_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_UDP_CONFIG_EID, CFE_EVS_ERROR,
                "invalid peer entry (expected %d items, found %d)",
                SBN_UDP_ITEMS_PER_FILE_LINE, FieldCount);
        return SBN_ERROR;
    }/* end if */

    strncpy(PeerData->Host, Row[0], sizeof(PeerData->Host));
    PeerData->Port = strtol(Row[1], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[1])
    {
        CFE_EVS_SendEvent(SBN_UDP_CONFIG_EID, CFE_EVS_ERROR,
                "invalid port");
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_UDP_LoadHost */

/**
 * Initializes an UDP host.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
int SBN_UDP_InitNet(SBN_NetInterface_t *Net)
{
    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_DEBUG,
        "creating socket (%s:%d)", NetData->Host, NetData->Port);

    if((NetData->Socket
        = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
            "socket call failed (errno=%d)", errno);
        return SBN_ERROR;
    }/* end if */

    static struct sockaddr_in my_addr;

    my_addr.sin_addr.s_addr = inet_addr(NetData->Host);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(NetData->Port);

    if(bind(NetData->Socket, (struct sockaddr *) &my_addr,
        sizeof(my_addr)) < 0)
    {
        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
            "bind call failed (%s:%d Socket=%d errno=%d)",
            NetData->Host, NetData->Port, NetData->Socket, errno);
        return SBN_ERROR;
    }/* end if */

    return SBN_SUCCESS;
}/* end SBN_UDP_InitNet */

/**
 * Initializes an UDP host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_SUCCESS on success, error code otherwise
 */
int SBN_UDP_InitPeer(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}/* end SBN_UDP_InitPeer */

int SBN_UDP_Send(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, SBN_Payload_t Payload)
{
    SBN_PackedMsg_t SendBuf;

    SBN_UDP_Peer_t *PeerData = (SBN_UDP_Peer_t *)Peer->ModulePvt;
    SBN_NetInterface_t *Net = Peer->Net;
    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

    SBN_PackMsg(&SendBuf, MsgSize, MsgType, CFE_CPU_ID, Payload);

    static struct sockaddr_in s_addr;

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(PeerData->Host);
    s_addr.sin_port = htons(PeerData->Port);

    sendto(NetData->Socket, &SendBuf,
        MsgSize + SBN_PACKED_HDR_SIZE, 0,
        (struct sockaddr *) &s_addr, sizeof(s_addr));

    return SBN_SUCCESS;
}/* end SBN_UDP_Send */

/* Note that this Recv function is indescriminate, packets will be received
 * from all peers but that's ok, I just inject them into the SB and all is
 * good!
 */
int SBN_UDP_Recv(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr,
        SBN_MsgSize_t *MsgSizePtr, SBN_CpuID_t *CpuIDPtr,
        SBN_Payload_t Payload)
{
    SBN_PackedMsg_t RecvBuf;

    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)Net->ModulePvt;

#ifndef SBN_RECV_TASK

    /* task-based peer connections block on reads, otherwise use select */
  
    fd_set ReadFDs;
    struct timeval Timeout;

    FD_ZERO(&ReadFDs);
    FD_SET(NetData->Socket, &ReadFDs);

    memset(&Timeout, 0, sizeof(Timeout));

    if(select(FD_SETSIZE, &ReadFDs, NULL, NULL, &Timeout) == 0)
    {
        /* nothing to receive */
        return SBN_IF_EMPTY;
    }/* end if */

#endif /* !SBN_RECV_TASK */

    int Received = recv(NetData->Socket, (char *)&RecvBuf,
        CFE_SB_MAX_SB_MSG_SIZE, 0);

    if(Received < 0)
    {
        return SBN_ERROR;
    }/* end if */

    /* each UDP packet is a full SBN message */

    SBN_UnpackMsg(&RecvBuf, MsgSizePtr, MsgTypePtr, CpuIDPtr, Payload);

    return SBN_SUCCESS;
}/* end SBN_UDP_Recv */

int SBN_UDP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_UDP_ReportModuleStatus */

int SBN_UDP_ResetPeer(SBN_PeerInterface_t *Peer)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_UDP_ResetPeer */
