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
#include "sbn_netif.h"
#include "sbn_main_events.h"

#include <network_includes.h>
#include <string.h>
#include <errno.h>

static int AddHost(uint8 ProtocolId, uint8 NetNum, void **ModulePvtPtr)
{
    if(SBN.Hk.HostCount >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "too many host entries (%d, max = %d)",
            SBN.Hk.HostCount, SBN_MAX_NETWORK_PEERS);

        return SBN_ERROR;
    }/* end if */

    SBN_HostInterface_t *HostInterface = &SBN.Hosts[SBN.Hk.HostCount];

    memset(HostInterface, 0, sizeof(*HostInterface));

    HostInterface->Status = &SBN.Hk.HostStatus[SBN.Hk.HostCount];
    memset(HostInterface->Status, 0, sizeof(*HostInterface->Status));
    HostInterface->Status->NetNum = NetNum;
    HostInterface->Status->ProtocolId = ProtocolId;

    *ModulePvtPtr = HostInterface->ModulePvt;
    SBN.Hk.HostCount++;

    return SBN_SUCCESS;
}/* end AddHost */

static int AddPeer(const char *Name, uint32 ProcessorId, uint8 ProtocolId,
    uint32 SpacecraftId, uint8 QoS, uint8 NetNum, void **ModulePvtPtr)
{
    if(SBN.Hk.PeerCount >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "too many peer entries (%d, max = %d)",
            SBN.Hk.PeerCount, SBN_MAX_NETWORK_PEERS);

        return SBN_ERROR;
    }/* end if */


    SBN_PeerInterface_t *PeerInterface = &SBN.Peers[SBN.Hk.PeerCount];

    memset(PeerInterface, 0, sizeof(*PeerInterface));

    PeerInterface->Status = &SBN.Hk.PeerStatus[SBN.Hk.PeerCount];
    memset(PeerInterface->Status, 0, sizeof(*PeerInterface->Status));
    strncpy(PeerInterface->Status->Name, Name, SBN_MAX_PEERNAME_LENGTH);
    PeerInterface->Status->ProcessorId = ProcessorId;
    PeerInterface->Status->ProtocolId = ProtocolId;
    PeerInterface->Status->SpacecraftId = SpacecraftId;
    PeerInterface->Status->QoS = QoS;
    PeerInterface->Status->NetNum = NetNum;

    *ModulePvtPtr = PeerInterface->ModulePvt;
    SBN.Hk.PeerCount++;

    return SBN_SUCCESS;
}/* end AddPeer */

static int AddEntry(const char *Name, uint32 ProcessorId, uint8 ProtocolId,
    uint32 SpacecraftId, uint8 QoS, uint8 NetNum, void **ModulePvtPtr)
{
    if(SpacecraftId != CFE_PSP_GetSpacecraftId())
    {
        *ModulePvtPtr = NULL;
        return SBN_ERROR; /* ignore other spacecraft entries */
    }/* end if */

    if(ProcessorId == CFE_PSP_GetProcessorId())
    {
        return AddHost(ProtocolId, NetNum, ModulePvtPtr);
    }
    else
    {
        return AddPeer(Name, ProcessorId, ProtocolId, SpacecraftId,
            QoS, NetNum, ModulePvtPtr);
    }/* end if */

}/* end AddPeer */

#ifdef CFE_ES_CONFLOADER

#include "cfe_es_conf.h"

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
    void *ModulePvt = NULL;
    char *ValidatePtr = NULL;
    int Status = 0;

    if(FieldCount < 4)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "too few fields (FieldCount=%d)", FieldCount);
        return OS_SUCCESS;
    }/* end if */

    if(SBN.Hk.EntryCount >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "max entry count reached, skipping");
        return OS_ERROR;
    }/* end if */

    const char *Name = Row[0];
    if(strlen(Name) >= SBN_MAX_PEERNAME_LENGTH)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "Processor name too long: %s (%d)", Name,
            strlen(Name));
        return OS_ERROR;
    }/* end if */

    uint32 ProcessorId = strtol(Row[1], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[1])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid processor id");
        return OS_ERROR;
    }/* end if */

    uint8 ProtocolId = strtol(Row[2], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[2]
        || ProtocolId < 0 || ProtocolId > SBN_MAX_INTERFACE_TYPES
        || !SBN.IfOps[ProtocolId])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid protocol id");
        return OS_ERROR;
    }/* end if */

    uint32 SpacecraftId = strtol(Row[3], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[3])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid spacecraft id");
        return OS_ERROR;
    }/* end if */

    uint8 QoS = strtol(Row[4], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[4])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid quality of service");
        return OS_ERROR;
    }/* end if */

    uint8 NetNum = strtol(Row[5], &ValidatePtr, 0);
    if(!ValidatePtr || ValidatePtr == Row[5])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid network number");
        return OS_ERROR;
    }/* end if */

    if((Status = AddEntry(Name, ProcessorId, ProtocolId, SpacecraftId, QoS,
        NetNum, &ModulePvt)) == SBN_SUCCESS)
    {
        Status = SBN.IfOps[ProtocolId]->Load(Row + 6, FieldCount - 6,
            ModulePvt);
    }

    return Status;
}/* end PeerFileRowCallback */

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

