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
**            C. Knight/ARC Code TI
**
******************************************************************************/

/*
** Include Files
*/
#include "cfe.h"
#include "cfe_sb_msg.h"
#include "cfe_sb.h"
#include "sbn_app.h"
#include "sbn_msgids.h"
#include "sbn_netif.h"
#include "sbn_main_events.h"

#include <network_includes.h>
#include <string.h>
#include <errno.h>


/* Remap table from fields are sorted and unique, use a binary search. */
static CFE_SB_MsgId_t RemapMsgID(uint32 ProcessorID, CFE_SB_MsgId_t from)
{
    SBN_RemapTable_t *RemapTable = SBN.RemapTable;
    int start = 0, end = RemapTable->Entries - 1, midpoint = 0;

    while(end > start)
    {   
        midpoint = (end + start) / 2;
        if(RemapTable->Entry[midpoint].ProcessorID == ProcessorID
            && RemapTable->Entry[midpoint].from == from)
        {   
            break;  
        }/* end if */

        if(RemapTable->Entry[midpoint].ProcessorID < ProcessorID
            || (RemapTable->Entry[midpoint].ProcessorID == ProcessorID
            && RemapTable->Entry[midpoint].from < from))
        {   
            if(midpoint == start) /* degenerate case where end = start + 1 */
            {   
                return 0;
            }  
            else
            {   
                start = midpoint;
            }/* end if */
        }  
        else
        {   
            end = midpoint;
        }/* end if */
    }

    if(end > start)
    {
        return RemapTable->Entry[midpoint].to;
    }/* end if */

    /* not found... */

    if(RemapTable->RemapDefaultFlag == SBN_REMAP_DEFAULT_SEND)
    {
        return from;
    }/* end if */

    return 0;
}/* end RemapMsgID */

#ifdef CFE_ES_CONFLOADER
#include "cfe_es_conf.h"
#endif /* CFE_ES_CONFLOADER */

/**
 * Handles a row's worth of fields from a configuration file.
 * @param[in] FileName The FileName of the configuration file currently being
 *            processed.
 * @param[in] LineNum The line number of the line being processed.
 * @param[in] header The section header (if any) for the configuration file
 *            section.
 * @param[in] Row The entries from the row.
 * @param[in] FieldCount The number of fields in the row array.
 * @param[in] Opaque The Opaque data passed through the parser.
 * @return OS_SUCCESS on successful loading of the configuration file row.
 */
