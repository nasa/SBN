/******************************************************************************
 * @file
** File: sbn_netif.c
**
**      Copyright (c) 2004-2006, United States government as represented by the
**      administrator of the National Aeronautics Space Administration.
**      All rights reserved. This software(cFE) was created at NASA's Goddard
**      Space Flight Center pursuant to government contracts.
**
**      This software may be used only pursuant to a United States government
**      sponsored project and the United States government may not be charged
**      for use thereof.
**
** Purpose:
**      This file contains source code for the Software Bus Network Application.
**
** Authors:   J. Wilmot/GSFC Code582
**            R. McGraw/SSI
**
** $Log: sbn_app.c  $
** Revision 1.4 2010/10/05 15:24:12EDT jmdagost
**
******************************************************************************/

/*
** Include Files
*/
#include "cfe.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"
#include "sbn_app.h"
#include "sbn_netif.h"
#include "sbn_main_events.h"

#include <network_includes.h>
#include <string.h>
#include <errno.h>

#ifdef _osapi_confloader_
static int PeerFileRowCallback(const char *filename, int linenum,
    const char *header, const char *row[], int fieldcount, void *opaque)
{
    int ProtocolId = 0, status = 0;

    if(fieldcount < 4)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "too few fields (fieldcount=%d)", fieldcount);
        return OS_SUCCESS;
    }/* end if */

    if(SBN.NumEntries >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "max entry count reached, skipping");
        return OS_ERROR;
    }/* end if */

    ProtocolId = atoi(row[2]);
    if(ProtocolId < 0 || ProtocolId > SBN_MAX_INTERFACE_TYPES
        || !SBN.IfOps[ProtocolId])
    {   
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid protocol id (ProtocolId=%d)",
            ProtocolId);
        return OS_SUCCESS;
    }/* end if */

    /* TODO: check fields to ensure they are integer and within valid value
        ranges */

    /* Copy over the general info into the interface data structure */
    strncpy(SBN.IfData[SBN.NumEntries].Name, row[0], SBN_MAX_PEERNAME_LENGTH);
    SBN.IfData[SBN.NumEntries].QoS = atoi(row[4]);
    SBN.IfData[SBN.NumEntries].ProtocolId = ProtocolId;
    SBN.IfData[SBN.NumEntries].ProcessorId = atoi(row[1]);
    SBN.IfData[SBN.NumEntries].SpaceCraftId = atoi(row[3]);

    /* Call the correct parse entry function based on the interface type */
    status = SBN.IfOps[ProtocolId]->LoadInterfaceEntry(row + 5, fieldcount - 5,
        &SBN.IfData[SBN.NumEntries].InterfacePvt);

    if(status == SBN_OK)
    {   
        SBN.NumEntries++;
    }/* end if */

    return OS_SUCCESS;
}/* end PeerFileRowCallback */

static int PeerFileErrCallback(const char *filename, int linenum,
    const char *err, void *opaque)
{
    return OS_SUCCESS;
}/* end PeerFileErrCallback */

int32 SBN_GetPeerFileData(void)
{
    int32 status = 0, id = 0;

    status = OS_ConfLoaderAPIInit();
    if(status != OS_SUCCESS)
    {   
        return status;
    }/* end if */

    status = OS_ConfLoaderInit(&id, "sbn_peer");
    if(status != OS_SUCCESS)
    {   
        return status;
    }/* end if */

    status = OS_ConfLoaderSetRowCallback(id, PeerFileRowCallback, NULL);
    if(status != OS_SUCCESS)
    {
        return status;
    }/* end if */

    status = OS_ConfLoaderSetErrorCallback(id, PeerFileErrCallback, NULL);
    if(status != OS_SUCCESS)
    {
        return status;
    }/* end if */

    OS_ConfLoaderLoad(id, SBN_VOL_PEER_FILENAME);
    OS_ConfLoaderLoad(id, SBN_NONVOL_PEER_FILENAME);

    return SBN_OK;
}/* end SBN_GetPeerFileData */

