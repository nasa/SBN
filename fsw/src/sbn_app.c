/******************************************************************************
 ** \file sbn_app.c
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
 **      This file contains source code for the Software Bus Network
 **      Application.
 **
 ** Authors:   J. Wilmot/GSFC Code582
 **            R. McGraw/SSI
 **            C. Knight/ARC Code TI
 ******************************************************************************/

/*
 ** Include Files
 */
#include <fcntl.h>

#include "sbn_pack.h"
#include "sbn_app.h"
#include "sbn_remap.h"
#include "cfe_sb_events.h" /* For event message IDs */
#include "cfe_sb_priv.h" /* For CFE_SB_SendMsgFull */
#include "cfe_es.h" /* PerfLog */
#include "cfe_platform_cfg.h"
#include "cfe_msgids.h"

#ifdef CFE_ES_CONFLOADER
#include "cfe_es_conf.h"
#else /* !CFE_ES_CONFLOADER */
#include <ctype.h> /* isspace() */
#endif /* CFE_ES_CONFLOADER */

/** \brief SBN global application data, indexed by AppID. */
SBN_App_t SBN;

/**
 * Handles a row's worth of fields from a configuration file.
 * @param[in] FileName The FileName of the configuration file currently being
 *            processed.
 * @param[in] LineNum The line number of the line being processed.
 * @param[in] header The section header (if any) for the configuration file
 *            section.
 * @param[in] Row The entries from the row.
 * @param[in] FieldCnt The number of fields in the row array.
 * @param[in] Opaque The Opaque data passed through the parser.
 * @return OS_SUCCESS on successful loading of the configuration file row.
 */
static int PeerFileRowCallback(const char *Filename, int LineNum,
    const char *Header, const char *Row[], int FieldCnt, void *Opaque)
{
    char *ValidatePtr = NULL;
    int Status = OS_SUCCESS;

    if(FieldCnt < 4)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "too few fields (FieldCnt=%d)", FieldCnt);
        return OS_ERROR;
    }/* end if */

    const char *Name = Row[0];
    if(strlen(Name) >= SBN_MAX_PEER_NAME_LEN)
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
        return OS_ERROR; /* ignore other spacecraft entries */
    }/* end if */

    uint16 QoSInt = strtol(Row[4], &ValidatePtr, 0);
    CFE_SB_Qos_t QoS = {(QoSInt & 0xFF00) >> 8, QoSInt & 0xFF};
    if(!ValidatePtr || ValidatePtr == Row[4])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "invalid quality of service");
        return OS_ERROR;
    }/* end if */

    uint8 NetIdx;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        if(!strcmp(Row[5], SBN.Nets[NetIdx].Name))
        {
            break;
        }
    }

    SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

    if(NetIdx == SBN.NetCnt)
    {
        if(NetIdx == SBN_MAX_NETS)
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
                "too many networks.");
            return OS_ERROR;
        }

        SBN.NetCnt++;

        memset(Net, 0, sizeof(*Net));

        strncpy(Net->Name, Row[5], sizeof(Net->Name) - 1);
    }

    if(ProcessorID == CFE_PSP_GetProcessorId())
    {
        Net->Configured = TRUE;

        Net->ProtocolID = ProtocolID;
        Net->IfOps = SBN.IfOps[ProtocolID];
        Net->IfOps->LoadNet(Row + 6, FieldCnt - 6, Net);
    }
    else
    {
        if(Net->PeerCnt >= SBN_MAX_PEERS_PER_NET)
        {   
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
                "too many peer entries (%d, max = %d)",
                Net->PeerCnt, SBN_MAX_PEERS_PER_NET);

            return OS_ERROR;
        }/* end if */

        SBN_PeerInterface_t *Peer = &Net->Peers[Net->PeerCnt++];

        memset(Peer, 0, sizeof(*Peer));

        strncpy(Peer->Name, Row[0], sizeof(Peer->Name) - 1);
        Peer->Net = Net;
        Peer->ProcessorID = ProcessorID;
        Peer->QoS = QoS;
        SBN.IfOps[ProtocolID]->LoadPeer(Row + 6, FieldCnt - 6, Peer);
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