static int PeerFileRowCallback(const char *Filename, int LineNum,
    const char *Header, const char *Row[], int FieldCount, void *Opaque)
{
    char *ValidatePtr = NULL;
    int Status = 0;

    if(FieldCount < 4)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "too few fields (FieldCount=%d)", FieldCount);
        return OS_SUCCESS;
    }/* end if */

    const char *Name = Row[0];
    if(strlen(Name) >= SBN_MAX_PEERNAME_LENGTH)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "Processor name too long: %s (%d)", Name,
            strlen(Name));
        return OS_ERROR;
    }/* end if */

    uint32 ProcessorID = strtol(Row[1], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[1])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid processor id");
        return OS_ERROR;
    }/* end if */

    uint8 ProtocolID = strtol(Row[2], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[2]
        || ProtocolID < 0 || ProtocolID > SBN_MAX_INTERFACE_TYPES
        || !SBN.IfOps[ProtocolID])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid protocol id");
        return OS_ERROR;
    }/* end if */

    uint32 SpacecraftID = strtol(Row[3], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[3])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid spacecraft id");
        return OS_ERROR;
    }/* end if */

    if(SpacecraftID != CFE_PSP_GetSpacecraftId())
    {   
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "Invalid spacecraft ID (got '%d', expected '%d')", SpacecraftID,
            CFE_PSP_GetSpacecraftId());
        return SBN_ERROR; /* ignore other spacecraft entries */
    }/* end if */

    uint8 QoS = strtol(Row[4], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[4])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid quality of service");
        return OS_ERROR;
    }/* end if */

    int NetIdx = 0;

    for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
    {
        if(!strcmp(Row[5], SBN.Nets[NetIdx].Status.Name))
        {
            break;
        }/* end if */
    }/* end for */

    SBN_NetInterface_t *Net = NULL;
    if(NetIdx == SBN.Hk.NetCount)
    {
        if(SBN.Hk.NetCount >= SBN_MAX_NETS)
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
                "Too many nets. (Max=%d)", SBN_MAX_NETS);
            return SBN_ERROR;
        }/* end if */

        NetIdx = SBN.Hk.NetCount++;
        Net = &SBN.Nets[NetIdx];
        memset(Net, 0, sizeof(*Net));
        memset(&Net->Status, 0, sizeof(Net->Status));
        CFE_SB_InitMsg(&Net->Status, SBN_TLM_MID, sizeof(Net->Status), TRUE);
        strncpy(Net->Status.Name, Row[5], sizeof(Net->Status.Name));
    }
    else
    {
        Net = &SBN.Nets[NetIdx];
    }/* end if */

    if(ProcessorID == CFE_PSP_GetProcessorId())
    {
        strncpy(Net->Status.Name, Row[5], sizeof(Net->Status.Name));
        Net->Status.ProtocolID = ProtocolID;
        Net->IfOps = SBN.IfOps[ProtocolID];
        Net->IfOps->LoadNet(Row + 6, FieldCount - 6, Net);
    }
    else
    {
        int PeerIdx = Net->Status.PeerCount++;
        if(PeerIdx >= SBN_MAX_PEERS_PER_NET)
        {   
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
                "too many peer entries (%d, max = %d)",
                PeerIdx, SBN_MAX_PEERS_PER_NET);

            return SBN_ERROR;
        }/* end if */

        SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

        memset(Peer, 0, sizeof(*Peer));

        Peer->Net = Net;
        memset(&Peer->Status, 0, sizeof(Peer->Status));
        CFE_SB_InitMsg(&Peer->Status, SBN_TLM_MID, sizeof(Peer->Status), TRUE);
        strncpy(Peer->Status.Name, Name, SBN_MAX_PEERNAME_LENGTH);
        Peer->Status.ProcessorID = ProcessorID;
        Peer->Status.QoS = QoS;
        SBN.IfOps[ProtocolID]->LoadPeer(Row + 6, FieldCount - 6, Peer);
    }/* end if */

    return Status;
}/* end PeerFileRowCallback */

#ifdef CFE_ES_CONFLOADER
/**
 * When the configuration file loader encounters an error, it will call this
 * function with details.
 * @param[in] FileName The FileName of the configuration file currently being
 *            processed.
 * @param[in] LineNum The line number of the line being processed.
 * @param[in] ErrMessage The textual description of the error.
 * @param[in] Opaque The Opaque data passed through the parser.
 * @return OS_SUCCESS
 */
static int PeerFileErrCallback(const char *FileName, int LineNum,
    const char *Err, void *Opaque)
{
    return OS_SUCCESS;
}/* end PeerFileErrCallback */

/**
 * Function to read in, using the CFE_ES_Conf API, the configuration file
 * entries.
 *
 * @return OS_SUCCESS on successful completion.
 */
int32 SBN_GetPeerFileData(void)
{
    CFE_ES_ConfLoad(SBN_VOL_PEER_FILENAME, NULL, PeerFileRowCallback,
        PeerFileErrCallback, NULL);
    CFE_ES_ConfLoad(SBN_NONVOL_PEER_FILENAME, NULL, PeerFileRowCallback,
        PeerFileErrCallback, NULL);

    return SBN_SUCCESS;
}/* end SBN_GetPeerFileData */

#else /* !CFE_ES_CONFLOADER */

#include <ctype.h> /* isspace() */

/**
 * Parses a peer configuration file entry to obtain the peer configuration.
 *
 * @param[in] FileEntry  The row of peer configuration data.

 * @return  SBN_SUCCESS on success, SBN_ERROR on error
 */