#else /* ! _osapi_confloader_ */

static char *SBN_FindFileEntryAppData(char *entry, int num_fields)
{
    char *char_ptr = entry;
    int num_found_fields = 0;

    DEBUG_START();

    while(*char_ptr != '\0' && num_found_fields < num_fields)
    {
        while(*char_ptr != ' ')
        {
            ++char_ptr;
        }/* end while */
        ++char_ptr;
        ++num_found_fields;
    }/* end while */
    return char_ptr;
}/* end SBN_FindFileEntryAppData */

static int ParseFileEntry(char *FileEntry)
{
    char Name[SBN_MAX_PEERNAME_LENGTH];
    char  *app_data = NULL;
    uint32 ProcessorId = 0;
    int ProtocolId = 0;
    uint32 SpaceCraftId = 0, QoS = 0;
    int ScanfStatus = 0, status = 0, NumFields = 5, ProcessorIdInt = 0,
        ProtocolIdInt = 0, SpaceCraftIdInt = 0, QoSInt = 0;

    DEBUG_START();

    app_data = SBN_FindFileEntryAppData(FileEntry, NumFields);

    /* switch on protocol ID */
    ScanfStatus = sscanf(FileEntry, "%s %d %d %d %d" ,
        Name,
        &ProcessorIdInt, &ProtocolIdInt, &SpaceCraftIdInt, &QoSInt);
    ProcessorId = ProcessorIdInt;
    ProtocolId = ProtocolIdInt;
    SpaceCraftId = SpaceCraftIdInt;
    QoS = QoSInt;

    /*
    ** Check to see if the correct number of items were parsed
    */
    if(ScanfStatus != NumFields)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "invalid peer file line (expected %d, found %d)",
            NumFields, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    if(SBN.NumEntries >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "max peers exceeded (max=%d this=%d)",
            SBN_MAX_NETWORK_PEERS, SBN.NumEntries);
        return SBN_ERROR;
    }/* end if */

    if(ProtocolId < 0 || ProtocolId > SBN_MAX_INTERFACE_TYPES
        || !SBN.IfOps[ProtocolId])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid protocol ID (%d)", ProtocolId);
        return SBN_ERROR;
    }

    /* Copy over the general info into the interface data structure */
    strncpy(SBN.IfData[SBN.NumEntries].Name, Name, SBN_MAX_PEERNAME_LENGTH);
    SBN.IfData[SBN.NumEntries].QoS = QoS;
    SBN.IfData[SBN.NumEntries].ProtocolId = ProtocolId;
    SBN.IfData[SBN.NumEntries].ProcessorId = ProcessorId;
    SBN.IfData[SBN.NumEntries].SpaceCraftId = SpaceCraftId;

    /* Call the correct parse entry function based on the interface type */
    status = SBN.IfOps[ProtocolId]->ParseInterfaceFileEntry(app_data, SBN.NumEntries, &SBN.IfData[SBN.NumEntries].InterfacePvt);

    if(status == SBN_OK)
    {
        SBN.NumEntries++;
    }/* end if */

    return status;
}/* end ParseFileEntry */