#else /* ! CFE_ES_CONFLOADER */

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
    char Name[SBN_MAX_PEERNAME_LENGTH + 1], *NamePtr = Name,
        *ParameterEnd = NULL;
    void *ModulePvt = NULL;
    int Status = 0, ProcessorId = 0, ProtocolId = 0, SpacecraftId = 0,
        QoS = 0, NetNum = 0;

    DEBUG_START();

    ParseLineNum++;

    while(isspace(*FileEntry))
    {
        FileEntry++;
    }/* end while */

    ParameterEnd = FileEntry;
    while(ParameterEnd < FileEntry + SBN_MAX_PEERNAME_LENGTH
        && *ParameterEnd && !isspace(*ParameterEnd))
    {
        *NamePtr++ = *ParameterEnd++;
    }/* end if */

    *NamePtr = '\0';

    if(ParameterEnd - FileEntry > SBN_MAX_PEERNAME_LENGTH)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "peer name too long");
        return SBN_ERROR;
    }/* end if */

    FileEntry = ParameterEnd;
    ProcessorId = strtol(FileEntry, &ParameterEnd, 0);
    if(!ParameterEnd || ParameterEnd == FileEntry)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid processor id");
        return SBN_ERROR;
    }/* end if */

    FileEntry = ParameterEnd;
    ProtocolId = strtol(FileEntry, &ParameterEnd, 0);
    if(!ParameterEnd || ParameterEnd == FileEntry)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid protocol id");
        return SBN_ERROR;
    }/* end if */

    FileEntry = ParameterEnd;
    SpacecraftId = strtol(FileEntry, &ParameterEnd, 0);
    if(!ParameterEnd || ParameterEnd == FileEntry)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid spacecraft id");
        return SBN_ERROR;
    }/* end if */

    FileEntry = ParameterEnd;
    QoS = strtol(FileEntry, &ParameterEnd, 0);
    if(!ParameterEnd || ParameterEnd == FileEntry)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid quality of service");
        return SBN_ERROR;
    }/* end if */

    FileEntry = ParameterEnd;
    NetNum = strtol(FileEntry, &ParameterEnd, 0);
    if(!ParameterEnd || ParameterEnd == FileEntry)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid network number");
        return SBN_ERROR;
    }/* end if */

    FileEntry = ParameterEnd;
    while(isspace(*FileEntry))
    {
        FileEntry++;
    }/* end while */

    if((Status = AddEntry(Name, ProcessorId, ProtocolId, SpacecraftId,
        QoS, NetNum, &ModulePvt)) == SBN_SUCCESS)
    {
        Status = SBN.IfOps[ProtocolId]->Parse(FileEntry, ParseLineNum,
            ModulePvt);
    }/* end if */

    return Status;
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

/* \brief Utility function to swap headers.
 * \note Don't run this on a pointer given to us by the SB, as the SB may
 *       re-use the same buffer for multiple pipes.
 */
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
 * \param CpuId[in] The CpuId of the sender (should be CFE_CPU_ID)
 * \param Msg[in] The payload (CCSDS message or SBN sub/unsub.)
 */
