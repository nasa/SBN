#ifndef _sbn_udp_if_struct_h_
#define _sbn_udp_if_struct_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "sbn_platform_cfg.h"
#include "cfe.h"

#define SBN_UDP_ITEMS_PER_FILE_LINE 2

typedef struct
{
    char Host[16];
    int  Port;
} SBN_UDP_Peer_t;

typedef struct
{
    char Host[16];
    int Port, Socket;
} SBN_UDP_Net_t;

#endif /* _sbn_udp_if_struct_h_ */