int32 SBN_GetPeerFileData(void)
{
    static char     SBN_PeerData[SBN_PEER_FILE_LINE_SIZE];
    int             BuffLen = 0; /* Length of the current buffer */
    int             PeerFile = 0;
    char            c = '\0';
    int             FileOpened = FALSE;
    int             LineNum = 0;

    DEBUG_START();

    /* First check for the file in RAM */
    PeerFile = OS_open(SBN_VOL_PEER_FILENAME, O_RDONLY, 0);
    if(PeerFile != OS_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
            "opened peer data file '%s'", SBN_VOL_PEER_FILENAME);
        FileOpened = TRUE;
    }
    else
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "failed to open peer file '%s'", SBN_VOL_PEER_FILENAME);
        FileOpened = FALSE;
    }/* end if */

    /* If ram file failed to open, try to open non vol file */
    if(!FileOpened)
    {
        PeerFile = OS_open(SBN_NONVOL_PEER_FILENAME, O_RDONLY, 0);

        if(PeerFile != OS_ERROR)
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
                "opened peer data file '%s'", SBN_NONVOL_PEER_FILENAME);
            FileOpened = TRUE;
        }
        else
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
                "peer file '%s' failed to open", SBN_NONVOL_PEER_FILENAME);
            FileOpened = FALSE;
        }/* end if */
    }/* end if */

    /*
     ** If no file was opened, SBN must terminate
     */
    if(!FileOpened)
    {
        return SBN_ERROR;
    }/* end if */

    CFE_PSP_MemSet(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
    BuffLen = 0;

    /*
     ** Parse the lines from the file
     */

    while(1)
    {
        OS_read(PeerFile, &c, 1);

        if(c == '!')
        {
            break;
        }/* end if */

        if(c == '\n' || c == ' ' || c == '\t')
        {
            /*
             ** Skip all white space in the file
             */
            ;
        }
        else if(c == ',')
        {
            /*
             ** replace the field delimiter with a space
             ** This is used for the sscanf string parsing
             */
            SBN_PeerData[BuffLen] = ' ';
            if(BuffLen < (SBN_PEER_FILE_LINE_SIZE - 1))
                BuffLen++;
        }
        else if(c != ';')
        {
            /*
             ** Regular data gets copied in
             */
            SBN_PeerData[BuffLen] = c;
            if(BuffLen < (SBN_PEER_FILE_LINE_SIZE - 1))
                BuffLen++;
        }
        else
        {
            /*
             ** Send the line to the file parser
             */
            if(ParseFileEntry(SBN_PeerData) == SBN_ERROR)
            {
                OS_close(PeerFile);
                return SBN_ERROR;
            }/* end if */
            LineNum++;
            CFE_PSP_MemSet(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
            BuffLen = 0;
        }/* end if */
    }/* end while */

    OS_close(PeerFile);

    return SBN_OK;
}/* end SBN_GetPeerFileData */

#endif /* _osapi_confloader_ */

#ifdef SOFTWARE_BIG_BIT_ORDER
  #define CFE_MAKE_BIG32(n) (n)
#else /* !SOFTWARE_BIG_BIT_ORDER */
  #define CFE_MAKE_BIG32(n) ( (((n) << 24) & 0xFF000000) | (((n) << 8) & 0x00FF0000) | (((n) >> 8) & 0x0000FF00) | (((n) >> 24) & 0x000000FF) )
#endif /* SOFTWARE_BIG_BIT_ORDER */

/* \brief Utility function to swap headers.
 * \note Don't run this on a pointer given to us by the SB, as the SB may
 *       re-use the same buffer for multiple pipes.
 */
static void SwapCCSDS(CFE_SB_Msg_t *Msg)
{
    uint16 *Util16 = NULL; uint32 *Util32 = NULL;
    int CCSDSType = CCSDS_RD_TYPE(*((CCSDS_PriHdr_t *)Msg));
    if(CCSDSType == CCSDS_TLM)
    {   
        CCSDS_TlmPkt_t *TlmPktPtr = (CCSDS_TlmPkt_t *)Msg;
        /* SBN sends CCSDS telemetry messages with secondary headers in
         * big-endian order.
         */
        Util32 = (uint32 *)TlmPktPtr->SecHdr.Time;
        *Util32 = CFE_MAKE_BIG32(*Util32);
        if(CCSDS_TIME_SIZE == 6)
        {
            Util16 = (uint16 *)TlmPktPtr->SecHdr.Time + 4;
            *Util16 = CFE_MAKE_BIG16(*Util16);
        }
        else
        {
            Util32 = (uint32 *)(TlmPktPtr->SecHdr.Time + 4);
            *Util32 = CFE_MAKE_BIG32(*Util32);
        }/* end if */
    }
    else if(CCSDSType == CCSDS_CMD)
    {
        CCSDS_CmdPkt_t *CmdPktPtr = (CCSDS_CmdPkt_t *)Msg;

        Util16 = (uint16 *)&CmdPktPtr->SecHdr.Command;
        *Util16 = CFE_MAKE_BIG16(*Util16);
    /* else what? */
    }/* end if */
}/* end SwapCCSDS */

/**
 * \brief Packs a CCSDS message with an SBN message header.
 * \note Ensures the SBN fields (CPU ID, MsgSize) and CCSDS message headers
 *       are in network (big-endian) byte order.
 * \param SBNBuf[out] The buffer to pack into.
 * \param MsgType[in] The SBN message type.
 * \param MsgSize[in] The size of the payload.
 * \param CpuId[in] The CpuId of the sender (should be CFE_CPU_ID)
 * \param Msg[in] The payload (CCSDS message or SBN sub/unsub.)
 * \todo Use a type for SBNBuf.
 */
void SBN_PackMsg(void *SBNBuf, SBN_MsgSize_t MsgSize, SBN_MsgType_t MsgType,
    SBN_CpuId_t CpuId, void *Msg)
{
    char *BufOffset = SBNBuf;
    uint16 *Util16 = NULL;
    uint32 *Util32 = NULL;

    CFE_PSP_MemCpy(BufOffset, &MsgSize, sizeof(MsgSize));
    Util16 = (uint16 *)BufOffset; *Util16 = CFE_MAKE_BIG16(*Util16);
    BufOffset += sizeof(MsgSize);

    CFE_PSP_MemCpy(BufOffset, &MsgType, sizeof(MsgType));
    BufOffset += sizeof(MsgType);

    CFE_PSP_MemCpy(BufOffset, &CpuId, sizeof(CpuId));
    Util32 = (uint32 *)BufOffset; *Util32 = CFE_MAKE_BIG32(*Util32);
    BufOffset += sizeof(CpuId);

    if(!Msg || !MsgSize)
    {
        return;
    }/* end if */

    CFE_PSP_MemCpy(BufOffset, Msg, MsgSize);

    if(MsgType == SBN_APP_MSG)
    {
        SwapCCSDS((CFE_SB_Msg_t *)BufOffset);
    }/* end if */
}/* end SBN_PackMsg */

/**
 * \brief Unpacks a CCSDS message with an SBN message header.
 * \param SBNBuf[in] The buffer to unpack.
 * \param MsgTypePtr[out] The SBN message type.
 * \param MsgSizePtr[out] The payload size.
 * \param CpuId[out] The CpuId of the sender.
 * \param Msg[out] The payload (a CCSDS message, or SBN sub/unsub).
 * \note Ensures the SBN fields (CPU ID, MsgSize) and CCSDS message headers
 *       are in platform byte order.
 * \todo Use a type for SBNBuf.
 */
void SBN_UnpackMsg(void *SBNBuf, SBN_MsgSize_t *MsgSizePtr,
    SBN_MsgType_t *MsgTypePtr, SBN_CpuId_t *CpuIdPtr, void *Msg)
{
    char *BufOffset = SBNBuf;

    CFE_PSP_MemCpy(MsgSizePtr, BufOffset, sizeof(*MsgSizePtr));
    *MsgSizePtr = CFE_MAKE_BIG16(*MsgSizePtr);
    BufOffset += sizeof(*MsgSizePtr);

    CFE_PSP_MemCpy(MsgTypePtr, BufOffset, sizeof(*MsgTypePtr));
    BufOffset += sizeof(*MsgTypePtr);

    CFE_PSP_MemCpy(CpuIdPtr, BufOffset, sizeof(*CpuIdPtr));
    *CpuIdPtr = CFE_MAKE_BIG32(*CpuIdPtr);
    BufOffset += sizeof(*CpuIdPtr);

    if(!*MsgSizePtr)
    {
        return;
    }/* end if */

    CFE_PSP_MemCpy(Msg, BufOffset, *MsgSizePtr);

    if(*MsgTypePtr == SBN_APP_MSG)
    {
        SwapCCSDS((CFE_SB_Msg_t *)Msg);
    }/* end if */
}/* end SBN_UnpackMsg */

/**
 * Loops through all entries, categorizes them as "Host" or "Peer", and
 * initializes them according to their role and protocol ID.
 *
 * @return SBN_OK if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
int SBN_InitPeerInterface(void)
{
    int32 Stat = 0, IFRole = 0; /* host or peer */
    int i = 0, PeerIdx = 0;

    DEBUG_START();

    SBN.NumHosts = 0;
    SBN.NumPeers = 0;

    /* loop through entries in peer data */
    for(PeerIdx = 0; PeerIdx < SBN.NumEntries; PeerIdx++)
    {
        /* Call the correct init interface function based on the interface type */
        IFRole = SBN.IfOps[SBN.IfData[PeerIdx].ProtocolId]->InitPeerInterface(&SBN.IfData[PeerIdx]);
        if(IFRole == SBN_HOST)
        {
            SBN.Host[SBN.NumHosts] = &SBN.IfData[PeerIdx];
            SBN.NumHosts++;
        }
        else if(IFRole == SBN_PEER)
        {
            CFE_PSP_MemSet(&SBN.Peer[SBN.NumPeers], 0,
                sizeof(SBN.Peer[SBN.NumPeers]));
            SBN.Peer[SBN.NumPeers].IfData = &SBN.IfData[PeerIdx];
            SBN.Peer[SBN.NumPeers].InUse = TRUE;

            /* for ease of use, copy some data from the entry into the peer */
            SBN.Peer[SBN.NumPeers].QoS = SBN.IfData[PeerIdx].QoS;
            SBN.Peer[SBN.NumPeers].ProcessorId =
                SBN.IfData[PeerIdx].ProcessorId;
            SBN.Peer[SBN.NumPeers].ProtocolId =
                SBN.IfData[PeerIdx].ProtocolId;
            SBN.Peer[SBN.NumPeers].SpaceCraftId =
                SBN.IfData[PeerIdx].SpaceCraftId;
            strncpy(SBN.Peer[SBN.NumPeers].Name, SBN.IfData[PeerIdx].Name,
                SBN_MAX_PEERNAME_LENGTH);

            Stat = SBN_CreatePipe4Peer(SBN.NumPeers);
            if(Stat == SBN_ERROR)
            {
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                    "error creating pipe for %s (Stat=0x%x)",
                    SBN.Peer[SBN.NumPeers].Name,
                    (unsigned int)Stat);
                return SBN_ERROR;
            }/* end if */

            /* Initialize the subscriptions count for each entry */
            for(i = 0; i < SBN_MAX_SUBS_PER_PEER; i++)
            {
                SBN.Peer[SBN.NumPeers].Sub[i].InUseCtr = FALSE;
            }/* end for */

            /* Reset counters, flags and timers */
            SBN.Peer[SBN.NumPeers].State = SBN_ANNOUNCING;

            SBN.NumPeers++;
        }
        else
        {
            /* TODO - error */
        }/* end if */
    }/* end for */

    CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
        "configured, %d hosts and %d peers", SBN.NumHosts, SBN.NumPeers);
    return SBN_OK;
}/* end SBN_InitPeerInterface */