/**
 * Parses a peer configuration file entry to obtain the peer configuration.
 *
 * @param[in] FileEntry The row of peer configuration data.

 * @return  SBN_SUCCESS on success, SBN_ERROR on error
 */
static int ParseLineNum = 0;

static int ParseFileEntry(char *FileEntry)
{
    const char *Row[16];
    memset(&Row, 0, sizeof(Row));

    int FieldCnt = 0;
    while(1)
    {
        /* eat leading spaces */
        for(; isspace(*FileEntry); FileEntry++);

        Row[FieldCnt++] = FileEntry;
        if(FieldCnt >= 16)
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

    
    return PeerFileRowCallback("unknown", ParseLineNum++, "", Row, FieldCnt,
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
    static char     SBN_PeerData[SBN_PEER_FILE_LINE_SZ];
    int             BuffLen = 0; /* Length of the current buffer */
    int             PeerFile = 0;
    char            c = '\0';
    int             FileOpened = FALSE;
    int             LineNum = 0;

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

    memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SZ);
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
            if(BuffLen < (SBN_PEER_FILE_LINE_SZ - 1))
                BuffLen++;
        }
        else if(c != ';')
        {
            /*
             ** Regular data gets copied in
             */
            SBN_PeerData[BuffLen] = c;
            if(BuffLen < (SBN_PEER_FILE_LINE_SZ - 1))
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
            memset(SBN_PeerData, 0x0, SBN_PEER_FILE_LINE_SZ);
            BuffLen = 0;
        }/* end if */
    }/* end while */

    OS_close(PeerFile);

    return SBN_SUCCESS;
}/* end SBN_GetPeerFileData */

#endif /* CFE_ES_CONFLOADER */

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
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in network (big-endian) byte order.
 * \param SBNBuf[out] The buffer to pack into.
 * \param MsgType[in] The SBN message type.
 * \param MsgSz[in] The size of the payload.
 * \param CpuID[in] The CpuID of the sender (should be CFE_CPU_ID)
 * \param Msg[in] The payload (CCSDS message or SBN sub/unsub.)
 */
void SBN_PackMsg(void *SBNBuf, SBN_MsgSz_t MsgSz,
    SBN_MsgType_t MsgType, SBN_CpuID_t CpuID, void * Msg)
{
    Pack_t Pack;
    Pack_Init(&Pack, SBNBuf, MsgSz + SBN_PACKED_HDR_SZ, 0);

    Pack_UInt16(&Pack, MsgSz);
    Pack_UInt8(&Pack, MsgType);
    Pack_UInt32(&Pack, CpuID);

    if(!Msg || !MsgSz)
    {
        /* valid to have a NULL pointer/empty size payload */
        return;
    }/* end if */

    Pack_Data(&Pack, Msg, MsgSz);

    if(MsgType == SBN_APP_MSG)
    {
        SwapCCSDS(SBNBuf + SBN_PACKED_HDR_SZ);
    }/* end if */
}/* end SBN_PackMsg */

/**
 * \brief Unpacks a CCSDS message with an SBN message header.
 * \param SBNBuf[in] The buffer to unpack.
 * \param MsgTypePtr[out] The SBN message type.
 * \param MsgSzPtr[out] The payload size.
 * \param CpuID[out] The CpuID of the sender.
 * \param Msg[out] The payload (a CCSDS message, or SBN sub/unsub).
 * \return TRUE if we were able to unpack the message.
 *
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in platform byte order.
 * \todo Use a type for SBNBuf.
 */
