#include "sbn_udp_if_struct.h"
#include "sbn_udp_if.h"
#include "sbn_udp_events.h"
#include "cfe.h"
#include "sbn_lib_utils.h"
#include <network_includes.h>
#include <string.h>
#include <strings.h> /* for bzero */
#include <errno.h>

static void ClearSocket(int SockId)
{
    struct sockaddr_in  s_addr;
    socklen_t           addr_len = 0;
    int                 i = 0;
    int                 status = 0;
    char                DiscardData[SBN_MAX_MSG_SIZE];

    addr_len = sizeof(s_addr);
    bzero((char *) &s_addr, sizeof(s_addr));

    CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_DEBUG,
        "Clearing socket %d", SockId);

    /* change to while loop */
    for(i = 0; i <= 50; i++)
    {
        status = recvfrom(SockId, DiscardData, sizeof(DiscardData),
            MSG_DONTWAIT,(struct sockaddr *) &s_addr, &addr_len);
        if((status < 0) && (errno == EWOULDBLOCK)) // TODO: add EAGAIN?
            break; /* no (more) messages */
    }/* end for */
}/* end ClearSocket */

#ifdef _osapi_confloader_

int SBN_UDP_LoadEntry(const char **row, int fieldcount, void *entryptr)
{
    SBN_UDP_Entry_t *entry = (SBN_UDP_Entry_t *)entryptr;
    if(fieldcount < SBN_UDP_ITEMS_PER_FILE_LINE)
    {
        return SBN_ERROR;
    }/* end if */

    strncpy(entry->Addr, row[0], 16);
    entry->Port = atoi(row[1]);

    return SBN_OK;
}/* end SBN_UDP_LoadEntry */

#else /* ! _osapi_confloader_ */

int SBN_UDP_ParseFileEntry(char *FileEntry, uint32 LineNum, void *entryptr)
{
    SBN_UDP_Entry_t *entry = (SBN_UDP_Entry_t *)entryptr;

    int     ScanfStatus = 0;
    char    Addr[16];
    int     Port = 0;

    /*
     * Using sscanf to parse the string.
     * Currently no error handling
     */
    ScanfStatus = sscanf(FileEntry, "%s %d", Addr, &Port);

    /*
     * Check to see if the correct number of items were parsed
     */
    if(ScanfStatus != SBN_UDP_ITEMS_PER_FILE_LINE)
    {
        CFE_EVS_SendEvent(SBN_UDP_CONFIG_EID,CFE_EVS_ERROR,
                "%s:Invalid SBN peer file line,exp %d items,found %d",
                CFE_CPU_NAME, SBN_UDP_ITEMS_PER_FILE_LINE, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    strncpy(entry->Addr, Addr, 16);
    entry->Port = Port;

    return SBN_OK;
}/* end SBN_UDP_ParseFileEntry */

#endif /* _osapi_confloader_ */

/**
 * Initializes an UDP host or peer data struct depending on the
 * CPU name.
 *
 * @param  Interface data structure containing the file entry
 * @return SBN_OK on success, error code otherwise
 */
int SBN_UDP_Init(SBN_InterfaceData *Data)
{
    if(Data->ProcessorId == CFE_CPU_ID)
    {
        /* CPU names match - this is host data.
         * Create msg interface when we find entry matching its own name
         * because the self entry has port info needed to bind this interface.
         */
        /* create, fill, and store an UDP-specific host data structure */
        SBN_UDP_HostData_t *Host = (SBN_UDP_HostData_t *)Data->InterfacePvt;

        static struct sockaddr_in my_addr;
        Host->SockId = 0;

        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_DEBUG,
            "Creating socket for %s:%d", Host->Entry.Addr, Host->Entry.Port);

        if((Host->SockId = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
                "%s:socket call failed,line %d,rtn val %d,errno=%d",
                CFE_CPU_NAME, __LINE__, Host->SockId, errno);
            return SBN_ERROR;
        }/* end if */

        my_addr.sin_addr.s_addr = inet_addr(Host->Entry.Addr);
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(Host->Entry.Port);

        if(bind(Host->SockId, (struct sockaddr *) &my_addr,
            sizeof(my_addr)) < 0)
        {
            CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
                "%s:bind call failed,line %d,rtn val %d,errno=%d",
                CFE_CPU_NAME, __LINE__, Host->SockId, errno);
            return SBN_ERROR;
        }/* end if */

        #ifdef _HAVE_FCNTL_
            /*
            ** Set the socket to non-blocking
            ** This is not available to vxWorks, so it has to be
            ** Conditionally compiled in
            */
            fcntl(Host->SockId, F_SETFL, O_NONBLOCK);
        #endif

        ClearSocket(Host->SockId);

        return SBN_HOST;
    }
    else
    {
        /* peer has nothing beyond Entry */
        return SBN_PEER;
    }/* end if */
}/* end SBN_UDP_Init */

