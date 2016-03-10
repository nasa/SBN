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

uint8 SBN_CheckForMissedPkts(int PeerIdx);

char* SBN_FindFileEntryAppData(char *entry, int num_fields)
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
int SBN_ParseFileEntry(char *FileEntry, int LineNum)
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

    DEBUG_START();

    app_data = SBN_FindFileEntryAppData(FileEntry, NumFields);

    /* switch on protocol ID */
    ScanfStatus = sscanf(FileEntry, "%s %lu %d %lu %lu" ,
        Name,
        (long unsigned int *) &ProcessorId,
        (int *) &ProtocolId,
        (long unsigned int *) &SpaceCraftId,
        (long unsigned int *) &QoS
        );

    /*
    ** Check to see if the correct number of items were parsed
    */
    if(ScanfStatus != NumFields)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_ERROR,
            "%s:Invalid SBN peer file line,"
            "expected %d, found %d",
            CFE_CPU_NAME, NumFields, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    if(LineNum >= SBN_MAX_NETWORK_PEERS)
    {
        CFE_EVS_SendEvent(SBN_FILE_EID, CFE_EVS_CRITICAL,
            "SBN %s: Max Peers Exceeded. Max=%d, This=%u. "
            "Did not add %s, %u, %u %u\n",
            CFE_CPU_NAME, SBN_MAX_NETWORK_PEERS, LineNum,
            Name, ProcessorId, ProtocolId, SpaceCraftId);
        return SBN_ERROR;
    }/* end if */

    /* Copy over the general info into the interface data structure */
    strncpy(SBN.IfData[LineNum].Name, Name, SBN_MAX_PEERNAME_LENGTH);
    SBN.IfData[LineNum].QoS = QoS;
    SBN.IfData[LineNum].ProtocolId = ProtocolId;
    SBN.IfData[LineNum].ProcessorId = ProcessorId;
    SBN.IfData[LineNum].SpaceCraftId = SpaceCraftId;

    /* Call the correct parse entry function based on the interface type */
    status = SBN.IfOps[ProtocolId]->ParseInterfaceFileEntry(app_data, LineNum,
        (void **)(&SBN.IfData[LineNum].EntryData));

    if(status == SBN_OK)
    {
        SBN.NumEntries++;
    }/* end if */

    return status;
}/* end SBN_ParseFileEntry */

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
            SBN.MsgBuf.Hdr.SequenceCount = 0;
            SBN.Host[SBN.NumHosts] = &SBN.IfData[PeerIdx];
            SBN.NumHosts++;
        }
        else if(IFRole == SBN_PEER)
        {
            memset(&SBN.Peer[SBN.NumPeers], 0, sizeof(SBN.Peer[SBN.NumPeers]));
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
 * @param MsgType       Type of msg (App, Un/Sub, Announce/Ack, Heartbeat/Ack)
 * @param MsgSize       Size of message
 * @param PeerIdx       Index of peer data in peer array
 * @param SenderPtr     Pointer to the message sender
 * @param IsRetransmit  Marks retransmit packets (they are sent as-is)
 * @return Number of characters sent on success, -1 on error.
 *
 */
int SBN_SendNetMsgNoBuf(uint32 MsgType, uint32 MsgSize, int PeerIdx,
    SBN_SenderId_t *SenderPtr)
{
    int     status = 0;

    SBN.MsgBuf.Hdr.Type = MsgType;

    DEBUG_MSG("%s type=%04x size=%d", __FUNCTION__, MsgType, MsgSize);
    
    strncpy(SBN.MsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH);
    SBN.MsgBuf.Hdr.MsgSender.ProcessorId = CFE_CPU_ID;

    SBN.MsgBuf.Hdr.MsgSize = MsgSize;

    status = SBN.IfOps[SBN.Peer[PeerIdx].ProtocolId]->SendNetMsg(MsgType,
	MsgSize, SBN.Host, SBN.NumHosts, SenderPtr, SBN.Peer[PeerIdx].IfData,
        &SBN.MsgBuf);

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
 * @param MsgType       Type of msg (App, Un/Sub, Announce/Ack, Heartbeat/Ack)
 * @param MsgSize       Size of message
 * @param PeerIdx       Index of peer data in peer array
 * @param SenderPtr     Pointer to the message sender
 * @param IsRetransmit  Marks retransmit packets (they are sent as-is)
 * @return Number of characters sent on success, -1 on error.
 *
 */
int SBN_SendNetMsg(uint32 MsgType, uint32 MsgSize, int PeerIdx, SBN_SenderId_t *SenderPtr)
{
    int status = 0;

    DEBUG_START();

    /* if I'm not the sender, don't send (prevent loops) */
    if(SenderPtr->ProcessorId != CFE_CPU_ID)
    {
        return SBN_OK;
    }/* end if */

    strncpy((char *)&(SBN.MsgBuf.Hdr.MsgSender.AppName),
	&SenderPtr->AppName[0], OS_MAX_API_NAME);
    SBN.MsgBuf.Hdr.MsgSender.ProcessorId = SenderPtr->ProcessorId;

    SBN.MsgBuf.Hdr.Type = MsgType;

    strncpy(SBN.MsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
        SBN_MAX_PEERNAME_LENGTH);

    SBN.MsgBuf.Hdr.SequenceCount = SBN.Peer[PeerIdx].SentCount;

    DEBUG_MSG("Sending message %d", SBN.MsgBuf.Hdr.SequenceCount);
    
    status = SBN_SendNetMsgNoBuf(MsgType, MsgSize, PeerIdx, SenderPtr);

    if(status != -1)
    {
        SBN.Peer[PeerIdx].SentCount++;

        SBN_AddMsgToMsgBufOverwrite(&SBN.MsgBuf,
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
uint8 SBN_CheckForMissedPkts(int PeerIdx)
{
    uint8   sequenceCount = SBN.MsgBuf.Hdr.SequenceCount,
            msgsRcvdByUs = SBN.Peer[PeerIdx].RcvdCount,
            numMissed = sequenceCount - msgsRcvdByUs;

    if(numMissed > 0)
    {
        SBN.Peer[PeerIdx].RcvdInOrderCount = 0;
        CFE_EVS_SendEvent(SBN_PROTO_EID, CFE_EVS_ERROR,
            "Missed packet! Adding msg %d to deferred buffer",
            sequenceCount);
        SBN_AddMsgToMsgBufSend(&SBN.MsgBuf, &SBN.Peer[PeerIdx].DeferredBuf,
            PeerIdx);
        SBN.MsgBuf.Hdr.SequenceCount = msgsRcvdByUs;
        SBN_SendNetMsgNoBuf(SBN_COMMAND_NACK_MSG,
                        sizeof(SBN_Hdr_t), PeerIdx, NULL);
        return numMissed;
    }
    else if(numMissed == 0)
    {
        SBN.Peer[PeerIdx].RcvdInOrderCount++;
    }/* end if */

    /*
     * if numMissed < 0, then we assume a duplicate message was received and
     * it is silently discarded
     */

    if(SBN.Peer[PeerIdx].RcvdInOrderCount == 16)
    {
        SBN.MsgBuf.Hdr.SequenceCount = sequenceCount;
        SBN_SendNetMsgNoBuf(SBN_COMMAND_ACK_MSG,
                        sizeof(SBN_Hdr_t), PeerIdx, NULL);
        SBN.Peer[PeerIdx].RcvdInOrderCount = 0;
    }/* end if */

    return numMissed;
}/* end SBN_CheckForMissedPkts */

/**
 * Receives a message from a peer over the appropriate interface.
 *
 * @param HostIdx   Index of host data in host array
 *
 * @return SBN_OK on success, SBN_ERROR on failure
 */
int SBN_IFRcv(int HostIdx)
{
    /* DEBUG_START(); too chatty */

    return SBN.IfOps[SBN.Host[HostIdx]->ProtocolId]->ReceiveMsg(
        SBN.Host[HostIdx], &SBN.MsgBuf);
}/* end SBN_IFRcv */

void inline SBN_ProcessNetAppMsgsFromHost(int HostIdx)
{
    int i = 0, status = 0, PeerIdx = 0, missed = 0;

    /* DEBUG_START(); chatty */

    /* Process all the received messages */
    for(i = 0; i <= 100; i++)
    {
        status = SBN_IFRcv(HostIdx);
        if(status == SBN_IF_EMPTY)
        {
            break; /* no (more) messages */
        }/* end if */

        if(status == SBN_OK)
        {
            PeerIdx = SBN_GetPeerIndex(SBN.MsgBuf.Hdr.MsgSender.ProcessorId);
            if(PeerIdx == SBN_ERROR)
            {
                CFE_EVS_SendEvent(SBN_PROTO_EID, CFE_EVS_ERROR,
                    "PeerIdx Bad.  PeerIdx = %d", PeerIdx);
                continue;
            }/* end if */

            OS_GetLocalTime(&SBN.Peer[PeerIdx].last_received);

            switch(SBN.MsgBuf.Hdr.Type)
            {
                case SBN_COMMAND_ACK_MSG:
                    DEBUG_MSG("ACK, PeerIdx = %d", PeerIdx);
                    SBN_ClearMsgBufBeforeSeq(SBN.MsgBuf.Hdr.SequenceCount,
                        &SBN.Peer[PeerIdx].SentMsgBuf);
                    break;
                case SBN_COMMAND_NACK_MSG:
                    DEBUG_MSG("NACK, PeerIdx = %d", PeerIdx);
                    SBN_RetransmitSeq(&SBN.Peer[PeerIdx].SentMsgBuf, 
                        SBN.MsgBuf.Hdr.SequenceCount,
                        PeerIdx);
                    break;
		case SBN_ANNOUNCE_MSG:
                    DEBUG_MSG("HEARTBEAT, PeerIdx = %d", PeerIdx);
		    SBN_ProcessNetAppMsg(status);
		    break;
		case SBN_HEARTBEAT_MSG:
                    DEBUG_MSG("HEARTBEAT, PeerIdx = %d", PeerIdx);
		    SBN_ProcessNetAppMsg(status);
		    break;
                default:
                    missed = SBN_CheckForMissedPkts(PeerIdx); 
                    if(missed == 0)
                    {
                        SBN_ProcessNetAppMsg(status);
                        SBN.Peer[PeerIdx].RcvdCount++;
       
                        /* see if this message closed a gap and send any
                        * deferred messages that immediately follow it */
                        if(SBN.Peer[PeerIdx].DeferredBuf.MsgCount != 0)
                        {
                            SBN_SendConsecutiveFromBuf(
                                &SBN.Peer[PeerIdx].DeferredBuf,
                                SBN.MsgBuf.Hdr.SequenceCount, PeerIdx);
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