static int ParseLineNum = 0;

static int ParseFileEntry(char *FileEntry)
{
    DEBUG_START();

    const char *Row[16];
    memset(&Row, 0, sizeof(Row));

    int FieldCount = 0;
    while(1)
    {
        /* eat leading spaces */
        for(; isspace(*FileEntry); FileEntry++);

        Row[FieldCount++] = FileEntry;
        if(FieldCount >= 16)
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
                "too many parameters, line %d", ParseLineNum);
            return SBN_ERROR;
        }/* end if */

        char *End = FileEntry;
        /* run to the end of this parameter */
        for(; *End && !isspace(*End); End++);

        if(!*End)
        {
            break;
        }/* end if */

        *End = '\0';
        FileEntry = End + 1;
    }/* end while */

    
    return PeerFileRowCallback("unknown", ParseLineNum++, "", Row, FieldCount,
        NULL);
}/* end ParseFileEntry */

/**
 * Parse the peer configuration file(s) to find the peers that this cFE should
 * connect to.
 *
 * @return SBN_SUCCESS on success.
 */
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

    memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
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
            memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SIZE);
            BuffLen = 0;
        }/* end if */
    }/* end while */

    OS_close(PeerFile);

    return SBN_SUCCESS;
}/* end SBN_GetPeerFileData */

#endif /* CFE_ES_CONFLOADER */

#ifdef SOFTWARE_BIG_BIT_ORDER
  #define CFE_MAKE_BIG32(n) (n)
#else /* !SOFTWARE_BIG_BIT_ORDER */
  #define CFE_MAKE_BIG32(n) ( (((n) << 24) & 0xFF000000) | (((n) << 8) & 0x00FF0000) | (((n) >> 8) & 0x0000FF00) | (((n) >> 24) & 0x000000FF) )
#endif /* SOFTWARE_BIG_BIT_ORDER */

