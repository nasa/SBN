#include "sbn_udp_if_struct.h"
#include "sbn_udp_if.h"
#include "sbn_udp_events.h"
#include "cfe.h"
#include <network_includes.h>
#include <string.h>
#include <errno.h>

static void ClearSocket(int SockId)
{
    struct sockaddr_in  s_addr;
    socklen_t           addr_len = 0;
    int                 i = 0;
    int                 status = 0;
    char                DiscardData[SBN_MAX_MSG_SIZE];

    addr_len = sizeof(s_addr);
    CFE_PSP_MemSet(&s_addr, 0, sizeof(s_addr));

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

    entry->NetworkNumber = atoi(row[0]);
    if(entry->NetworkNumber < 0
        || entry->NetworkNumber >= SBN_MAX_NETWORK_PEERS)
    {
        return SBN_ERROR;
    }/* end if */
 
    entry->Addr = inet_addr(row[1]);
    entry->Port = atoi(row[2]);

    return SBN_OK;
}/* end SBN_UDP_LoadEntry */

#else /* ! _osapi_confloader_ */

int SBN_UDP_ParseFileEntry(char *FileEntry, uint32 LineNum, void *entryptr)
{
    SBN_UDP_Entry_t *entry = (SBN_UDP_Entry_t *)entryptr;

    char Addr[16];
    int ScanfStatus = 0, NetworkNumber = 0, Port = 0;

    /*
     * Using sscanf to parse the string.
     * Currently no error handling
     */
    ScanfStatus = sscanf(FileEntry, "%d %s %d", &NetworkNumber, Addr, &Port);

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

    if(NetworkNumber < 0
        || NetworkNumber >= SBN_MAX_NETWORK_PEERS)
    {
        return SBN_ERROR;
    }/* end if */

 
    entry->Addr = inet_addr(Addr);
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
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)Data->InterfacePvt;
    SBN_UDP_Network_t *Network
        = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];

    if(Data->ProcessorId == CFE_CPU_ID)
    {
        /* CPU id match - this is host data.
         * Create msg interface when we find entry matching its own name
         * because the self entry has port info needed to bind this interface.
         */
        /* create, fill, and store an UDP-specific host data structure */
        SBN_UDP_Network_t *Network = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];

        Network->Host.EntryPtr = Entry;

        static struct sockaddr_in my_addr;
        Network->Host.Socket = 0;

        CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_DEBUG,
            "Creating socket for 0x%04X:%d",
            Entry->Addr, Entry->Port);

        if((Network->Host.Socket
            = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
                "%s:socket call failed,line %d,rtn val %d,errno=%d",
                CFE_CPU_NAME, __LINE__, Network->Host.Socket, errno);
            return SBN_ERROR;
        }/* end if */

        my_addr.sin_addr.s_addr = Entry->Addr;
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(Entry->Port);

        if(bind(Network->Host.Socket, (struct sockaddr *) &my_addr,
            sizeof(my_addr)) < 0)
        {
            CFE_EVS_SendEvent(SBN_UDP_SOCK_EID, CFE_EVS_ERROR,
                "%s:bind call failed,line %d,rtn val %d,errno=%d",
                CFE_CPU_NAME, __LINE__, Network->Host.Socket, errno);
            return SBN_ERROR;
        }/* end if */

        #ifdef _HAVE_FCNTL_
            /*
            ** Set the socket to non-blocking
            ** This is not available to vxWorks, so it has to be
            ** Conditionally compiled in
            */
            fcntl(Network->Host.Socket, F_SETFL, O_NONBLOCK);
        #endif

        ClearSocket(Network->Host.Socket);

        return SBN_HOST;
    }

    /* not me, it's a peer */
    Entry->PeerNumber = Network->PeerCount++;
    Network->Peers[Entry->PeerNumber].EntryPtr = Entry;

    return SBN_PEER;
}/* end SBN_UDP_Init */

#ifndef LITTLE_ENDIAN

/**
 * Utility function to copy memory and simultaneously swapping bytes for 
 * a little-endian platform. The SBN over-the-wire protocol is network order
 * (big-endian).
 * 
 * @param[in] dest Pointer to the destination block in memory.
 * @param[in] src Pointer to the source block in memory.
 * @param[in] n The number of bytes to copy from the src to the dest.
 *
 * @return CFE_PSP_SUCCESS on successful copy.
 */
static int32 EndianMemCpy(void *dest, void *src, uint32 n)
{
    uint32 i = 0;
    for(i = 0; i < n; i++)
    {   
        ((uint8 *)dest)[i] = ((uint8 *)src)[n - i - 1];
    }/* end for */
    return CFE_PSP_SUCCESS;
}/* end EndianMemCpy */

#else /* !LITTLE_ENDIAN */

#define EndianMemCpy(D, S, N) CFE_PSP_MemCpy(D, S, N)

#endif /* LITTLE_ENDIAN */

int SBN_UDP_Send(SBN_InterfaceData *PeerInterface, SBN_MsgType_t MsgType,
    SBN_MsgSize_t MsgSize, void *Msg)
{
    static struct sockaddr_in s_addr;
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)PeerInterface->InterfacePvt;
    SBN_UDP_Network_t *Network
        = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];
    SBN_UDP_Peer_t *Peer = &Network->Peers[Entry->PeerNumber];

    CFE_PSP_MemSet(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = Peer->EntryPtr->Addr;
    s_addr.sin_port = htons(Peer->EntryPtr->Port);

    SBN_PackMsg(Network->SendBuf, MsgSize, MsgType, CFE_CPU_ID, Msg);

    sendto(Network->Host.Socket, Network->SendBuf,
        MsgSize + SBN_PACKED_HDR_SIZE, 0,
        (struct sockaddr *) &s_addr, sizeof(s_addr));

    return SBN_OK;
}/* end SBN_UDP_Send */

/* Note that this Recv function is indescriminate, packets will be received
 * from all peers but that's ok, I just inject them into the SB and all is
 * good!
 */
int SBN_UDP_Recv(SBN_InterfaceData *Data, SBN_MsgType_t *MsgTypePtr,
    SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr, void *MsgBuf)
{
    SBN_UDP_Entry_t *Entry = (SBN_UDP_Entry_t *)Data->InterfacePvt;
    SBN_UDP_Network_t *Network
        = &SBN_UDP_ModuleData.Networks[Entry->NetworkNumber];

    int Received = 0;

    Received = recv(Network->Host.Socket, (char *)Network->RecvBuf,
        SBN_MAX_MSG_SIZE, MSG_DONTWAIT);

    if(Received == 0)
    {
        return SBN_IF_EMPTY;
    }/* end if */

    if(Received < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            /* the socket is set to O_NONBLOCK, so these are valid return
             *  values when there are no packets to recv.
             */
            return SBN_IF_EMPTY;
        }/* end if */

        return SBN_ERROR;
    }/* end if */

    /* each UDP packet is a full SBN message */

    SBN_UnpackMsg(Network->RecvBuf, MsgSizePtr, MsgTypePtr, CpuIdPtr, MsgBuf);

    return SBN_OK;
}/* end SBN_UDP_Recv */

int SBN_UDP_VerifyPeerInterface(SBN_InterfaceData *Peer,
        SBN_InterfaceData *HostList[], int NumHosts)
{
    return TRUE;
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
