#ifndef _sbn_tcp_if_struct_h_
#define _sbn_tcp_if_struct_h_

#include "sbn_constants.h"
#include "sbn_platform_cfg.h"
#include "cfe.h"

#define SBN_TCP_ITEMS_PER_FILE_LINE 3

typedef struct /* stashed in the SBN InterfacePvt buffer */
{
    int NetworkNumber;
    int PeerNumber;
    uint32 Addr;
    int  Port;
} SBN_TCP_Entry_t;

typedef struct
{
    int Socket;
    SBN_TCP_Entry_t *EntryPtr;
} SBN_TCP_Host_t;

typedef struct
{
    SBN_TCP_Entry_t *EntryPtr;
    int Socket;
    char RecvBuf[SBN_MAX_MSG_SIZE];
    int RecvSize;
    int ConnectOut;
    OS_time_t LastConnectTry;
} SBN_TCP_Peer_t;

typedef struct
{
    SBN_TCP_Host_t Host;
    SBN_TCP_Peer_t Peers[SBN_MAX_NETWORK_PEERS];
    char SendBuf[SBN_MAX_MSG_SIZE];
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