static void SwapCCSDS(CFE_SB_Msg_t *Msg)
{
    int CCSDSType = CCSDS_RD_TYPE(*((CCSDS_PriHdr_t *)Msg));
    if(CCSDSType == CCSDS_TLM)
    {   
        CCSDS_TlmPkt_t *TlmPktPtr = (CCSDS_TlmPkt_t *)Msg;

        uint32 Seconds = CCSDS_RD_SEC_HDR_SEC(TlmPktPtr->SecHdr);
        Seconds = CFE_MAKE_BIG32(Seconds);
        CCSDS_WR_SEC_HDR_SEC(TlmPktPtr->SecHdr, Seconds);

        /* SBN sends CCSDS telemetry messages with secondary headers in
         * big-endian order.
         */
        if(CCSDS_TIME_SIZE == 6)
        {
            uint16 SubSeconds = CCSDS_RD_SEC_HDR_SUBSEC(TlmPktPtr->SecHdr);
            SubSeconds = CFE_MAKE_BIG16(SubSeconds);
            CCSDS_WR_SEC_HDR_SUBSEC(TlmPktPtr->SecHdr, SubSeconds);
        }
        else
        {
            uint32 SubSeconds = CCSDS_RD_SEC_HDR_SUBSEC(TlmPktPtr->SecHdr);
            SubSeconds = CFE_MAKE_BIG32(SubSeconds);
            CCSDS_WR_SEC_HDR_SUBSEC(TlmPktPtr->SecHdr, SubSeconds);
        }/* end if */
    }
    else if(CCSDSType == CCSDS_CMD)
    {
        CCSDS_CmdPkt_t *CmdPktPtr = (CCSDS_CmdPkt_t *)Msg;

        CmdPktPtr->SecHdr.Command = CFE_MAKE_BIG16(CmdPktPtr->SecHdr.Command);;
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
 * \param CpuID[in] The CpuID of the sender (should be CFE_CPU_ID)
 * \param Msg[in] The payload (CCSDS message or SBN sub/unsub.)
 */
void SBN_PackMsg(SBN_PackedMsg_t *SBNBuf, SBN_MsgSize_t MsgSize,
    SBN_MsgType_t MsgType, SBN_CpuID_t CpuID, SBN_Payload_t Msg)
{
    MsgSize = CFE_MAKE_BIG16(MsgSize);
    memcpy(SBNBuf->Hdr.MsgSizeBuf, &MsgSize, sizeof(MsgSize));
    MsgSize = CFE_MAKE_BIG16(MsgSize); /* swap back */

    memcpy(SBNBuf->Hdr.MsgTypeBuf, &MsgType, sizeof(MsgType));

    CpuID = CFE_MAKE_BIG32(CpuID);
    memcpy(SBNBuf->Hdr.CpuIDBuf, &CpuID, sizeof(CpuID));
    CpuID = CFE_MAKE_BIG32(CpuID); /* swap back */

    if(!Msg || !MsgSize)
    {
        return;
    }/* end if */

    memcpy(SBNBuf->Payload, Msg, MsgSize);

    if(MsgType == SBN_APP_MSG)
    {
        SwapCCSDS((CFE_SB_Msg_t *)SBNBuf->Payload);
    }/* end if */
}/* end SBN_PackMsg */

/**
 * \brief Unpacks a CCSDS message with an SBN message header.
 * \param SBNBuf[in] The buffer to unpack.
 * \param MsgTypePtr[out] The SBN message type.
 * \param MsgSizePtr[out] The payload size.
 * \param CpuID[out] The CpuID of the sender.
 * \param Msg[out] The payload (a CCSDS message, or SBN sub/unsub).
 * \note Ensures the SBN fields (CPU ID, MsgSize) and CCSDS message headers
 *       are in platform byte order.
 * \todo Use a type for SBNBuf.
 */
void SBN_UnpackMsg(SBN_PackedMsg_t *SBNBuf, SBN_MsgSize_t *MsgSizePtr,
    SBN_MsgType_t *MsgTypePtr, SBN_CpuID_t *CpuIDPtr, SBN_Payload_t Msg)
{
    *MsgSizePtr = CFE_MAKE_BIG16(*((SBN_MsgSize_t *)SBNBuf->Hdr.MsgSizeBuf));
    *MsgTypePtr = *((SBN_MsgType_t *)SBNBuf->Hdr.MsgTypeBuf);
    *CpuIDPtr = CFE_MAKE_BIG32(*((SBN_CpuID_t *)SBNBuf->Hdr.CpuIDBuf));

    if(!*MsgSizePtr)
    {
        return;
    }/* end if */

    memcpy(Msg, SBNBuf->Payload, *MsgSizePtr);

    if(*MsgTypePtr == SBN_APP_MSG)
    {
        SwapCCSDS((CFE_SB_Msg_t *)Msg);
    }/* end if */
}/* end SBN_UnpackMsg */

#ifdef SBN_RECV_TASK

/* Use a struct for all local variables in the task so we can specify exactly
 * how large of a stack we need for the task.
 */

typedef struct
{
    int Status;
    uint32 RecvTaskID;
    int PeerIdx, NetIdx;
    SBN_PeerInterface_t *Peer;
    SBN_NetInterface_t *Net;
    SBN_CpuID_t CpuID;
    SBN_MsgType_t MsgType;
    SBN_MsgSize_t MsgSize;
    uint8 Msg[CFE_SB_MAX_SB_MSG_SIZE];
} RecvPeerTaskData_t;

static void RecvPeerTask(void)
{
    RecvPeerTaskData_t D;
    memset(&D, 0, sizeof(D));
    if((D.Status = CFE_ES_RegisterChildTask()) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to register child task (%d)", D.Status);
        return;
    }/* end if */

    D.RecvTaskID = OS_TaskGetId();

    for(D.NetIdx = 0; D.NetIdx < SBN.Hk.NetCount; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        for(D.PeerIdx = 0; D.PeerIdx < D.Net->Status.PeerCount; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if(D.Peer->RecvTaskID == D.RecvTaskID)
            {
                break;
            }/* end if */
        }/* end for */

        if(D.PeerIdx < D.Net->Status.PeerCount)
        {
            break;
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to connect task to peer struct");
        return;
    }/* end if */

    while(1)
    {
        D.Status = D.Net->IfOps->RecvFromPeer(D.Net, D.Peer,
            &D.MsgType, &D.MsgSize, &D.CpuID, &D.Msg);

        if(D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }/* end if */

        if(D.Status == SBN_SUCCESS)
        {
            OS_GetLocalTime(&D.Peer->Status.LastRecv);

            SBN_ProcessNetMsg(D.Net, D.MsgType, D.CpuID, D.MsgSize, &D.Msg);
        }
        else
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                "recv error (%d)", D.Status);
            D.Peer->Status.RecvErrCount++;
        }/* end if */
    }/* end while */
}/* end RecvPeerTask */

typedef struct
{
    int NetIdx;
    SBN_NetInterface_t *Net;
    SBN_PeerInterface_t *Peer;
    int Status;
    uint32 RecvTaskID;
    SBN_CpuID_t CpuID;
    SBN_MsgType_t MsgType;
    SBN_MsgSize_t MsgSize;
    uint8 Msg[CFE_SB_MAX_SB_MSG_SIZE];
} RecvNetTaskData_t;

static void RecvNetTask(void)
{
    RecvNetTaskData_t D;
    memset(&D, 0, sizeof(D));
    if((D.Status = CFE_ES_RegisterChildTask()) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to register child task (%d)", D.Status);
        return;
    }/* end if */

    D.RecvTaskID = OS_TaskGetId();

    for(D.NetIdx = 0; D.NetIdx < SBN.Hk.NetCount; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        if(D.Net->RecvTaskID == D.RecvTaskID)
        {
            break;
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to connect task to net struct");
        return;
    }/* end if */

    while(1)
    {
        D.Status = D.Net->IfOps->RecvFromNet(D.Net, &D.MsgType,
            &D.MsgSize, &D.CpuID, &D.Msg);

        if(D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }/* end if */

        if(D.Status != SBN_SUCCESS)
        {
            return;
        }/* end if */

        D.Peer = SBN_GetPeer(D.Net, D.CpuID);
        if(!D.Peer)
        {
            return;
        }/* end if */

        OS_GetLocalTime(&D.Peer->Status.LastRecv);

        SBN_ProcessNetMsg(D.Net, D.MsgType, D.CpuID, D.MsgSize, &D.Msg);
    }/* end while */
}/* end RecvNetTask */

#else /* !SBN_RECV_TASK */

/**
 * Checks all interfaces for messages from peers.
 * Receive (up to a hundred) messages from the specified peer, injecting them
 * onto the local software bus.
 */
void SBN_RecvNetMsgs(void)
{
    int Status = 0;
    uint8 Msg[CFE_SB_MAX_SB_MSG_SIZE];

    /* DEBUG_START(); chatty */

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        if(Net->IfOps->RecvFromNet)
        {
                Status = Net->IfOps->RecvFromNet(
                    Net, &MsgType, &MsgSize, &CpuID, Msg);
        }/* end if */

        if(Net->IfOps->RecvFromPeer)
        {
            int PeerIdx = 0;
            for(PeerIdx = 0; PeerIdx < Net->Status.PeerCount; PeerIdx++)
            {
                SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

                /* Process up to 100 received messages
                 * TODO: make configurable
                 */
                int i = 0;
                for(i = 0; i < 100; i++)
                {
                    SBN_CpuID_t CpuID = 0;
                    SBN_MsgType_t MsgType = 0;
                    SBN_MsgSize_t MsgSize = 0;

                    memset(Msg, 0, sizeof(Msg));

                    Status = Net->IfOps->RecvFromPeer(Net, Peer,
                        &MsgType, &MsgSize, &CpuID, Msg);

                    if(Status == SBN_IF_EMPTY)
                    {
                        break; /* no (more) messages */
                    }/* end if */

                    /* for UDP, the message received may not be from the peer
                     * expected.
                     */
                    SBN_PeerInterface_t *RealPeer = SBN_GetPeer(Net, CpuID);

                    if(RealPeer)
                    {
                        OS_GetLocalTime(&RealPeer->Status.LastRecv);

                        SBN_ProcessNetMsg(Net, MsgType, CpuID, MsgSize, Msg);
                    }/* end if */
                }/* end for */
        }/* end for */
        }
    }/* end for */
}/* end SBN_RecvNetMsgs */

#endif /* SBN_RECV_TASK */

/**
 * Sends a message to a peer using the module's SendNetMsg.
 *
 * @param MsgType       SBN type of the message
 * @param MsgSize       Size of the message
 * @param Msg           Message to send
 * @param Peer          The peer to send the message to.
 * @return Number of characters sent on success, -1 on error.
 *
 */
int SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSize_t MsgSize,
    SBN_Payload_t *Msg, SBN_PeerInterface_t *Peer)
{
    int Status = 0;
    SBN_NetInterface_t *Net = Peer->Net;

    DEBUG_MSG("%s type=%04x size=%d", __FUNCTION__, MsgType,
        MsgSize);

    #ifdef SBN_SEND_TASK

    if(OS_MutSemTake(Net->SendMutex) != OS_SUCCESS)
    {   
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR, "unable to take mutex");
        return SBN_ERROR;
    }/* end if */

    #endif /* SBN_SEND_TASK */

    Status = Net->IfOps->Send(Net, Peer, MsgType, MsgSize, Msg);

    if(Status != -1)
    {
        Peer->Status.SendCount++;
        OS_GetLocalTime(&Peer->Status.LastSend);
    }
    else
    {
        Peer->Status.SendErrCount++;
    }/* end if */

    #ifdef SBN_SEND_TASK

    if(OS_MutSemGive(Net->SendMutex) != OS_SUCCESS)
    {   
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR, "unable to give mutex");
        return SBN_ERROR;
    }/* end if */

    #endif /* SBN_SEND_TASK */

    return Status;
}/* end SBN_SendNetMsg */

