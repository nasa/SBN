#ifndef _sbn_tcp_if_struct_h_
#define _sbn_tcp_if_struct_h_

#include "sbn_types.h"
#include "sbn_interfaces.h"
#include "sbn_platform_cfg.h"
#include "cfe.h"

/**
 * If I haven't sent a message in SBN_TCP_PEER_HEARTBEAT seconds, send an empty
 * one just to maintain the connection. If this is set to 0, no heartbeat
 * messages will be generated.
 */
#define SBN_TCP_PEER_HEARTBEAT 5
/* #define SBN_TCP_PEER_HEARTBEAT 0 */

/**
 * If I haven't received a message from a peer in SBN_TCP_PEER_TIMEOUT seconds,
 * consider the peer lost and disconnect. If this is set to 0, no timeout is
 * checked.
 */
/* #define SBN_TCP_PEER_TIMEOUT 10 */
#define SBN_TCP_PEER_TIMEOUT 0

typedef struct
{
    bool InUse, ReceivingBody;
    int RecvSz;
    int Socket;
    uint8 BufNum;
    SBN_PeerInterface_t *PeerInterface; /* affiliated peer, if known */
} SBN_TCP_Conn_t;

typedef struct
{
    OS_SockAddr_t Addr;
    bool ConnectOut;
    uint8 BufNum; /* incoming buffer */
    OS_time_t LastConnectTry;
    SBN_TCP_Conn_t *Conn; /* when connected and affiliated */
} SBN_TCP_Peer_t;

typedef struct
{
    OS_SockAddr_t Addr;
    uint8 BufNum; /* outgoing buffer */
    int Socket; /* server socket */
    SBN_TCP_Conn_t Conns[SBN_MAX_PEER_CNT];
} SBN_TCP_Net_t;

#endif /* _sbn_tcp_if_struct_h_ */
