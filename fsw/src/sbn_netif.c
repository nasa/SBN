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
#include "sbn_buf.h"
#include "sbn_main_events.h"

#include <network_includes.h>
#include <string.h>
#include <errno.h>

#ifdef _osapi_confloader_
static int SBN_PeerFileRowCallback(const char *filename, int linenum,
    const char *header, const char *row[], int fieldcount, void *opaque)
{
    int ProtocolId = 0, status = 0;

    if(fieldcount < 4)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "SBN %s: Too few fields (%d)",
            CFE_CPU_NAME, fieldcount);
        return OS_SUCCESS;
    }/* end if */

    if(SBN.NumEntries >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "SBN %s: Max entry count reached, skipping.",
            CFE_CPU_NAME);
        return OS_ERROR;
    }/* end if */

    ProtocolId = atoi(row[2]);
    if(ProtocolId < 0 || ProtocolId > SBN_MAX_INTERFACE_TYPES
        || !SBN.IfOps[ProtocolId])
    {   
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "SBN %s: Invalid ProtocolId %d",
            CFE_CPU_NAME, ProtocolId);
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
}

static int SBN_PeerFileErrCallback(const char *filename, int linenum,
    const char *err, void *opaque)
{
    return OS_SUCCESS;
}

int32 SBN_GetPeerFileData(void)
{
    int32 status = 0, id = 0;

    status = OS_ConfLoaderAPIInit();
    if (status != OS_SUCCESS)
    {   
        return status;
    }

    status = OS_ConfLoaderInit(&id, "sbn_peer");
    if (status != OS_SUCCESS)
    {   
        return status;
    }

    status = OS_ConfLoaderSetRowCallback(id, SBN_PeerFileRowCallback, NULL);
    if (status != OS_SUCCESS)
    {
        return status;
    }

    status = OS_ConfLoaderSetErrorCallback(id, SBN_PeerFileErrCallback, NULL);
    if (status != OS_SUCCESS)
    {
        return status;
    }

    OS_ConfLoaderLoad(id, SBN_VOL_PEER_FILENAME);
    OS_ConfLoaderLoad(id, SBN_NONVOL_PEER_FILENAME);

    return SBN_OK;
}

#else /* ! _osapi_confloader_ */

static char* SBN_FindFileEntryAppData(char *entry, int num_fields)
{
    char    *char_ptr = entry;
    int     num_found_fields = 0;

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

/**
 * Parses the peer data file into SBN_FileEntry_t structures.
 * Parses information that is common to all interface types and
 * allows individual interface modules to parse out interface-
 * specfic information.
 *
 * For now, all data from the file is stored together, it will later
 * be divided into peer and host data.
 *
 * @param FileEntry  Interface description line as read from file
 * @param LineNum    The line number in the peer file
 * @return SBN_OK if entry is parsed correctly, SBN_ERROR otherwise
 *
 */
static int SBN_ParseFileEntry(char *FileEntry)
{
    char    Name[SBN_MAX_PEERNAME_LENGTH];
    char    *app_data = NULL;
    uint32  ProcessorId = 0;
    int     ProtocolId = 0;
    uint32  SpaceCraftId = 0;
    uint32  QoS = 0;
    int     ScanfStatus = 0;
    int     status = 0;
    int     NumFields = 5;
    int ProcessorIdInt = 0, ProtocolIdInt = 0, SpaceCraftIdInt = 0, QoSInt = 0;

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
            "%s:Invalid SBN peer file line, "
            "expected %d, found %d",
            CFE_CPU_NAME, NumFields, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    if(SBN.NumEntries >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "SBN %s: Max Peers Exceeded. Max=%d, This=%d.",
            CFE_CPU_NAME, SBN_MAX_NETWORK_PEERS, SBN.NumEntries);
        return SBN_ERROR;
    }/* end if */

    if (ProtocolId < 0 || ProtocolId > SBN_MAX_INTERFACE_TYPES
        || !SBN.IfOps[ProtocolId])
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "SBN %s: Invalid ProtocolId %d",
            CFE_CPU_NAME, ProtocolId);
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
}/* end SBN_ParseFileEntry */

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
            "%s:Opened SBN Peer Data file %s", CFE_CPU_NAME,
            SBN_VOL_PEER_FILENAME);
        FileOpened = TRUE;
    }
    else
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "%s:Failed to open peer file %s", CFE_CPU_NAME,
            SBN_VOL_PEER_FILENAME);
        FileOpened = FALSE;
    }/* end if */

    /* If ram file failed to open, try to open non vol file */
    if(FileOpened == FALSE)
    {
        PeerFile = OS_open(SBN_NONVOL_PEER_FILENAME, O_RDONLY, 0);

        if(PeerFile != OS_ERROR)
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_INFORMATION,
                "%s:Opened SBN Peer Data file %s", CFE_CPU_NAME,
                SBN_NONVOL_PEER_FILENAME);
            FileOpened = TRUE;
        }
        else
        {
            CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
                "%s:Peer file %s failed to open", CFE_CPU_NAME,
                SBN_NONVOL_PEER_FILENAME);
            FileOpened = FALSE;
        }/* end if */
    }/* end if */

    /*
     ** If no file was opened, SBN must terminate
     */
    if(FileOpened == FALSE)
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
        }

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
            if(SBN_ParseFileEntry(SBN_PeerData) == SBN_ERROR)
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