boolean SBN_UnpackMsg(void *SBNBuf, SBN_MsgSz_t *MsgSzPtr,
    SBN_MsgType_t *MsgTypePtr, SBN_CpuID_t *CpuIDPtr, void *Msg)
{
    Unpack_t Unpack; Unpack_Init(&Unpack, SBNBuf, SBN_MAX_PACKED_MSG_SZ);
    Unpack_UInt16(&Unpack, MsgSzPtr);
    Unpack_UInt8(&Unpack, MsgTypePtr);
    Unpack_UInt32(&Unpack, CpuIDPtr);

    if(!*MsgSzPtr)
    {
        return FALSE;
    }/* end if */

    if(*MsgSzPtr > CFE_SB_MAX_SB_MSG_SIZE)
    {
        return FALSE;
    }/* end if */

    Unpack_Data(&Unpack, Msg, *MsgSzPtr);

    if(*MsgTypePtr == SBN_APP_MSG)
    {
        SwapCCSDS((CFE_SB_Msg_t *)Msg);
    }/* end if */
    return TRUE;
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
    SBN_MsgSz_t MsgSz;
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

    for(D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        if(!D.Net.Configured)
        {
            continue;
        }

        for(D.PeerIdx = 0; D.PeerIdx < D.Net->PeerCnt; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if(D.Peer->RecvTaskID == D.RecvTaskID)
            {
                break;
            }/* end if */
        }/* end for */

        if(D.PeerIdx < D.Net->PeerCnt)
        {
            break;
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.NetCnt)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to connect task to peer struct");
        return;
    }/* end if */

    while(1)
    {
        D.Status = D.Net->IfOps->RecvFromPeer(D.Net, D.Peer,
            &D.MsgType, &D.MsgSz, &D.CpuID, &D.Msg);

        if(D.Status == SBN_IF_EMPTY)
        {
            continue; /* no (more) messages */
        }/* end if */

        if(D.Status == SBN_SUCCESS)
        {
            OS_GetLocalTime(&D.Peer->LastRecv);

            SBN_ProcessNetMsg(D.Net, D.MsgType, D.CpuID, D.MsgSz, &D.Msg);
        }
        else
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                "recv error (%d)", D.Status);
            D.Peer->RecvErrCnt++;
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
    SBN_MsgSz_t MsgSz;
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

    for(D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        if(D.Net->RecvTaskID == D.RecvTaskID)
        {
            break;
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.NetCnt)
    {
        CFE_EVS_SendEvent(SBN_PEERTASK_EID, CFE_EVS_ERROR,
            "unable to connect task to net struct");
        return;
    }/* end if */

    while(1)
    {
        D.Status = D.Net->IfOps->RecvFromNet(D.Net, &D.MsgType,
            &D.MsgSz, &D.CpuID, &D.Msg);

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

        OS_GetLocalTime(&D.Peer->LastRecv);

        SBN_ProcessNetMsg(D.Net, D.MsgType, D.CpuID, D.MsgSz, &D.Msg);
    }/* end while */
}/* end RecvNetTask */

#else /* !SBN_RECV_TASK */

/**
 * Checks all interfaces for messages from peers.
 * Receive messages from the specified peer, injecting them onto the local
 * software bus.
 */
void SBN_RecvNetMsgs(void)
{
    int Status = 0;
    uint8 Msg[CFE_SB_MAX_SB_MSG_SIZE];

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        SBN_MsgType_t MsgType;
        SBN_MsgSz_t MsgSz;
        SBN_CpuID_t CpuID;

        if(Net->IfOps->RecvFromNet)
        {
            int MsgCnt = 0;
            // TODO: make configurable
            for(MsgCnt = 0; MsgCnt < 100; MsgCnt++)
            {
                memset(Msg, 0, sizeof(Msg));

                Status = Net->IfOps->RecvFromNet(
                    Net, &MsgType, &MsgSz, &CpuID, Msg);

                if(Status == SBN_IF_EMPTY)
                {
                    break; /* no (more) messages */
                }/* end if */

                /* for UDP, the message received may not be from the peer
                 * expected.
                 */
                SBN_PeerInterface_t *Peer = SBN_GetPeer(Net, CpuID);

                if(Peer)
                {
                    OS_GetLocalTime(&Peer->LastRecv);
                }/* end if */

                SBN_ProcessNetMsg(Net, MsgType, CpuID, MsgSz, Msg);
            }
        }
        else if(Net->IfOps->RecvFromPeer)
        {
            int PeerIdx = 0;
            for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
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
                    SBN_MsgSz_t MsgSz = 0;

                    memset(Msg, 0, sizeof(Msg));

                    Status = Net->IfOps->RecvFromPeer(Net, Peer,
                        &MsgType, &MsgSz, &CpuID, Msg);

                    if(Status == SBN_IF_EMPTY)
                    {
                        break; /* no (more) messages */
                    }/* end if */

                    OS_GetLocalTime(&Peer->LastRecv);

                    SBN_ProcessNetMsg(Net, MsgType, CpuID, MsgSz, Msg);
                }/* end for */
            }/* end for */
        }
        else
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                "neither RecvFromPeer nor RecvFromNet defined for net #%d",
                NetIdx);
        }/* end if */
    }/* end for */
}/* end SBN_RecvNetMsgs */