int SBN_UDP_Send(SBN_InterfaceData *HostList[], int NumHosts,
    SBN_InterfaceData *IfData, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, void *Msg)
{
    static struct sockaddr_in s_addr;
    SBN_UDP_PeerData_t *Peer = NULL;
    SBN_UDP_HostData_t *Host = NULL;
    uint32 HostIdx = 0;
    uint8 Buf[SBN_MAX_MSG_SIZE]; /* it all needs to go into one packet */
    void *BufOffset = Buf;
    SBN_CpuId_t CpuId = CFE_CPU_ID;

    /* Find the host that goes with this peer.  There should only be one
       ethernet host */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++)
    {
        if(HostList[HostIdx]->ProtocolId == SBN_UDP)
        {
            Host = (SBN_UDP_HostData_t *)HostList[HostIdx]->InterfacePvt;
        }/* end if */
    }/* end for */

    if(!Host)
    {
        OS_printf("No UDP Host Found!\n");
        return SBN_ERROR;
    }/* end if */

    Peer = (SBN_UDP_PeerData_t *)IfData->InterfacePvt;
    bzero((char *) &s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(Peer->Entry.Addr);
    s_addr.sin_port = htons(Peer->Entry.Port);

    SBN_EndianMemCpy(BufOffset, &MsgSize, sizeof(MsgSize));
    BufOffset += sizeof(MsgSize);
    CFE_PSP_MemCpy(BufOffset, &MsgType, sizeof(MsgType));
    BufOffset += sizeof(MsgType);
    SBN_EndianMemCpy(BufOffset, &CpuId, sizeof(CpuId));
    BufOffset += sizeof(CpuId);

    if(Msg && MsgSize)
    {
        CFE_PSP_MemCpy(BufOffset, Msg, MsgSize);
    }/* end if */

    sendto(Host->SockId, Buf, MsgSize + SBN_PACKED_HDR_SIZE, 0,
        (struct sockaddr *) &s_addr, sizeof(s_addr));

    return SBN_OK;
}/* end SBN_UDP_Send */

int SBN_UDP_Recv(SBN_InterfaceData *Data, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, void *MsgBuf)
{
    ssize_t             Received = 0, TotalReceived = 0;
    struct sockaddr_in  s_addr;
    socklen_t           addr_len = sizeof(s_addr);
    SBN_UDP_HostData_t  *Host = (SBN_UDP_HostData_t *)Data->InterfacePvt;
    void *MsgBufPtr = MsgBuf;

    bzero((char *) &s_addr, sizeof(s_addr));
    *MsgSizePtr = 0;

    while(1)
    {
        addr_len = sizeof(s_addr);
        Received = recvfrom(Host->SockId, (char *)MsgBuf + TotalReceived,
            SBN_MAX_MSG_SIZE - TotalReceived,
            MSG_DONTWAIT, (struct sockaddr *) &s_addr, &addr_len);

        if(Received == 0) return SBN_IF_EMPTY;

        if((Received < 0) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)))
            return SBN_ERROR;

        TotalReceived += Received;
        if(TotalReceived < sizeof(SBN_MsgSize_t))
        {
            continue;
        }/* end if */

        if(MsgBufPtr == MsgBuf) /* we haven't read the size yet */
        {
            SBN_EndianMemCpy(MsgSizePtr, MsgBufPtr, sizeof(*MsgSizePtr));
            MsgBufPtr += sizeof(*MsgSizePtr);
        }/* end if */

        if(TotalReceived >= SBN_PACKED_HDR_SIZE + *MsgSizePtr)
        {
            break;
        }/* end if */
    }/* end while */

    CFE_PSP_MemCpy(MsgTypePtr, MsgBufPtr, sizeof(*MsgTypePtr));
    MsgBufPtr += sizeof(*MsgTypePtr);

    SBN_EndianMemCpy(CpuIdPtr, MsgBufPtr, sizeof(*CpuIdPtr));
    MsgBufPtr += sizeof(*CpuIdPtr);

    if(TotalReceived > SBN_PACKED_HDR_SIZE)
    {
        memmove(MsgBuf, MsgBufPtr, TotalReceived - SBN_PACKED_HDR_SIZE);
    }/* end if */

    return SBN_OK;
}/* end SBN_UDP_Recv */

int SBN_UDP_VerifyPeerInterface(SBN_InterfaceData *Peer,
        SBN_InterfaceData *HostList[], int NumHosts)
{
    int     HostIdx = 0;

    /* Find the host that goes with this peer.  There should only be one
       ethernet host */
    for(HostIdx = 0; HostIdx < NumHosts; HostIdx++)
    {
        if(HostList[HostIdx]->ProtocolId == SBN_UDP)
        {
            return TRUE;
        }/* end if */
    }/* end for */

    return FALSE;
}/* end SBN_UDP_VerifyPeerInterface */

int SBN_UDP_VerifyHostInterface(SBN_InterfaceData *Host,
        SBN_PeerData_t *PeerList, int NumPeers)
{
    return TRUE;
}/* end SBN_UDP_VerifyHostInterface */

int SBN_UDP_ReportModuleStatus(SBN_ModuleStatusPacket_t *Packet,
        SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[], int NumHosts)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_UDP_ReportModuleStatus */

int SBN_UDP_ResetPeer(SBN_InterfaceData *Peer, SBN_InterfaceData *HostList[],
        int NumHosts)
{
    return SBN_NOT_IMPLEMENTED;
}/* end SBN_UDP_ResetPeer */