/**
 * Loops through all entries, categorizes them as "Host" or "Peer", and
 * initializes them according to their role and protocol ID.
 *
 * @return SBN_OK if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
int SBN_InitPeerInterface(void)
{
    int32   Stat = 0;
    int     i = 0;
    int32   IFRole = 0; /* host or peer */
    int     PeerIdx = 0;

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
            CFE_PSP_MemSet(&SBN.Peer[SBN.NumPeers], 0, sizeof(SBN.Peer[SBN.NumPeers]));
            SBN.Peer[SBN.NumPeers].IfData = &SBN.IfData[PeerIdx];
            SBN.Peer[SBN.NumPeers].InUse = SBN_IN_USE;

            /* for ease of use, copy some data from the entry into the peer */
            SBN.Peer[SBN.NumPeers].QoS = SBN.IfData[PeerIdx].QoS;
            SBN.Peer[SBN.NumPeers].ProcessorId = SBN.IfData[PeerIdx].ProcessorId;
            SBN.Peer[SBN.NumPeers].ProtocolId = SBN.IfData[PeerIdx].ProtocolId;
            SBN.Peer[SBN.NumPeers].SpaceCraftId = SBN.IfData[PeerIdx].SpaceCraftId;
            strncpy(SBN.Peer[SBN.NumPeers].Name, SBN.IfData[PeerIdx].Name,
                SBN_MAX_PEERNAME_LENGTH);

            Stat = SBN_CreatePipe4Peer(SBN.NumPeers);
            if(Stat == SBN_ERROR)
            {
                CFE_EVS_SendEvent(SBN_PEER_EID, CFE_EVS_ERROR,
                    "%s:Error creating pipe for %s,status=0x%x",
                    CFE_CPU_NAME, SBN.Peer[SBN.NumPeers].Name, Stat);
                return SBN_ERROR;
            }/* end if */

            /* Initialize the subscriptions count for each entry */
            for(i = 0; i < SBN_MAX_SUBS_PER_PEER; i++)
            {
                SBN.Peer[SBN.NumPeers].Sub[i].InUseCtr = SBN_NOT_IN_USE;
            }/* end for */

            /* Reset counters, flags and timers */
            SBN.Peer[SBN.NumPeers].State = SBN_ANNOUNCING;

            SBN_InitMsgBuf(&SBN.Peer[SBN.NumPeers].SentMsgBuf);
            SBN_InitMsgBuf(&SBN.Peer[SBN.NumPeers].DeferredBuf);

            SBN.NumPeers++;
        }
        else
        {
            /* TODO - error */
        }/* end if */
    }/* end for */

    OS_printf("SBN: Num Hosts = %d, Num Peers = %d\n", SBN.NumHosts, SBN.NumPeers);
    return SBN_OK;
}/* end SBN_InitPeerInterface */