#endif /* SBN_RECV_TASK */

/**
 * Sends a message to a peer using the module's SendNetMsg.
 *
 * @param MsgType SBN type of the message
 * @param MsgSz Size of the message
 * @param Msg Message to send
 * @param Peer The peer to send the message to.
 * @return Number of characters sent on success, -1 on error.
 *
 */
int SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz,
    void *Msg, SBN_PeerInterface_t *Peer)
{
    int Status = 0;
    SBN_NetInterface_t *Net = Peer->Net;

    #ifdef SBN_SEND_TASK

    if(OS_MutSemTake(SBN.SendMutex) != OS_SUCCESS)
    {   
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR, "unable to take mutex");
        return SBN_ERROR;
    }/* end if */

    #endif /* SBN_SEND_TASK */

    Status = Net->IfOps->Send(Peer, MsgType, MsgSz, Msg);

    if(Status != -1)
    {
        Peer->SendCnt++;
        OS_GetLocalTime(&Peer->LastSend);
    }
    else
    {
        Peer->SendErrCnt++;
    }/* end if */

    #ifdef SBN_SEND_TASK

    if(OS_MutSemGive(SBN.SendMutex) != OS_SUCCESS)
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

    for(D.NetIdx = 0; D.NetIdx < SBN.NetCnt; D.NetIdx++)
    {
        D.Net = &SBN.Nets[D.NetIdx];
        for(D.PeerIdx = 0; D.PeerIdx < D.Net->PeerCnt; D.PeerIdx++)
        {
            D.Peer = &D.Net->Peers[D.PeerIdx];
            if(D.Peer->SendTaskID == D.SendTaskID)
            {
                break;
            }/* end if */
        }/* end for */
        
        if(D.PeerIdx < D.Net->PeerCnt)
        {
            break; /* found a ringer */
        }/* end if */
    }/* end for */

    if(D.NetIdx == SBN.NetCnt)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "error connecting send task\n");
        return;
    }/* end if */

    while(1)
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

        if(SBN.RemapEnabled)
        {   
            D.MsgID = SBN_RemapMsgID(D.Peer->ProcessorID,
                CFE_SB_GetMsgId(D.SBMsgPtr));
            if(D.MsgID == 0x0000)
            {   
                continue; /* don't send message, filtered out */
            }/* end if */
            CFE_SB_SetMsgId(D.SBMsgPtr, D.MsgID);
        }/* end if */

        SBN_SendNetMsg(SBN_APP_MSG,
            CFE_SB_GetTotalMsgLength(D.SBMsgPtr),
            D.SBMsgPtr, D.Peer);
    }/* end while */
}/* end SendTask */

#else /* !SBN_SEND_TASK */

/**
 * Iterate through all peers, examining the pipe to see if there are messages
 * I need to send to that peer.
 */