#ifdef SBN_SEND_TASK

typedef struct
{
    int NetIdx, PeerIdx, Status;
    uint32 SendTaskID;
    CFE_SB_MsgPtr_t SBMsgPtr;
    CFE_SB_MsgId_t MsgID;
    CFE_SB_SenderId_t *LastSenderPtr;
    SBN_NetInterface_t *Net;
    SBN_PeerInterface_t *Peer;
} SendTaskData_t;

/**
 * \brief When a peer is connected, a task is created to listen to the relevant
 * pipe for messages to send to that peer.
 */
static void SendTask(void)
{
    SendTaskData_t D;

    memset(&D, 0, sizeof(D));

    if((D.Status = CFE_ES_RegisterChildTask()) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to register child task (%d)", D.Status);
        return;
    }/* end if */

    D.SendTaskID = OS_TaskGetId();

    for(D.NetIdx = 0; D.NetIdx < SBN.Hk.NetCount; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        for(D.PeerIdx = 0; D.PeerIdx < D.Net->Status.PeerCount; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if(D.Peer->SendTaskID == D.SendTaskID)
            {
                break;
            }/* end if */
        }/* end for */
        
        if(D.PeerIdx < D.Net->Status.PeerCount)
        {
            break; /* found a ringer */
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.Hk.NetCount)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "error connecting send task\n");
        return;
    }/* end if */

    while(1)
    {
        if(D.Peer->Status.State != SBN_HEARTBEATING)
        {
            OS_TaskDelay(1000);
            continue;
        }/* end if */

        /** as long as it's connected, process messages for the peer **/
        while(D.Peer->Status.State == SBN_HEARTBEATING)
        {
            if(CFE_SB_RcvMsg(&D.SBMsgPtr, D.Peer->Pipe, CFE_SB_PEND_FOREVER)
                != CFE_SUCCESS)
            {   
                break;
            }/* end if */

            /* don't re-send what SBN sent */
            CFE_SB_GetLastSenderId(&D.LastSenderPtr, D.Peer->Pipe);

            if(!strncmp(SBN.App_FullName, D.LastSenderPtr->AppName,
                strlen(SBN.App_FullName)))
            {
                continue;
            }/* end if */

            if(SBN.RemapTable)
            {   
                D.MsgID = RemapMsgID(D.Peer->Status.ProcessorID,
                    CFE_SB_GetMsgId(D.SBMsgPtr));
                if(!D.MsgID)
                {   
                    break; /* don't send message, filtered out */
                }/* end if */
                CFE_SB_SetMsgId(D.SBMsgPtr, D.MsgID);
            }/* end if */

            SBN_SendNetMsg(SBN_APP_MSG,
                CFE_SB_GetTotalMsgLength(D.SBMsgPtr),
                (SBN_Payload_t *)D.SBMsgPtr, D.Peer);
        }/* end while */
    }/* end while */
}/* end SendTask */