/**
 * Sends a message to a peer over the appropriate interface, without buffer
 * or sequence management.
 *
 * @param Msg           Message to send
 * @param PeerIdx       Index of peer data in peer array
 * @return Number of characters sent on success, -1 on error.
 *
 */
int SBN_SendNetMsgNoBuf(SBN_NetPkt_t *Msg, int PeerIdx)
{
    int     status = 0;

    DEBUG_MSG("%s type=%04x size=%d", __FUNCTION__, Msg->Hdr.Type,
        Msg->Hdr.MsgSize);

    status = SBN.IfOps[SBN.Peer[PeerIdx].ProtocolId]->SendNetMsg(
        SBN.Host, SBN.NumHosts, SBN.Peer[PeerIdx].IfData,
        Msg);

    if(status != -1)
    {
        OS_GetLocalTime(&SBN.Peer[PeerIdx].last_sent);
    }
    else
    {
        SBN.HkPkt.PeerAppMsgSendErrCount[PeerIdx]++;
    }/* end if */

    return status;
}/* end SBN_SendNetMsgNoBuf */

/**
 * Sends a message to a peer over the appropriate interface.
 *
 * @param Msg           Message to send
 * @param PeerIdx       Index of peer data in peer array
 * @return Number of characters sent on success, -1 on error.
 *
 */
int SBN_SendNetMsg(SBN_NetPkt_t *Msg, int PeerIdx)
{
    int status = 0;

    DEBUG_START();

    /* if I'm not the sender, don't send (prevent loops) */
    if(Msg->Hdr.MsgSender.ProcessorId != CFE_CPU_ID)
    {
        return SBN_OK;
    }/* end if */

    Msg->Hdr.SequenceCount = SBN.Peer[PeerIdx].SentCount;

    DEBUG_MSG("Sending message %d", Msg->Hdr.SequenceCount);
    
    status = SBN_SendNetMsgNoBuf(Msg, PeerIdx);

    if(status != -1)
    {
        SBN.Peer[PeerIdx].SentCount++;

        SBN_AddMsgToMsgBufOverwrite(Msg,
            &SBN.Peer[PeerIdx].SentMsgBuf);
    }
    else
    {
        SBN.HkPkt.PeerAppMsgSendErrCount[PeerIdx]++;
    }/* end if */

    return status;
}/* end SBN_SendNetMsg */

/**
 * Detects and handles missed messages.  If a message(s) is missed, messages
 * received out of order are deferred until missing messages are received.
 *
 * @param HostIdx  Index of receiving host
 * @param PeerIdx  Index of sending peer
 * @return  The number of missed messages
 */
static uint8 CheckForMissedPkts(SBN_NetPkt_t *msg, int PeerIdx)
{
    uint8   sequenceCount = msg->Hdr.SequenceCount,
            msgsRcvdByUs = SBN.Peer[PeerIdx].RcvdCount,
            numMissed = sequenceCount - msgsRcvdByUs;

    if(numMissed > 0)
    {
        SBN_Hdr_t nackpkt;
        CFE_PSP_MemSet(&nackpkt, 0, sizeof(nackpkt));
        
        SBN.Peer[PeerIdx].RcvdInOrderCount = 0;
        CFE_EVS_SendEvent(SBN_PROTO_EID, CFE_EVS_ERROR,
            "Missed packet! Adding msg %d to deferred buffer",
            sequenceCount);
        SBN_AddMsgToMsgBufSend(msg, &SBN.Peer[PeerIdx].DeferredBuf, PeerIdx);

        nackpkt.SequenceCount = msgsRcvdByUs;
        nackpkt.Type = SBN_COMMAND_NACK_MSG;
        nackpkt.MsgSize = sizeof(nackpkt);
        SBN_SendNetMsgNoBuf((SBN_NetPkt_t *)&nackpkt, PeerIdx);

        return numMissed;
    }
    else if(numMissed == 0)
    {
        SBN.Peer[PeerIdx].RcvdInOrderCount++;
    }/* end if */

    /* if we have received 16 in order, let's ack them */
    if(SBN.Peer[PeerIdx].RcvdInOrderCount == 16)
    {
        SBN_Hdr_t ackpkt;
        CFE_PSP_MemSet(&ackpkt, 0, sizeof(ackpkt));
        ackpkt.SequenceCount = sequenceCount;
        ackpkt.Type = SBN_COMMAND_ACK_MSG;
        ackpkt.MsgSize = sizeof(ackpkt);
        SBN_SendNetMsgNoBuf((SBN_NetPkt_t *)&ackpkt, PeerIdx);
        SBN.Peer[PeerIdx].RcvdInOrderCount = 0;
    }/* end if */

    return numMissed;
}/* end CheckForMissedPkts */

