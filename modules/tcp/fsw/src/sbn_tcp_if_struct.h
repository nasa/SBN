#ifndef _sbn_tcp_if_struct_h_
#define _sbn_tcp_if_struct_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "sbn_platform_cfg.h"
#include "cfe.h"

#define SBN_TCP_ITEMS_PER_FILE_LINE 3

typedef struct /* stashed in the SBN InterfacePvt buffer */
{
    int NetworkNumber;
    int PeerNumber;
    char Host[16];
    int  Port;
} SBN_TCP_Entry_t;

typedef struct
{
    uint8 ConnectedFlag;
#ifdef OS_NET_IMPL
    int NetID;
#else /* !OS_NET_IMPL */
    int Socket;
#endif /* OS_NET_IMPL */
    SBN_TCP_Entry_t *EntryPtr;
} SBN_TCP_Host_t;

typedef struct
{
    SBN_TCP_Entry_t *EntryPtr;
#ifdef OS_NET_IMPL
    int NetID;
#else /* !OS_NET_IMPL */
    int Socket;
#endif /* OS_NET_IMPL */
    uint8 /** flags */
            /** \brief recv the header first */
            ReceivingBody,
            /** \brief Do I connect to this peer or do they connect to me? */
            ConnectOut,
            /** \brief Is this peer currently connected? */
            Connected;
    SBN_PackedMsg_t RecvBuf;
    int RecvSize;
    OS_time_t LastConnectTry;
} SBN_TCP_Peer_t;

typedef struct
{
    SBN_TCP_Host_t Host;
    SBN_TCP_Peer_t Peers[SBN_MAX_NETWORK_PEERS];
    SBN_PackedMsg_t SendBuf;
    int PeerCount;
} SBN_TCP_Network_t;

typedef struct
{
    SBN_TCP_Network_t Networks[SBN_MAX_NETWORK_PEERS];
    int NetworkCount;
} SBN_TCP_ModuleData_t;

SBN_TCP_ModuleData_t SBN_TCP_ModuleData;
int SBN_TCP_ModuleDataInitialized = 0;

#endif /* _sbn_tcp_if_struct_h_ */