static uint32 CreateSendTask(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer)
{
    uint32 Status = OS_SUCCESS;
    char MutexName[OS_MAX_API_NAME], SendTaskName[32];

    snprintf(MutexName, OS_MAX_API_NAME, "sendM_%s_%s", Net->Status.Name,
        Peer->Status.Name);

    Status = OS_MutSemCreate(&(Peer->SendMutex), MutexName, 0);

    if(Status != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "error creating mutex for %s", Peer->Status.Name);
        return Status;
    }

    snprintf(SendTaskName, 32, "sendT_%s_%s", Net->Status.Name,
        Peer->Status.Name);
    Status = CFE_ES_CreateChildTask(&(Peer->SendTaskID),
        SendTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SendTask, NULL,
        CFE_ES_DEFAULT_STACK_SIZE + 2 * sizeof(SendTaskData_t), 0, 0);

    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "error creating send task for %s", Peer->Status.Name);
        return Status;
    }/* end if */

    return Status;
}/* end CreateSendTask */

#else /* !SBN_SEND_TASK */

/**
 * Iterate through all peers, examining the pipe to see if there are messages
 * I need to send to that peer.
 */
void SBN_CheckPeerPipes(void)
{
    int PeerIdx = 0, ReceivedFlag = 0, iter = 0;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    CFE_SB_SenderId_t *LastSenderPtr = NULL;

    /* DEBUG_START(); chatty */

    /**
     * \note This processes one message per peer, then start again until no
     * peers have pending messages. At max only process SBN_MAX_MSG_PER_WAKEUP
     * per peer per wakeup otherwise I will starve other processing.
     */
    for(iter = 0; iter < SBN_MAX_MSG_PER_WAKEUP; iter++)
    {   
        ReceivedFlag = 0;

        int NetIdx = 0;
        for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
        {
            SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

            int PeerIdx = 0;
            for(PeerIdx = 0; PeerIdx < Net->Status.PeerCount; PeerIdx++)
            {
                SBN_PeerInterface_t *Peer = Net->Peers[PeerIdx];
                /* if peer data is not in use, go to next peer */
                if(Peer->Status.State != SBN_HEARTBEATING)
                {   
                    continue;
                }

                if(CFE_SB_RcvMsg(&SBMsgPtr, Peer->Pipe, CFE_SB_POLL)
                    != CFE_SUCCESS)
                {   
                    continue;
                }/* end if */

                ReceivedFlag = 1;

                /* don't re-send what SBN sent */
                CFE_SB_GetLastSenderId(&LastSenderPtr, Peer->Pipe);

                if(!strncmp(SBN.App_FullName, LastSenderPtr->AppName,
                    strlen(SBN.App_FullName)))
                {   
                    continue;
                }/* end if */

                if(SBN.RemapTable)
                {   
                    CFE_SB_MsgId_t MsgID =
                        RemapMsgID(Peer->Status.ProcessorID,
                        CFE_SB_GetMsgId(SBMsgPtr));
                    if(!MsgID)
                    {   
                        break; /* don't send message, filtered out */
                    }/* end if */
                    CFE_SB_SetMsgId(SBMsgPtr, MsgID);
                }/* end if */

                SBN_SendNetMsg(SBN_APP_MSG,
                    CFE_SB_GetTotalMsgLength(SBMsgPtr),
                    (SBN_Payload_t *)SBMsgPtr, Peer);
            }/* end for */
        }/* end for */

        if(!ReceivedFlag)
        {
            break;
        }/* end if */
    } /* end for */
}/* end SBN_CheckPeerPipes */