static void CheckPeerPipes(void)
{
    int ReceivedFlag = 0, iter = 0;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    CFE_SB_SenderId_t *LastSenderPtr = NULL;

    /**
     * \note This processes one message per peer, then start again until no
     * peers have pending messages. At max only process SBN_MAX_MSG_PER_WAKEUP
     * per peer per wakeup otherwise I will starve other processing.
     */
    for(iter = 0; iter < SBN_MAX_MSG_PER_WAKEUP; iter++)
    {
        ReceivedFlag = 0;

        int NetIdx = 0;
        for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
        {
            SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

            int PeerIdx = 0;
            for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];
                /* if peer data is not in use, go to next peer */

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

                if(SBN.RemapEnabled)
                {
                    CFE_SB_MsgId_t MsgID =
                        SBN_RemapMsgID(Peer->ProcessorID,
                        CFE_SB_GetMsgId(SBMsgPtr));
                    if(!MsgID)
                    {   
                        continue; /* don't send message, filtered out */
                    }/* end if */

                    CFE_SB_SetMsgId(SBMsgPtr, MsgID);
                }/* end if */

                SBN_SendNetMsg(SBN_APP_MSG,
                    CFE_SB_GetTotalMsgLength(SBMsgPtr),
                    SBMsgPtr, Peer);
            }/* end for */
        }/* end for */

        if(!ReceivedFlag)
        {
            break;
        }/* end if */
    } /* end for */
}/* end CheckPeerPipes */

#endif /* SBN_SEND_TASK */

/**
 * Iterate through all peers, calling the poll interface if no messages have
 * been sent in the last SBN_POLL_TIME seconds.
 */
static void PeerPoll(void)
{
    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

        int PeerIdx = 0;
        for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            Net->IfOps->PollPeer(Peer);
        }/* end for */
    }/* end for */
}/* end PeerPoll */

/**
 * Loops through all hosts and peers, initializing all.
 *
 * @return SBN_SUCCESS if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
int SBN_InitInterfaces(void)
{
    if(SBN.NetCnt < 1)
    {
        CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
            "no networks configured");

        return SBN_ERROR;
    }/* end if */

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];

        if(!Net->Configured)
        {
            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                "network #%d not configured", NetIdx);

            return SBN_ERROR;
        }/* end if */

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
                    "error creating task for %s", Net->Name);
                return Status;
            }/* end if */
        }/* end if */

#endif /* SBN_RECV_TASK */

        int PeerIdx = 0;
        for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
        {
            SBN_PeerInterface_t *Peer = &Net->Peers[PeerIdx];

            char PipeName[OS_MAX_API_NAME];

            /* create a pipe name string similar to SBN_0_CPU2_Pipe */
            snprintf(PipeName, OS_MAX_API_NAME, "SBN_%d_%s_Pipe",
                NetIdx, Peer->Name);
            int Status = CFE_SB_CreatePipe(&(Peer->Pipe), 2, PipeName);

            if(Status != CFE_SUCCESS)
            {   
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                    "failed to create pipe '%s'", PipeName);

                return Status;
            }/* end if */

            CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_INFORMATION,
                "pipe created '%s'", PipeName);

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
                        "error creating task for %s", Peer->Name);
                    return Status;
                }/* end if */
            }/* end if */

#endif /* SBN_RECV_TASK */

#ifdef SBN_SEND_TASK

            char SendTaskName[32];

            snprintf(SendTaskName, 32, "sendT_%s_%s", Net->Name,
                Peer->Name);
            Status = CFE_ES_CreateChildTask(&(Peer->SendTaskID),
                SendTaskName, (CFE_ES_ChildTaskMainFuncPtr_t)&SendTask, NULL,
                CFE_ES_DEFAULT_STACK_SIZE + 2 * sizeof(SendTaskData_t), 0, 0);

            if(Status != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                    "error creating send task for %s", Peer->Name);
                return Status;
            }/* end if */