/**
 * Sends a message to a peer using the module's SendNetMsg.
 *
 * @param MsgType       SBN type of the message
 * @param MsgSize       Size of the message
 * @param Msg           Message to send
 * @param PeerIdx       Index of peer data in peer array
 * @return Number of characters sent on success, -1 on error.
 *
 */
int SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSize_t MsgSize, void *Msg,
     int PeerIdx)
{
    int status = 0;

    DEBUG_MSG("%s type=%04x size=%d", __FUNCTION__, MsgType,
        MsgSize);

    status = SBN.IfOps[SBN.Peer[PeerIdx].ProtocolId]->SendNetMsg(
        SBN.Host, SBN.NumHosts, SBN.Peer[PeerIdx].IfData,
        MsgType, MsgSize, Msg);

    if(status != -1)
    {
        OS_GetLocalTime(&SBN.Peer[PeerIdx].last_sent);
    }
    else
    {
        SBN.HkPkt.PeerHk[PeerIdx].SendErrCount++;
    }/* end if */

    return status;
}/* end SBN_SendNetMsg */

void SBN_ProcessNetAppMsgsFromHost(int HostIdx)
{
    int i = 0, status = 0, PeerIdx = 0;

    /* DEBUG_START(); chatty */

    /* Process all the received messages */
    for(i = 0; i <= 100; i++)
    {
        SBN_CpuId_t CpuId = 0;
        SBN_MsgType_t MsgType = 0;
        SBN_MsgSize_t MsgSize = 0;
        uint8 Msg[SBN_MAX_MSG_SIZE];

        status = SBN.IfOps[SBN.Host[HostIdx]->ProtocolId]->ReceiveMsg(
            SBN.Host[HostIdx], &MsgType, &MsgSize, &CpuId, Msg);

        if(status == SBN_IF_EMPTY)
        {
            break; /* no (more) messages */
        }/* end if */

        if(status == SBN_OK)
        {
            PeerIdx = SBN_GetPeerIndex(CpuId);
            if(PeerIdx == SBN_ERROR)
            {
                CFE_EVS_SendEvent(SBN_PROTO_EID, CFE_EVS_ERROR,
                    "unknown peer (CpuId = %d)", CpuId);
                continue;
            }/* end if */

            OS_GetLocalTime(&SBN.Peer[PeerIdx].last_received);

            SBN_ProcessNetMsg(MsgType, CpuId, MsgSize, Msg);
        }
        else if(status == SBN_ERROR)
        {
            // TODO error message
            SBN.HkPkt.PeerHk[HostIdx].RecvErrCount++;
        }/* end if */
    }/* end for */
}/* end SBN_RcvHostMsgs */