#endif /* SBN_SEND_TASK */

/**
 * Loops through all hosts and peers, initializing all.
 *
 * @return SBN_SUCCESS if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
int SBN_InitInterfaces(void)
{
    DEBUG_START();

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        Net->IfOps->InitNet(Net);

#ifdef SBN_RECV_TASK

        if(Net->IfOps->RecvFromNet)
        {
            char RecvTaskName[32];
            snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_recvs_%d", NetIdx);
            int Status = CFE_ES_CreateChildTask(&(Net->RecvTaskID),
                RecvTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&RecvNetTask,
                NULL, CFE_ES_DEFAULT_STACK_SIZE + 2 * sizeof(RecvNetTaskData_t),
                0, 0);

            if(Status != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                    "error creating task for %s", Net->Status.Name);
                return Status;
            }/* end if */
        }/* end if */

#endif /* SBN_RECV_TASK */

        int PeerIdx = 0;
        for(PeerIdx = 0; PeerIdx < Net->Status.PeerCount; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            char PipeName[OS_MAX_API_NAME];

            /* create a pipe name string similar to SBN_CPU2_Pipe */
            snprintf(PipeName, OS_MAX_API_NAME, "SBN_%s_%s_Pipe",
                Net->Status.Name, Peer->Status.Name);
            int Status = CFE_SB_CreatePipe(&(Peer->Pipe), SBN_PEER_PIPE_DEPTH,
                PipeName);

            if(Status != CFE_SUCCESS)
            {   
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                    "failed to create pipe '%s'", PipeName);

                return Status;
            }/* end if */

            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
                "pipe created '%s'", PipeName);

            /* Reset counters, flags and timers */
            Peer->Status.State = SBN_ANNOUNCING;

            Net->IfOps->InitPeer(Peer);