void inline SBN_ProcessNetAppMsgsFromHost(int HostIdx)
{
    int i = 0, status = 0, PeerIdx = 0, missed = 0;

    /* DEBUG_START(); chatty */

    /* Process all the received messages */
    for(i = 0; i <= 100; i++)
    {
        SBN_NetPkt_t msg;
        CFE_PSP_MemSet(&msg, 0, sizeof(msg));

        status = SBN.IfOps[SBN.Host[HostIdx]->ProtocolId]->ReceiveMsg(
            SBN.Host[HostIdx], &msg);
        if(status == SBN_IF_EMPTY)
        {
            break; /* no (more) messages */
        }/* end if */

        if(status == SBN_OK)
        {
            PeerIdx = SBN_GetPeerIndex(msg.Hdr.MsgSender.ProcessorId);
            if(PeerIdx == SBN_ERROR)
            {
                CFE_EVS_SendEvent(SBN_PROTO_EID, CFE_EVS_ERROR,
                    "PeerIdx Bad.  PeerIdx = %d", PeerIdx);
                continue;
            }/* end if */

            OS_GetLocalTime(&SBN.Peer[PeerIdx].last_received);

            switch(msg.Hdr.Type)
            {
                case SBN_COMMAND_ACK_MSG:
                    DEBUG_MSG("ACK, PeerIdx = %d", PeerIdx);
                    SBN_ClearMsgBufBeforeSeq(msg.Hdr.SequenceCount,
                        &SBN.Peer[PeerIdx].SentMsgBuf);
                    break;
                case SBN_COMMAND_NACK_MSG:
                    DEBUG_MSG("NACK, PeerIdx = %d", PeerIdx);
                    SBN_RetransmitSeq(&SBN.Peer[PeerIdx].SentMsgBuf, 
                        msg.Hdr.SequenceCount,
                        PeerIdx);
                    break;
		case SBN_ANNOUNCE_MSG:
                    DEBUG_MSG("HEARTBEAT, PeerIdx = %d", PeerIdx);
		    SBN_ProcessNetAppMsg(&msg);
		    break;
		case SBN_HEARTBEAT_MSG:
                    DEBUG_MSG("HEARTBEAT, PeerIdx = %d", PeerIdx);
		    SBN_ProcessNetAppMsg(&msg);
		    break;
                default:
                    missed = CheckForMissedPkts(&msg, PeerIdx); 
                    if(missed == 0)
                    {
                        SBN_ProcessNetAppMsg(&msg);
                        SBN.Peer[PeerIdx].RcvdCount++;
       
                        /* see if this message closed a gap and send any
                        * deferred messages that immediately follow it */
                        if(SBN.Peer[PeerIdx].DeferredBuf.MsgCount != 0)
                        {
                            SBN_SendConsecutiveFromBuf(
                                &SBN.Peer[PeerIdx].DeferredBuf,
                                msg.Hdr.SequenceCount, PeerIdx);
                        }/* end if */
                    }/* end if */
                    break;
            }/* end switch */
        }
        else if(status == SBN_ERROR)
        {
            // TODO error message
            SBN.HkPkt.PeerAppMsgRecvErrCount[HostIdx]++;
        }/* end if */
    }/* end for */
}/* end SBN_RcvHostMsgs */

/**
 * Checks all interfaces for messages from peers.
 */