#endif /* SBN_SEND_TASK */

        }/* end for */
    }/* end for */

    CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
        "configured, %d nets",
        SBN.NetCnt);

    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
            "net %s (#%d) has %d peers", SBN.Nets[NetIdx].Name, NetIdx,
            SBN.Nets[NetIdx].PeerCnt);
    }/* end for */

    return SBN_SUCCESS;
}/* end SBN_InitInterfaces */
/**
 * This function waits for the scheduler (SCH) to wake this code up, so that
 * nothing transpires until the cFE is fully operational.
 *
 * @param[in] iTimeOut The time to wait for the scheduler to notify this code.
 * @return CFE_SUCCESS on success, otherwise an error value.
 */
static int32 WaitForWakeup(int32 iTimeOut)
{
    int32 Status = CFE_SUCCESS;
    CFE_SB_MsgPtr_t Msg = 0;

    /* Wait for WakeUp messages from scheduler */
    Status = CFE_SB_RcvMsg(&Msg, SBN.CmdPipe, iTimeOut);

    switch(Status)
    {
        case CFE_SB_NO_MESSAGE:
        case CFE_SB_TIME_OUT:
            Status = CFE_SUCCESS;
            break;
        case CFE_SUCCESS:
            SBN_HandleCommand(Msg);
            break;
        default:
            return Status;
    }/* end switch */

    /* For sbn, we still want to perform cyclic processing
    ** if the WaitForWakeup time out
    ** cyclic processing at timeout rate
    */
    CFE_ES_PerfLogEntry(SBN_PERF_RECV_ID);

#ifndef SBN_RECV_TASK
    SBN_RecvNetMsgs();
#endif /* !SBN_RECV_TASK */

    SBN_CheckSubscriptionPipe();

#ifndef SBN_SEND_TASK
    CheckPeerPipes();
#endif /* !SBN_SEND_TASK */

    PeerPoll();

    if(Status == CFE_SB_NO_MESSAGE) Status = CFE_SUCCESS;

    CFE_ES_PerfLogExit(SBN_PERF_RECV_ID);

    return Status;
}/* end WaitForWakeup */

/**
 * Waits for either a response to the "get subscriptions" message from SB, OR
 * an event message that says SB has finished initializing. The latter message
 * means that SB was not started at the time SBN sent the "get subscriptions"
 * message, so that message will need to be sent again.
 * @return TRUE if message received was a initialization message and
 *      requests need to be sent again, or
 * @return FALSE if message received was a response
 */
static int WaitForSBStartup(void)
{
    CFE_EVS_Packet_t *EvsTlm = NULL;
    CFE_SB_MsgPtr_t SBMsgPtr = 0;
    uint8 counter = 0;
    CFE_SB_PipeId_t EventPipe = 0;
    uint32 Status = CFE_SUCCESS;

    /* Create event message pipe */
    Status = CFE_SB_CreatePipe(&EventPipe, 100, "SBNEventPipe");
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to create event pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    /* Subscribe to event messages temporarily to be notified when SB is done
     * initializing
     */
    Status = CFE_SB_Subscribe(CFE_EVS_EVENT_MSG_MID, EventPipe);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to event pipe (%d)", (int)Status);
        return SBN_ERROR;
    }/* end if */

    while(1)
    {
        /* Check for subscription message from SB */
        if(SBN_CheckSubscriptionPipe())
        {
            /* SBN does not need to re-send request messages to SB */
            break;
        }
        else if(counter % 100 == 0)
        {
            /* Send subscription request messages again. This may cause the SB
             * to respond to duplicate requests but that should be okay
             */
            SBN_SendSubsRequests();
        }/* end if */

        /* Check for event message from SB */
        if(CFE_SB_RcvMsg(&SBMsgPtr, EventPipe, 100) == CFE_SUCCESS)
        {
            if(CFE_SB_GetMsgId(SBMsgPtr) == CFE_EVS_EVENT_MSG_MID)
            {
                EvsTlm = (CFE_EVS_Packet_t *)SBMsgPtr;

                /* If it's an event message from SB, make sure it's the init
                 * message
                 */
                if(strcmp(EvsTlm->Payload.PacketID.AppName, "CFE_SB") == 0
                    && EvsTlm->Payload.PacketID.EventID == CFE_SB_INIT_EID)
                {
                    break;
                }/* end if */
            }/* end if */
        }/* end if */

        counter++;
    }/* end while */

    /* Unsubscribe from event messages */
    if(CFE_SB_Unsubscribe(CFE_EVS_EVENT_MSG_MID, EventPipe) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "unable to unsubscribe from event messages");
    }/* end if */

    if(CFE_SB_DeletePipe(EventPipe) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "unable to delete event pipe");
    }/* end if */

    /* SBN needs to re-send request messages */
    return TRUE;
}/* end WaitForSBStartup */