void SBN_PackMsg(SBN_PackedMsg_t *SBNBuf, SBN_MsgSize_t MsgSize,
    SBN_MsgType_t MsgType, SBN_CpuId_t CpuId, SBN_Payload_t Msg)
{
    MsgSize = CFE_MAKE_BIG16(MsgSize);
    memcpy(SBNBuf->Hdr.MsgSizeBuf, &MsgSize, sizeof(MsgSize));
    MsgSize = CFE_MAKE_BIG16(MsgSize); /* swap back */

    memcpy(SBNBuf->Hdr.MsgTypeBuf, &MsgType, sizeof(MsgType));

    CpuId = CFE_MAKE_BIG32(CpuId);
    memcpy(SBNBuf->Hdr.CpuIdBuf, &CpuId, sizeof(CpuId));
    CpuId = CFE_MAKE_BIG32(CpuId); /* swap back */

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
 * \param CpuId[out] The CpuId of the sender.
 * \param Msg[out] The payload (a CCSDS message, or SBN sub/unsub).
 * \note Ensures the SBN fields (CPU ID, MsgSize) and CCSDS message headers
 *       are in platform byte order.
 * \todo Use a type for SBNBuf.
 */
void SBN_UnpackMsg(SBN_PackedMsg_t *SBNBuf, SBN_MsgSize_t *MsgSizePtr,
    SBN_MsgType_t *MsgTypePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t Msg)
{
    *MsgSizePtr = CFE_MAKE_BIG16(*((SBN_MsgSize_t *)SBNBuf->Hdr.MsgSizeBuf));
    *MsgTypePtr = *((SBN_MsgType_t *)SBNBuf->Hdr.MsgTypeBuf);
    *CpuIdPtr = CFE_MAKE_BIG32(*((SBN_CpuId_t *)SBNBuf->Hdr.CpuIdBuf));

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

/* Use a struct for all local variables in the task so we can specify exactly
 * how large of a stack we need for the task.
 */
typedef struct
{
    int PeerIdx, RecvPeerIdx, Status;
    uint32 TaskID;
    SBN_PeerInterface_t *Interface;
    SBN_CpuId_t CpuId;
    SBN_MsgType_t MsgType;
    SBN_MsgSize_t MsgSize;
    uint8 Msg[CFE_SB_MAX_SB_MSG_SIZE];
} PeerTaskData_t;

void PeerTask(void)
{
    PeerTaskData_t D;
    memset(&D, 0, sizeof(D));
    if((D.Status = CFE_ES_RegisterChildTask()) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to register child task (%d)", D.Status);
        return;
    }/* end if */

    D.TaskID = OS_TaskGetId();

    for(D.PeerIdx = 0; D.PeerIdx < SBN.Hk.PeerCount; D.PeerIdx++)
    {
        if(SBN.Peers[D.PeerIdx].TaskID == D.TaskID)
        {
            break;
        }/* end if */
    }/* end for */

    if(D.PeerIdx == SBN.Hk.PeerCount)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to connect task to peer struct");
        return;
    }/* end if */

    D.Interface = &SBN.Peers[D.PeerIdx];

    while(1)
    {
        D.Status = SBN.IfOps[SBN.Hk.PeerStatus[D.PeerIdx].ProtocolId]->Recv(
                D.Interface, &D.MsgType, &D.MsgSize, &D.CpuId, &D.Msg);

        if(D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }/* end if */

        /* for UDP, the message received may not be from the peer
         * expected.
         */
        D.RecvPeerIdx = SBN_GetPeerIndex(D.CpuId);

        if(D.Status == SBN_SUCCESS)
        {
            OS_GetLocalTime(&SBN.Hk.PeerStatus[D.RecvPeerIdx].LastReceived);

            SBN_ProcessNetMsg(D.MsgType, D.CpuId, D.MsgSize, &D.Msg);
        }
        else
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                "error received from peer recv (%d)", D.Status);
            SBN.Hk.PeerStatus[D.RecvPeerIdx].RecvErrCount++;
        }/* end if */
    }/* end while */
}/* end PeerTask */

/**
 * Loops through all hosts and peers, initializing all.
 *
 * @return SBN_SUCCESS if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
int SBN_InitInterfaces(void)
{
    int EntryIdx = 0;
    char TaskName[OS_MAX_API_NAME];

    DEBUG_START();

    for(EntryIdx = 0; EntryIdx < SBN.Hk.HostCount; EntryIdx++)
    {
        SBN.IfOps[SBN.Hosts[EntryIdx].Status->ProtocolId]->InitHost(
            &SBN.Hosts[EntryIdx]);
    }/* end  for */

    for(EntryIdx = 0; EntryIdx < SBN.Hk.PeerCount; EntryIdx++)
    {
        SBN_PeerInterface_t *PeerInterface = &SBN.Peers[EntryIdx];

        int Status = SBN_CreatePipe4Peer(PeerInterface);
        if(Status != SBN_SUCCESS)
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                "error creating pipe for %s", PeerInterface->Status->Name);
            return Status;
        }/* end if */

        /* Reset counters, flags and timers */
        SBN.Hk.PeerStatus[SBN.Hk.PeerCount].State = SBN_ANNOUNCING;

        SBN.IfOps[PeerInterface->Status->ProtocolId]->InitPeer(PeerInterface);

        snprintf(TaskName, OS_MAX_API_NAME, "sbn_peer_%d", EntryIdx);
        Status = CFE_ES_CreateChildTask(&PeerInterface->TaskID, TaskName,
            (CFE_ES_ChildTaskMainFuncPtr_t)&PeerTask, NULL,
            CFE_ES_DEFAULT_STACK_SIZE + sizeof(PeerTaskData_t), 0, 0);
        if(Status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                "error creating task for %s", PeerInterface->Status->Name);
            return Status;
        }/* end if */
    }/* end for */

    CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
        "configured, %d hosts and %d peers",
        SBN.Hk.HostCount, SBN.Hk.PeerCount);

    return SBN_SUCCESS;
}/* end SBN_InitInterfaces */

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
int SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSize_t MsgSize,
    SBN_Payload_t *Msg, int PeerIdx)
{
    int Status = 0;

    DEBUG_MSG("%s type=%04x size=%d", __FUNCTION__, MsgType,
        MsgSize);

    Status = SBN.IfOps[SBN.Hk.PeerStatus[PeerIdx].ProtocolId]->Send(
        &SBN.Peers[PeerIdx], MsgType, MsgSize, Msg);

    if(Status != -1)
    {
        SBN.Hk.PeerStatus[PeerIdx].SentCount++;
        OS_GetLocalTime(&SBN.Hk.PeerStatus[PeerIdx].LastSent);
    }
    else
    {
        SBN.Hk.PeerStatus[PeerIdx].SentErrCount++;
    }/* end if */

    return Status;
}/* end SBN_SendNetMsg */