void SBN_CheckForNetAppMsgs(void)
{
    int     HostIdx = 0, priority = 0;

    /* DEBUG_START(); chatty */

    while(priority < SBN_MAX_PEER_PRIORITY)
    {
        for(HostIdx = 0; HostIdx < SBN.NumHosts; HostIdx++)
        {
            if(SBN.Host[HostIdx]->IsValid != SBN_VALID)
            {
                continue;
            }/* end if */

            if((int)SBN_GetPriorityFromQoS(SBN.Host[HostIdx]->QoS != priority))
            {
                continue;
            }/* end if */

            SBN_ProcessNetAppMsgsFromHost(HostIdx);
        }/* end for */
        priority++;
    }/* end while */
}/* end SBN_CheckForNetAppMsgs */

void SBN_VerifyPeerInterfaces()
{
    int     PeerIdx = 0, status = 0;

    DEBUG_START();

    for(PeerIdx = 0; PeerIdx < SBN.NumPeers; PeerIdx++)
    {
        status = SBN.IfOps[SBN.Peer[PeerIdx].ProtocolId]->VerifyPeerInterface(
            SBN.Peer[PeerIdx].IfData, SBN.Host, SBN.NumHosts);
        if(status == SBN_VALID)
        {
            SBN.Peer[PeerIdx].IfData->IsValid = SBN_VALID;
        }
        else
        {
            SBN.Peer[PeerIdx].IfData->IsValid = SBN_NOT_VALID;
            SBN.Peer[PeerIdx].InUse = SBN_NOT_IN_USE;
            /* TODO - send EVS Message */
            OS_printf("Peer %s Not Valid\n", SBN.Peer[PeerIdx].IfData->Name);
        }/* end if */
    }/* end for */
}/* end SBN_VerifyPeerInterfaces */

void SBN_VerifyHostInterfaces()
{
    int     HostIdx = 0, status = 0;

    DEBUG_START();

    for(HostIdx = 0; HostIdx < SBN.NumHosts; HostIdx++)
    {
        status = SBN.IfOps[SBN.Host[HostIdx]->ProtocolId]->VerifyHostInterface(
            SBN.Host[HostIdx], SBN.Peer, SBN.NumPeers);

        if(status == SBN_VALID)
        {
            SBN.Host[HostIdx]->IsValid = SBN_VALID;
        }
        else
        {
            SBN.Host[HostIdx]->IsValid = SBN_NOT_VALID;

            /* TODO - send EVS Message */
            OS_printf("Host %s Not Valid\n", SBN.Host[HostIdx]->Name);
        }/* end if */
    }/* end for */
}/* end SBN_VerifyHostInterfaces */

uint8 inline SBN_GetReliabilityFromQoS(uint8 QoS)
{
    DEBUG_START();

    return((QoS & 0xF0) >> 4);
}/* end SBN_GetReliabilityFromQoS */

uint8 inline SBN_GetPriorityFromQoS(uint8 QoS)
{
    /* DEBUG_START(); too chatty */

    return(QoS & 0x0F);
}/* end SBN_GetPriorityFromQoS */

uint8 inline SBN_GetPeerQoSReliability(const SBN_PeerData_t * peer)
{
    /* DEBUG_START(); too chatty */

    return SBN_GetReliabilityFromQoS(peer->QoS);
}/* end SBN_GetPeerQoSReliability */

uint8 inline SBN_GetPeerQoSPriority(const SBN_PeerData_t * peer)
{
    /* DEBUG_START(); too chatty */

    return SBN_GetPriorityFromQoS(peer->QoS);
}/* end SBN_GetPeerQoSPriority */

int SBN_ComparePeerQoS(const void * peer1, const void * peer2)
{
    uint8   peer1_priority = SBN_GetPeerQoSPriority((SBN_PeerData_t*)peer1),
            peer2_priority = SBN_GetPeerQoSPriority((SBN_PeerData_t*)peer2);

    DEBUG_START();

    if(peer1_priority < peer2_priority)
    {
        return 1;
    }/* end if */

    if(peer1_priority > peer2_priority)
    {
        return -1;
    }/* end if */
    return 0;
}/* end SBN_ComparePeerQoS */