/** \brief SBN Main Routine */
void SBN_AppMain(void)
{
    int     Status = CFE_SUCCESS;
    uint32  RunStatus = CFE_ES_APP_RUN,
            AppID = 0;

    if(CFE_ES_RegisterApp() != CFE_SUCCESS) return;

    if(CFE_EVS_Register(NULL, 0, CFE_EVS_BINARY_FILTER != CFE_SUCCESS)) return;

    if(CFE_ES_GetAppID(&AppID) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_CRITICAL,
            "unable to get AppID");
        return;
    }

    SBN.AppID = AppID;

    /* load the App_FullName so I can ignore messages I send out to SB */
    uint32 TskId = OS_TaskGetId();
    CFE_SB_GetAppTskName(TskId, SBN.App_FullName);

    if(SBN_ReadModuleFile() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "module file not found or data invalid");
        return;
    }/* end if */

    if(SBN_GetPeerFileData() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "peer file not found or data invalid");
        return;
    }/* end if */

    #ifdef SBN_SEND_TASK
    /** Create mutex for send tasks */
    Status = OS_MutSemCreate(&(SBN.SendMutex), "sbn_send_mutex", 0);
    #endif /* SBN_SEND_TASK */

    if(Status != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "error creating mutex for send tasks");
        return;
    }

    if(SBN_InitInterfaces() == SBN_ERROR)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "unable to initialize interfaces");
        return;
    }/* end if */

    /* Create pipe for subscribes and unsubscribes from SB */
    Status = CFE_SB_CreatePipe(&SBN.SubPipe, SBN_SUB_PIPE_DEPTH, "SBNSubPipe");
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to create subscription pipe (Status=%d)", (int)Status);
        return;
    }/* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ALLSUBS_TLM_MID, SBN.SubPipe,
        SBN_MAX_ALLSUBS_PKTS_ON_PIPE);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to allsubs (Status=%d)", (int)Status);
        return;
    }/* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ONESUB_TLM_MID, SBN.SubPipe,
        SBN_MAX_ONESUB_PKTS_ON_PIPE);
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to sub (Status=%d)", (int)Status);
        return;
    }/* end if */

    /* Create pipe for HK requests and gnd commands */
    Status = CFE_SB_CreatePipe(&SBN.CmdPipe, 20, "SBNCmdPipe");
    if(Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to create command pipe (%d)", (int)Status);
        return;
    }/* end if */

    Status = CFE_SB_Subscribe(SBN_CMD_MID, SBN.CmdPipe);
    if(Status == CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
            "Subscribed to command MID 0x%04X", SBN_CMD_MID);
    }
    else
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "failed to subscribe to command pipe (%d)", (int)Status);
        return;
    }/* end if */

    Status = SBN_LoadTbl(&SBN.TblHandle);
    if (Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_ERROR,
            "SBN failed to load SBN.RemapTbl (%d)", Status);
        return;
    }/* end if */

    CFE_TBL_GetAddress((void **)&SBN.RemapTbl, SBN.TblHandle);

    #ifdef SBN_REMAP_ENABLED
    SBN.RemapEnabled = 1;
    #endif /* SBN_REMAP_ENABLED */

    Status = OS_MutSemCreate(&(SBN.RemapMutex), "sbn_remap_mutex", 0);

    CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
        "initialized (CFE_CPU_NAME='%s' ProcessorID=%d SpacecraftId=%d %s "
        "SBN.AppID=%d...",
        CFE_CPU_NAME, CFE_PSP_GetProcessorId(), CFE_PSP_GetSpacecraftId(),
