#ifndef _sbn_tcp_if_struct_h_
#define _sbn_tcp_if_struct_h_

#include "sbn_constants.h"
#include "sbn_interfaces.h"
#include "sbn_platform_cfg.h"
#include "cfe.h"

#define SBN_TCP_ITEMS_PER_FILE_LINE 2

typedef struct
{
    char Host[16];
    int Port;
    uint8 BufNum;
    int Socket;
} SBN_TCP_Net_t;

typedef struct
{
    char Host[16];
    int  Port;
    int Socket;
    /* 0 = this peer connects to me, 1 = I connect to this peer */
    uint8 /** flags */
            /** \brief recv the header first */
            ReceivingBody,
            /** \brief Do I connect to this peer or do they connect to me? */
            ConnectOut,
            /** \brief Is this peer currently connected? */
            Connected,
            BufNum;
    OS_time_t LastConnectTry;
    int RecvSize;
} SBN_TCP_Peer_t;

#endif /* _sbn_tcp_if_struct_h_ */
