#ifndef _SBN_UDP_IF_H_
#define _SBN_UDP_IF_H_

#include "sbn_udp_events.h"
#include "sbn_platform_cfg.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>

#include "sbn_interfaces.h"
#include "cfe.h"

/**
 * UDP-specific message types.
 */
#define SBN_UDP_HEARTBEAT_MSG 0xA0
#define SBN_UDP_ANNOUNCE_MSG  0xA1
#define SBN_UDP_DISCONN_MSG   0xA2

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
    uint32        Socket;
} SBN_UDP_Net_t;

#endif /* _SBN_UDP_IF_H_ */