/**
 * Checks all interfaces for messages from peers.
 */
void SBN_CheckForNetAppMsgs(void)
{
    int HostIdx = 0;

    /* DEBUG_START(); chatty */

    for(HostIdx = 0; HostIdx < SBN.NumHosts; HostIdx++)
    {
        if(!SBN.Host[HostIdx]->IsValid)
        {
            continue;
        }/* end if */

        SBN_ProcessNetAppMsgsFromHost(HostIdx);
    }/* end for */
}/* end SBN_CheckForNetAppMsgs */

void SBN_VerifyPeerInterfaces()
{
    int PeerIdx = 0, status = 0;

    DEBUG_START();

    for(PeerIdx = 0; PeerIdx < SBN.NumPeers; PeerIdx++)
    {
        status = SBN.IfOps[SBN.Peer[PeerIdx].ProtocolId]->VerifyPeerInterface(
            SBN.Peer[PeerIdx].IfData, SBN.Host, SBN.NumHosts);
        SBN.Peer[PeerIdx].IfData->IsValid = status;
        if(!status)
        {
            SBN.Peer[PeerIdx].InUse = FALSE;

            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
                "peer '%s' not valid", SBN.Peer[PeerIdx].IfData->Name);
        }/* end if */
    }/* end for */
}/* end SBN_VerifyPeerInterfaces */

void SBN_VerifyHostInterfaces()
{
    int HostIdx = 0, status = 0;

    DEBUG_START();

    for(HostIdx = 0; HostIdx < SBN.NumHosts; HostIdx++)
    {
        status = SBN.IfOps[SBN.Host[HostIdx]->ProtocolId]->VerifyHostInterface(
            SBN.Host[HostIdx], SBN.Peer, SBN.NumPeers);

        SBN.Host[HostIdx]->IsValid = status;
        if(!status)
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
                "host '%s' not valid", SBN.Host[HostIdx]->Name);
        }/* end if */
    }/* end for */
}/* end SBN_VerifyHostInterfaces */
