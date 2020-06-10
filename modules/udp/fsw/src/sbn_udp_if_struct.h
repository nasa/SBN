#ifndef _sbn_udp_if_struct_h_
#define _sbn_udp_if_struct_h_

#include "sbn_types.h"
#include "sbn_interfaces.h"
#include "sbn_platform_cfg.h"
#include "cfe.h"

/**
 * \brief Number of seconds since last I've sent the peer a message when
 * I send an empty heartbeat message.
 */
#define SBN_UDP_PEER_HEARTBEAT 5

/**
 * \brief Number of seconds since I've last heard from the peer when I consider
 * the peer connection to be dropped.
 */
#define SBN_UDP_PEER_TIMEOUT 10

/**
 * \brief If we're not connected, send peer occasional messages to wake
 * them up and tell them "I'm here".
 */
#define SBN_UDP_ANNOUNCE_TIMEOUT 10

typedef struct
{
    OS_SockAddr_t Addr;
} SBN_UDP_Peer_t;

typedef struct
{
    OS_SockAddr_t Addr;
    uint32 Socket;
} SBN_UDP_Net_t;

#endif /* _sbn_udp_if_struct_h_ */
