#ifndef _sbn_udp_if_struct_h_
#define _sbn_udp_if_struct_h_

#define SBN_UDP_ITEMS_PER_FILE_LINE 2

typedef struct
{
    char Addr[16];
    int  Port;
} SBN_UDP_Entry_t;

typedef struct
{
    SBN_UDP_Entry_t Entry;
    int SockId;
} SBN_UDP_HostData_t;

typedef struct
{
    SBN_UDP_Entry_t Entry;
} SBN_UDP_PeerData_t;

#endif /* _sbn_udp_if_struct_h_ */