#ifdef SBN_RECV_TASK

            if(Net->IfOps->RecvFromPeer)
            {
                char RecvTaskName[32];
                snprintf(RecvTaskName, OS_MAX_API_NAME, "sbn_recv_%d", PeerIdx);
                Status = CFE_ES_CreateChildTask(&(Peer->RecvTaskID),
                    RecvTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&RecvPeerTask,
                    NULL,
                    CFE_ES_DEFAULT_STACK_SIZE + 2 * sizeof(RecvPeerTaskData_t),
                    0, 0);
                /* TODO: more accurate stack size required */

                if(Status != CFE_SUCCESS)
                {
                    CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                        "error creating task for %s", Peer->Status.Name);
                    return Status;
                }/* end if */
            }/* end if */

#endif /* SBN_RECV_TASK */

#ifdef SBN_SEND_TASK

            CreateSendTask(Net, Peer);

#endif /* SBN_SEND_TASK */

        }/* end for */
    }/* end for */

    CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
        "configured, %d nets",
        SBN.Hk.NetCount);

    for(NetIdx = 0; NetIdx < SBN.Hk.NetCount; NetIdx++)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
            "net #%d has %d peers", NetIdx,
            SBN.Nets[NetIdx].Status.PeerCount);
    }/* end for */

    return SBN_SUCCESS;
}/* end SBN_InitInterfaces */