#ifdef SOFTWARE_BIG_BIT_ORDER
        "big-endian",
#else /* !SOFTWARE_BIG_BIT_ORDER */
        "little-endian",
#endif /* SOFTWARE_BIG_BIT_ORDER */
        (int)SBN.AppID);
    CFE_EVS_SendEvent(SBN_INIT_EID, CFE_EVS_INFORMATION,
        "...SBN_IDENT=%s CMD_MID=0x%04X conf=%s)",
        SBN_IDENT,
        SBN_CMD_MID,
#ifdef CFE_ES_CONFLOADER
        "cfe_es_conf"
#else /* !CFE_ES_CONFLOADER */
        "scanf"
#endif /* CFE_ES_CONFLOADER */
    );

    SBN_InitializeCounters();

    /* Wait for event from SB saying it is initialized OR a response from SB
       to the above messages. TRUE means it needs to re-send subscription
       requests */
    if(WaitForSBStartup()) SBN_SendSubsRequests();

    if(Status != CFE_SUCCESS) RunStatus = CFE_ES_APP_ERROR;

    /* Loop Forever */
    while(CFE_ES_RunLoop(&RunStatus)) WaitForWakeup(SBN_MAIN_LOOP_DELAY);

    int NetIdx = 0;
    for(NetIdx = 0; NetIdx < SBN.NetCnt; NetIdx++)
    {
        SBN_NetInterface_t *Net = &SBN.Nets[NetIdx];
        Net->IfOps->UnloadNet(Net);
    }/* end for */

    /* SBN_UnloadModules(); */

    CFE_ES_ExitApp(RunStatus);
}/* end SBN_AppMain */

/**
 * Sends a message to a peer.
 * @param[in] MsgType The type of the message (application data, SBN protocol)
 * @param[in] CpuID The CpuID to send this message to.
 * @param[in] MsgSz The size of the message (in bytes).
 * @param[in] Msg The message contents.
 */
void SBN_ProcessNetMsg(SBN_NetInterface_t *Net, SBN_MsgType_t MsgType,
    SBN_CpuID_t CpuID, SBN_MsgSz_t MsgSize, void *Msg)
{
    int Status = 0;

    SBN_PeerInterface_t *Peer = SBN_GetPeer(Net, CpuID);

    if(!Peer)
    {
        return;
    }/* end if */

    switch(MsgType)
    {
        case SBN_APP_MSG:
            Status = CFE_SB_SendMsgFull(Msg,
                CFE_SB_DO_NOT_INCREMENT, CFE_SB_SEND_ONECOPY);

            if(Status != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(SBN_SB_EID, CFE_EVS_ERROR,
                    "CFE_SB_SendMsg error (Status=%d MsgType=0x%x)",
                    Status, MsgType);
            }/* end if */
            break;

        case SBN_SUBSCRIBE_MSG:
            SBN_ProcessSubsFromPeer(Peer, Msg);
            break;

        case SBN_UN_SUBSCRIBE_MSG:
            SBN_ProcessUnsubsFromPeer(Peer, Msg);
            break;

        /**
         * default: no default as the module may have its own types.
         */
    }/* end switch */
}/* end SBN_ProcessNetMsg */

/**
 * Find the PeerIndex for a given CpuID and net.
 * @param[in] Net The network interface to search.
 * @param[in] ProcessorID The CpuID of the peer being sought.
 * @return The Peer interface pointer, or NULL if not found.
 */
SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, uint32 ProcessorID)
{
    int PeerIdx = 0;

    for(PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        if(Net->Peers[PeerIdx].ProcessorID == ProcessorID)
        {
            return &Net->Peers[PeerIdx];
        }/* end if */
    }/* end for */

    return NULL;
}/* end SBN_GetPeer */
