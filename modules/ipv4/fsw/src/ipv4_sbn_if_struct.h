#ifndef _ip_sbn_if_struct_h_
#define _ip_sbn_if_struct_h_

#define IPV4_ITEMS_PER_FILE_LINE 3

typedef struct {
    char Addr[16];
    int  DataPort;
    int  ProtoPort;
} IPv4_SBNEntry_t;

typedef struct {
    int                ProtoSockId;
    int                ProtoRcvPort;
    int                ProtoXmtPort;
    int                DataSockId;
    int                DataRcvPort;
} IPv4_SBNHostData_t;

typedef struct {
    char            Addr[16];
    int             DataPort;
    int             ProtoSockId;
    int             ProtoRcvPort;
} IPv4_SBNPeerData_t;

#endif /* _ipv4_sbn_if_struct_h_ */
