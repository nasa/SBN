/******************************************************************************
<<<<<<< HEAD
** File: sbn_netif.c
**
**  Copyright © 2007-2014 United States Government as represented by the 
**  Administrator of the National Aeronautics and Space Administration. 
**  All Other Rights Reserved.  
**
**  This software was created at NASA's Goddard Space Flight Center.
**  This software is governed by the NASA Open Source Agreement and may be 
**  used, distributed and modified only pursuant to the terms of that 
**  agreement.
=======
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
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
**
** Purpose:
**      This file contains source code for the Software Bus Network Application.
**
** Authors:   J. Wilmot/GSFC Code582
**            R. McGraw/SSI
**
** $Log: sbn_app.c  $
<<<<<<< HEAD
** Revision 1.4 2010/10/05 15:24:12EDT jmdagost 
=======
** Revision 1.4 2010/10/05 15:24:12EDT jmdagost
>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
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
<<<<<<< HEAD
 /* #include "app_msgids.h" */
#include <network_includes.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <arpa/inet.h>


/*
**   Task Globals
*/

int32 SBN_ParseFileEntry(char *FileEntry, uint32 LineNum)
{
  char    Name[SBN_MAX_PEERNAME_LENGTH];
  uint32  ProcessorId; 
  uint32  ProtocolId;
  char    Addr[16];
  int     DataPort;
  int     ProtoPort;
  int     ScanfStatus;

   /*
   ** Using sscanf to parse the string.
   ** Currently no error handling
   */
   OS_printf(FileEntry);   
   OS_printf("\n");

   ScanfStatus = sscanf(FileEntry,"%s %d %d %s %d %d",Name, &ProcessorId, &ProtocolId, Addr, &DataPort, &ProtoPort);

   /* Fixme - 1) sscanf needs to be made safe. Use discrete sub functions to safely parse the file 
              2) Different protocol id's will have different line parameters
              3) Need check for my cpu name not found 
   */

   /*
   ** Check to see if the correct number of items were parsed
   */
   if (ScanfStatus != SBN_ITEMS_PER_FILE_LINE) {
     CFE_EVS_SendEvent(SBN_INV_LINE_EID,CFE_EVS_ERROR,
                        "%s:Invalid SBN peer file line,exp %d items,found %d",
                        CFE_CPU_NAME,SBN_ITEMS_PER_FILE_LINE, ScanfStatus);
     return SBN_ERROR;                        
   }/* end if */

  if (ProtocolId != SBN_IPv4) {
     CFE_EVS_SendEvent(SBN_INV_LINE_EID,CFE_EVS_ERROR,
                        "%s:Invalid SBN peer file line,exp %d items,found %d",
                        CFE_CPU_NAME,SBN_ITEMS_PER_FILE_LINE,ScanfStatus);
     return SBN_ERROR;                        
   }/* end if */
    
   if(LineNum < SBN_MAX_NETWORK_PEERS)
   {
     strncpy((char *)&SBN.FileData[LineNum].Name,(char *)&Name,SBN_MAX_PEERNAME_LENGTH);
     strncpy((char *)&SBN.FileData[LineNum].Addr,(char *)&Addr,16);
     
     SBN.FileData[LineNum].ProcessorId = ProcessorId;
     SBN.FileData[LineNum].DataPort    = DataPort;
     SBN.FileData[LineNum].ProtoPort   = ProtoPort;                 
 
     /* Add "!" in case this is the last line read from file */
     /* Declaration of SBN.FileData[SBN_MAX_NETWORK_PEERS + 1] ensures */
     /* writing to array element [LineNum + 1] is safe */
     strncpy((char *)&SBN.FileData[LineNum + 1].Name,"!", SBN_MAX_PEERNAME_LENGTH);
   }
  
   return SBN_OK;

}/* end SBN_ParseFileEntry */

int32 SBN_InitPeerInterface(void)
{
  uint32 PeerIdx = 0;
  uint32 LineNum = 0; 
  int32  Stat;
  uint32 i; 
 

  /* loop through entries in peer data until Name = "!"   loop is bounded because */
  /* ! is put in by software, see end of function SBN_ParseFileEntry */
  while( strncmp(SBN.FileData[LineNum].Name, "!", SBN_MAX_PEERNAME_LENGTH) != 0) {
   
    /* create msg interface when we find entry matching its own name */
    /* because the self entry has port info needed to bind this interface */
    if(strncmp(SBN.FileData[LineNum].Name, CFE_CPU_NAME, SBN_MAX_PEERNAME_LENGTH) == 0){
       SBN.DataRcvPort = SBN.FileData[LineNum].DataPort;
       SBN.DataSockId = SBN_CreateSocket(SBN.DataRcvPort);
       if(SBN.DataSockId == SBN_ERROR){
          return SBN_ERROR;
        }/* end if */

        SBN.ProtoXmtPort = SBN.FileData[LineNum].ProtoPort;
       SBN.ProtoSockId = SBN_CreateSocket(SBN.ProtoXmtPort);
       if(SBN.ProtoSockId == SBN_ERROR){
          return SBN_ERROR;
        }/* end if */

        LineNum++;
        continue;
    }/* end if */

    SBN.Peer[PeerIdx].InUse = SBN_IN_USE;

    Stat = SBN_CopyFileData(PeerIdx,LineNum);
    if(Stat == SBN_ERROR){
        CFE_EVS_SendEvent(SBN_CPY_ERR_EID,CFE_EVS_ERROR,
            "%s:Error copying file data for %s,status=0x%x",
            CFE_CPU_NAME,SBN.Peer[PeerIdx].Name,Stat);
        return SBN_ERROR;
    }/* end if */
 
    SBN.Peer[PeerIdx].ProtoRcvPort = SBN.FileData[LineNum].ProtoPort;
    SBN.Peer[PeerIdx].ProtoSockId  = SBN_CreateSocket(SBN.Peer[PeerIdx].ProtoRcvPort);
 
   if(SBN.Peer[PeerIdx].ProtoSockId == SBN_ERROR){
          return SBN_ERROR;
    }/* end if */
     
    Stat = SBN_CreatePipe4Peer(PeerIdx);
    if(Stat == SBN_ERROR){
        CFE_EVS_SendEvent(SBN_PEERPIPE_CR_ERR_EID,CFE_EVS_ERROR,
            "%s:Error creating pipe for %s,status=0x%x",
            CFE_CPU_NAME,SBN.Peer[PeerIdx].Name,Stat);
        return SBN_ERROR;
    }/* end if */
        
    /* Initialize the subscriptions count for each entry */
    for(i=0;i<SBN_MAX_SUBS_PER_PEER;i++){          
      SBN.Peer[PeerIdx].Sub[i].InUseCtr = SBN_NOT_IN_USE;
    }/* end for */
    
    /* Reset counters, flags and timers */
    SBN.Peer[PeerIdx].Timer = 0;
    SBN.Peer[PeerIdx].SubCnt = 0;
    SBN.Peer[PeerIdx].State = SBN_ANNOUNCING;
    SBN.Peer[PeerIdx].SentLocalSubs = SBN_FALSE;

    PeerIdx++;
    LineNum++;   

  }/* end while */ 

  return SBN_OK;

}/* end SBN_InitPeerInterface */


int SBN_CreateSocket(int Port){
  
  static struct sockaddr_in   my_addr;
  int    SockId;

  if((SockId = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    SBN_SendSockFailedEvent(__LINE__, SockId);         
    return SockId;
  }/* end if */

  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr.sin_port = htons(Port);

  if(bind(SockId, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0 )
  {
    SBN_SendBindFailedEvent(__LINE__,SockId);    
    return SockId;
  }/* end if */
  /* end if */
  
  SBN_ClearSocket(SockId); 
  
  return SockId;   
  
}/* end SBN_CreateSocket */

int32 SBN_CopyFileData(uint32 PeerIdx,uint32 LineNum){

  strncpy(SBN.Peer[PeerIdx].Name,SBN.FileData[LineNum].Name,SBN_MAX_PEERNAME_LENGTH);
  SBN.Peer[PeerIdx].ProcessorId = SBN.FileData[LineNum].ProcessorId;
  strncpy(SBN.Peer[PeerIdx].Addr, SBN.FileData[LineNum].Addr,
                                      sizeof(SBN.FileData[LineNum].Addr));
  SBN.Peer[PeerIdx].DataPort     = SBN.FileData[LineNum].DataPort;
  SBN.Peer[PeerIdx].ProtoRcvPort = SBN.FileData[LineNum].ProtoPort;
  
  return SBN_OK;

}/* end SBN_CopyFileData */


void SBN_SendSockFailedEvent(uint32 Line, int RtnVal){

  CFE_EVS_SendEvent(SBN_SOCK_FAIL_EID,CFE_EVS_ERROR,
                  "%s:socket call failed,line %d,rtn val %d,errno=%d",
                   CFE_CPU_NAME,Line,RtnVal,errno); 

}/* end SBN_SendSockFailedEvent */

void SBN_SendBindFailedEvent(uint32 Line, int RtnVal){

  CFE_EVS_SendEvent(SBN_BIND_FAIL_EID,CFE_EVS_ERROR,
                  "%s:bind call failed,line %d,rtn val %d,errno=%d",
                   CFE_CPU_NAME,Line,RtnVal,errno); 

}/* end SBN_SendBindFailedEvent */     

void SBN_ClearSocket(int SockID)
{
  struct sockaddr_in  s_addr;
  int                 addr_len;
  int                 i;
  int                 status;

  addr_len = sizeof(s_addr);
  bzero((char *) &s_addr, sizeof(s_addr));
   
  /* change to while loop */
  for (i=0; i<=50; i++)
  {
    status = recvfrom(SockID, (char *)&SBN.DataMsgBuf, sizeof(SBN_NetPkt_t),
                           MSG_DONTWAIT,(struct sockaddr *) &s_addr, &addr_len);

    if ( (status < 0) && (errno == EWOULDBLOCK) )
      break; /* no (more) messages */
  } /* end for */

} /* end SBN_ClearSocket */  


int32 SBN_SendNetMsg(uint32 MsgType, uint32 MsgSize, uint32 PeerIdx, CFE_SB_SenderId_t *SenderPtr)
{
  static struct sockaddr_in s_addr;
  int    status;
  SBN_NetProtoMsg_t ProtoMsgBuf;


  bzero((char *) &s_addr, sizeof(s_addr));
  s_addr.sin_family = AF_INET;
  s_addr.sin_addr.s_addr = inet_addr(SBN.Peer[PeerIdx].Addr);

  switch(MsgType){
 
    case SBN_APP_MSG: /* If my peer sent this message, don't send it back to them, avoids loops */
      if (CFE_PSP_GetProcessorId() != SenderPtr->ProcessorId)
         break;
      /* Then no break, so fill in the sender application infomation*/ 
      strncpy((char *)&SBN.DataMsgBuf.Hdr.MsgSender.AppName, &SenderPtr->AppName[0], OS_MAX_API_NAME);
      SBN.DataMsgBuf.Hdr.Type = htonl(MsgType);
      SBN.DataMsgBuf.Hdr.Length = htonl(MsgSize);

      SBN.DataMsgBuf.Hdr.MsgSender.ProcessorId = htonl(SenderPtr->ProcessorId);

    case SBN_SUBSCRIBE_MSG:
    case SBN_UN_SUBSCRIBE_MSG:

      s_addr.sin_port = htons(SBN.Peer[PeerIdx].DataPort);
    
      /* Initialize the SBN hdr of the outgoing network message */
      strncpy((char *)&SBN.DataMsgBuf.Hdr.SrcCpuName,CFE_CPU_NAME,SBN_MAX_PEERNAME_LENGTH);
 
      SBN.DataMsgBuf.Hdr.Type = htonl(MsgType);
      SBN.DataMsgBuf.Hdr.Length = htonl(MsgSize);
    
      status = sendto(SBN.DataSockId, (char *)&SBN.DataMsgBuf, MsgSize,
                      0, (struct sockaddr *) &s_addr, sizeof(s_addr) );    

       break;
      
    case SBN_ANNOUNCE_MSG:
    case SBN_ANNOUNCE_ACK_MSG:
    case SBN_HEARTBEAT_MSG:
    case SBN_HEARTBEAT_ACK_MSG:
 
      s_addr.sin_port = htons(SBN.ProtoXmtPort); /* dest port is always the same for each IP addr*/
    
      strncpy(ProtoMsgBuf.Hdr.SrcCpuName,CFE_CPU_NAME,SBN_MAX_PEERNAME_LENGTH);

      ProtoMsgBuf.Hdr.Type = htonl(MsgType);
      ProtoMsgBuf.Hdr.Length = htonl(MsgSize);
    

      status = sendto(SBN.ProtoSockId, (char *)&ProtoMsgBuf, MsgSize,
                      0, (struct sockaddr *) &s_addr, sizeof(s_addr) );
     break;
      
    default:
      /* send event to indicate unexpected msgtype */
      status = (-1);
      break;
  }/* end switch */

  if(status < 0)
  {
    SBN_NetMsgSendErrEvt(MsgType,PeerIdx,status);
  }else{
    SBN_NetMsgSendDbgEvt(MsgType,PeerIdx,status);
  }/* end if */

  return (status);

}/* end SBN_SendNetMsg */

/* checks for a single protocol message and returns 1 for message available 
   and 0 for no messages or an error 
*/
int32 SBN_CheckForNetProtoMsg(uint32 PeerIdx){

  struct sockaddr_in s_addr;
  int                status;
  int                addr_len;

  bzero((char *) &s_addr, sizeof(s_addr));
  
  addr_len = sizeof(s_addr);
  status = recvfrom(SBN.Peer[PeerIdx].ProtoSockId, (char *)&SBN.ProtoMsgBuf, 
                     sizeof(SBN_NetProtoMsg_t),MSG_DONTWAIT, 
                     (struct sockaddr *) &s_addr, &addr_len);

  if (status > 0) /* Positive number indicates byte length of message */
  {
     uint32 expected_len = ntohl(SBN.ProtoMsgBuf.Hdr.Length);
     if (status < expected_len)
     {
       CFE_EVS_SendEvent(SBN_NET_RCV_PROTO_ERR_EID,CFE_EVS_ERROR,
                         "%s:Socket recv err in CheckForNetProtoMsgs received (%d) < expected (%d)",
                         CFE_CPU_NAME, status, expected_len);
	return SBN_FALSE;
     }
     else
     {
        return SBN_TRUE; /* Message available and no errors */
     }/* end if */
    }/* end if */
    else
    {
     if ( (status <=0) && (errno != EWOULDBLOCK) )
     {
       CFE_EVS_SendEvent(SBN_NET_RCV_PROTO_ERR_EID,CFE_EVS_ERROR,
                         "%s:Socket recv err in CheckForNetProtoMsgs stat=%d,err=%d",
                         CFE_CPU_NAME, status, errno);
       SBN.ProtoMsgBuf.Hdr.Type = SBN_NO_MSG;
       return (-1);
     }/* end if */
   }/* end if */

   /* status = 0, so no messages and no errors */
   SBN.ProtoMsgBuf.Hdr.Type = SBN_NO_MSG;
   return SBN_NO_MSG; 
  
}/* end SBN_CheckForNetProtoMsg */


void SBN_CheckForNetAppMsgs(void){

  struct sockaddr_in s_addr;
  int                i;
  int                status;
  int                addr_len;

  bzero((char *) &s_addr, sizeof(s_addr));

  /* ----------------------------------------------------------------------- */
  /* Process all the received messages                                       */
  /* ----------------------------------------------------------------------- */
  for(i=0; i<=100; i++)
   {
     addr_len = sizeof(s_addr);
     status = recvfrom(SBN.DataSockId, (char *)&SBN.DataMsgBuf, SBN_MAX_MSG_SIZE, MSG_DONTWAIT,
                                    (struct sockaddr *) &s_addr, &addr_len);
     if ( (status < 0) && (errno == EWOULDBLOCK) )
       break; /* no (more) messages */

      if (status > 0)
         SBN_ProcessNetAppMsg(status);
       else /* Check the socket error conditions */
         CFE_EVS_SendEvent(SBN_SOCK_RCV_APP_ERR_EID,CFE_EVS_ERROR,
                          "%s,Socket recv err in CheckForNetAppMsgs errno %d", 
                          CFE_CPU_NAME,errno);
   }

}/* end SBN_CheckForNetAppMsgs */

    
    
   
=======
#include "sbn_buf.h"

#include <network_includes.h>
#include <string.h>
#include <errno.h>

#include <stdlib.h>  /* for qsort */

int32 SBN_CheckForMissedPkts(uint32 HostIdx, int32 PeerIdx);

char * SBN_FindFileEntryAppData(char *entry, int num_fields) {
    char *char_ptr = entry;
    int num_found_fields = 0;

    while(*char_ptr != '\0' && num_found_fields < num_fields) {
        while(*char_ptr != ' ') {
            ++char_ptr;
        }
        ++char_ptr;
        ++num_found_fields;
    }
    return char_ptr;
}

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
int32 SBN_ParseFileEntry(char *FileEntry, uint32 LineNum) {
    char    Name[SBN_MAX_PEERNAME_LENGTH];
    char    *app_data;
    uint32  ProcessorId;
    uint32  ProtocolId;
    uint32  SpaceCraftId;
    uint32  QoS;
    int     ScanfStatus;
    int32   status;

    int     NumFields = 5;
    app_data= SBN_FindFileEntryAppData(FileEntry, NumFields);

    /* switch on protocol ID */
    ScanfStatus = sscanf(
            FileEntry,
            "%s %lu %lu %lu %lu" ,
            Name,
            &ProcessorId,
            &ProtocolId,
            &SpaceCraftId,
            &QoS
            );

    /*
    ** Check to see if the correct number of items were parsed
    */
    if (ScanfStatus != NumFields) {
        CFE_EVS_SendEvent(SBN_INV_LINE_EID,CFE_EVS_ERROR,
                          "%s:Invalid SBN peer file line,"
                          "expected %d, found %d",
                          CFE_CPU_NAME, NumFields, ScanfStatus);
        return SBN_ERROR;
    }

    if(LineNum >= SBN_MAX_NETWORK_PEERS) {
        CFE_EVS_SendEvent(SBN_PEER_LIMIT_EID, CFE_EVS_CRITICAL,
                          "SBN %s: Max Peers Exceeded. Max=%d, This=%u. "
                          "Did not add %s, %u, %u %u\n",
                          CFE_CPU_NAME, SBN_MAX_NETWORK_PEERS, LineNum,
                          Name, ProcessorId, ProtocolId, SpaceCraftId);
        return SBN_ERROR;
    }

    /* Copy over the general info into the interface data structure */
    strncpy(SBN.IfData[LineNum].Name, Name, SBN_MAX_PEERNAME_LENGTH);
    SBN.IfData[LineNum].QoS = QoS;
    SBN.IfData[LineNum].ProtocolId = ProtocolId;
    SBN.IfData[LineNum].ProcessorId = ProcessorId;
    SBN.IfData[LineNum].SpaceCraftId = SpaceCraftId;

    /* Call the correct parse entry function based on the interface type */
    status = SBN.IfOps[ProtocolId]->ParseInterfaceFileEntry(app_data, LineNum,
        (int *)(&SBN.IfData[LineNum].EntryData));

    if(status == SBN_OK) {
        SBN.NumEntries++;
    }

    return status;
}/* end SBN_ParseFileEntry */

/**
 * Loops through all entries, categorizes them as "Host" or "Peer", and
 * initializes them according to their role and protocol ID.
 *
 * @return SBN_OK if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
int32 SBN_InitPeerInterface(void) {
    int32  Stat;
    uint32 i;
    uint32 ProtocolId;
    int32 IFRole; /* host or peer */

    uint32 PeerIdx = 0;
    SBN.NumHosts = 0;
    SBN.NumPeers = 0;

    /* loop through entries in peer data */
    for(PeerIdx = 0; PeerIdx < SBN.NumEntries; PeerIdx++) {

        ProtocolId = SBN.IfData[PeerIdx].ProtocolId;

        /* Call the correct init interface function based on the interface type */
        IFRole = SBN.IfOps[ProtocolId]->InitPeerInterface(&SBN.IfData[PeerIdx]);
        if(IFRole == SBN_HOST) {
            SBN.DataMsgBuf.Hdr.SequenceCount = 0;
            SBN.Host[SBN.NumHosts] = &SBN.IfData[PeerIdx];
            SBN.NumHosts++;
        }
        else if(IFRole == SBN_PEER) {
            SBN.Peer[SBN.NumPeers].SentCount = 0;
            SBN.Peer[SBN.NumPeers].RcvdCount = 0;
            SBN.Peer[SBN.NumPeers].MissCount = 0;
            SBN.Peer[SBN.NumPeers].RcvdInOrderCount = 0;
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
            if(Stat == SBN_ERROR){
                CFE_EVS_SendEvent(SBN_PEERPIPE_CR_ERR_EID,CFE_EVS_ERROR,
                    "%s:Error creating pipe for %s,status=0x%x",
                    CFE_CPU_NAME,SBN.Peer[SBN.NumPeers].Name,Stat);
                return SBN_ERROR;
            }

            /* Initialize the subscriptions count for each entry */
            for(i = 0; i < SBN_MAX_SUBS_PER_PEER; i++){
                SBN.Peer[SBN.NumPeers].Sub[i].InUseCtr = SBN_NOT_IN_USE;
            }

            /* Reset counters, flags and timers */
            SBN.Peer[SBN.NumPeers].Timer = 0;
            SBN.Peer[SBN.NumPeers].SubCnt = 0;
            SBN.Peer[SBN.NumPeers].State = SBN_ANNOUNCING;
            SBN.Peer[SBN.NumPeers].SentLocalSubs = SBN_FALSE;
            
            SBN_InitMsgBuf(&SBN.Peer[SBN.NumPeers].SentMsgBuf);
            SBN_InitMsgBuf(&SBN.Peer[SBN.NumPeers].DeferredBuf);

            SBN.NumPeers++;
        }
        else {
            /* TODO - error */
        }

    }/* end for */

    /* ensure that peers are sorted by QoS priority */
    qsort(SBN.Peer, SBN.NumEntries, sizeof(*(SBN.Peer)), SBN_ComparePeerQoS);

    OS_printf("SBN: Num Hosts = %d, Num Peers = %d\n", SBN.NumHosts, SBN.NumPeers);
    return SBN_OK;

}/* end SBN_InitPeerInterface */

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
int32 SBN_SendNetMsg(uint32 MsgType, uint32 MsgSize, uint32 PeerIdx,
    CFE_SB_SenderId_t *SenderPtr, uint8 IsRetransmit ) {

    int32 status;
    uint32 ProtocolId;
   
    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_SendNetMsg\n");
    }

    /* switch based on protocol type */
    ProtocolId = SBN.Peer[PeerIdx].ProtocolId;


    if(SBN_LIB_MsgTypeIsProto(MsgType)) {
        SBN.DataMsgBuf.Hdr.SequenceCount = 0;
        SBN.ProtoMsgBuf.Hdr.Type = MsgType;
        strncpy(SBN.ProtoMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
                SBN_MAX_PEERNAME_LENGTH);
    }
    else {
        if(MsgType == SBN_COMMAND_ACK_MSG || 
            MsgType == SBN_COMMAND_NACK_MSG) {
            /* special sequence counts are used by ack/nack messages - already 
             * set by caller function */
        }
        else {
            if(IsRetransmit == SBN_TRUE) {
                /* Retransmitted messages should be sent as-is (no updated
                 * sequence count) */
            }
            else {
                if(MsgType == SBN_APP_MSG) {
                    /* If my peer sent this message, don't send it back to them, avoids loops */
                    if (CFE_PSP_GetProcessorId() == SenderPtr->ProcessorId) {
                        strncpy((char *)&(SBN.DataMsgBuf.Hdr.MsgSender.AppName), 
                                 &SenderPtr->AppName[0], 
                                 OS_MAX_API_NAME);
                        SBN.DataMsgBuf.Hdr.MsgSender.ProcessorId = 
                                                SenderPtr->ProcessorId;
                    }
                    else {
                        return SBN_OK;
                    }
                }

                #ifdef TEST_MISSED_MSG
                if(SBN.Peer[PeerIdx].SentCount == 80) {
                    printf("Skipping Message %d\n", SBN.Peer[PeerIdx].SentCount);
                    SBN.DataMsgBuf.Hdr.SequenceCount = SBN.Peer[PeerIdx].SentCount;
                    strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
                         SBN_MAX_PEERNAME_LENGTH);
                    SBN_AddMsgToMsgBufOverwrite(&SBN.DataMsgBuf, &SBN.Peer[PeerIdx].SentMsgBuf);
                    SBN.Peer[PeerIdx].SentCount++;
                }
                #endif
         
                SBN.DataMsgBuf.Hdr.SequenceCount = SBN.Peer[PeerIdx].SentCount;
            
                SBN.DataMsgBuf.Hdr.Type = MsgType;
    
                strncpy(SBN.DataMsgBuf.Hdr.SrcCpuName, CFE_CPU_NAME,
                           SBN_MAX_PEERNAME_LENGTH);
            }
        }
    }
    
    status = SBN.IfOps[ProtocolId]->SendNetMsg(MsgType, MsgSize, SBN.Host,
        SBN.NumHosts, SenderPtr,
        SBN.Peer[PeerIdx].IfData,
        &SBN.ProtoMsgBuf,
        &SBN.DataMsgBuf);

    /* record result in HK telemetry (number of characters on success, -1 on
     * error) */
    if(status != -1) {
        if (SBN_LIB_MsgTypeIsProto(MsgType)) {
            SBN.HkPkt.PeerProtoMsgSendCount[PeerIdx]++;
        } 
        else {
            /* successful app message, bad type won't return success */
            if(MsgType != SBN_COMMAND_ACK_MSG && 
               MsgType != SBN_COMMAND_NACK_MSG &&
               IsRetransmit == SBN_FALSE) {
                SBN.Peer[PeerIdx].SentCount++;
                SBN_AddMsgToMsgBufOverwrite(&SBN.DataMsgBuf, &SBN.Peer[PeerIdx].SentMsgBuf);
            }
        }
    } 
    else {
        if (SBN_LIB_MsgTypeIsProto(MsgType)) {
            SBN.HkPkt.PeerProtoMsgSendErrCount[PeerIdx]++;
        } 
        else { /* unsuccessful app message or unknown type */
            SBN.HkPkt.PeerAppMsgSendErrCount[PeerIdx]++;
        }
    }

    return status;
}/* end SBN_SendNetMsg */

/**
 * Checks for a single protocol message.
 *
 * @param  PeerIdx   Index of peer data in peer array
 *
 * @return 1 for message available, 0 for no messages, -1 for error
 */
int32 SBN_CheckForNetProtoMsg(uint32 PeerIdx) {

    int32 status;
    uint32 ProtocolId;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckForNetProtoMsg\n");
    }
        
    /* switch based on protocol type */
    ProtocolId = SBN.Peer[PeerIdx].ProtocolId;

    status =
      SBN.IfOps[ProtocolId]->CheckForNetProtoMsg(SBN.Peer[PeerIdx].IfData,
                                                 &SBN.ProtoMsgBuf);

    /* update HK telemetry with result */
    if(status == 1) { /* message available and ok */
        SBN.HkPkt.PeerProtoMsgRecvCount[PeerIdx]++;
    }
    else if(status == -1) { /* receive error */
        SBN.HkPkt.PeerProtoMsgRecvErrCount[PeerIdx]++;
    }
    /* no counter for no messages condition */

    return status;
}/* end SBN_CheckForNetProtoMsg */

/**
 * Detects and handles missed messages.  If a message(s) is missed, messages
 * received out of order are deferred until missing messages are received.
 *
 * @param HostIdx  Index of receiving host
 * @param PeerIdx  Index of sending peer
 * @return  The number of missed messages
 */
int32 SBN_CheckForMissedPkts(uint32 HostIdx, int32 PeerIdx) {
    uint16 msgsSentToUs, msgsRcvdByUs;
    int32 numMissed;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckForMissedPkts\n");
    }

    msgsSentToUs = SBN.DataMsgBuf.Hdr.SequenceCount;
   
    PeerIdx = SBN_GetPeerIndex(SBN.DataMsgBuf.Hdr.SrcCpuName);
    if(PeerIdx == SBN_ERROR) {
        printf("SBN_CheckForMissedPkts: SrcCpuName = %s\n", SBN.DataMsgBuf.Hdr.SrcCpuName);
        printf("SBN_CheckForMissedPkts: PeerIdx = %u\n", PeerIdx);

        // TODO event message
        return;
    }
    msgsRcvdByUs = SBN.Peer[PeerIdx].RcvdCount;
    printf("SBN_CheckForMissedPkts: Received %u\n", msgsSentToUs);
    numMissed = msgsSentToUs - msgsRcvdByUs;
    if(numMissed > 0) {
        printf("Missed Packet!  Expected %u, Received %u\n", msgsRcvdByUs,  msgsSentToUs);
        // TODO event message
        SBN.Peer[PeerIdx].RcvdInOrderCount = 0;
        printf("Adding Msg %d to Deferred Buffer\n", msgsSentToUs);
        SBN_AddMsgToMsgBufSend(&SBN.DataMsgBuf, &SBN.Peer[PeerIdx].DeferredBuf, PeerIdx);
        SBN.DataMsgBuf.Hdr.SequenceCount = msgsRcvdByUs;
        SBN_SendNetMsg(SBN_COMMAND_NACK_MSG,
                        sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL, SBN_FALSE);
        return numMissed;
    }
    else if(numMissed == 0){
        SBN.Peer[PeerIdx].RcvdInOrderCount++;
    }
    /*
     * if numMissed < 0, then we assume a duplicate message was received and
     * it is silently discarded
     */

    if(SBN.Peer[PeerIdx].RcvdInOrderCount == 16) {
        SBN.DataMsgBuf.Hdr.SequenceCount = msgsSentToUs;
        SBN_SendNetMsg(SBN_COMMAND_ACK_MSG,
                        sizeof(SBN_NetProtoMsg_t), PeerIdx, NULL, SBN_FALSE);
        SBN.Peer[PeerIdx].RcvdInOrderCount = 0;
    }

    return numMissed;
}

/**
 * Receives a message from a peer over the appropriate interface.
 *
 * @param HostIdx   Index of host data in host array
 *
 * @return Bytes received on success, SBN_IF_EMPTY if empty, SBN_ERROR on error
 */
int SBN_IFRcv(int HostIdx) {

    int32 status;
    uint32 ProtocolId;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_IFRcv\n");
    }

    ProtocolId = SBN.Host[HostIdx]->ProtocolId;

    status = SBN.IfOps[ProtocolId]->ReceiveMsg(SBN.Host[HostIdx],
                                                     &SBN.DataMsgBuf);

    return status;
}

void inline SBN_ProcessNetAppMsgsFromHost(int HostIdx) {
    int32 i, status, PeerIdx, missed;

    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_ProcessNetAppMsgsFromHost\n");
    }

    /* Process all the received messages */
    for(i = 0; i <= 100; i++) {
        status = SBN_IFRcv(HostIdx);
        if(status == SBN_IF_EMPTY)
            return; /* no (more) messages */

        if (status > 0) {
            PeerIdx = SBN_GetPeerIndex(SBN.DataMsgBuf.Hdr.SrcCpuName);
            if(PeerIdx == SBN_ERROR) {
                printf("PeerIdx Bad.  PeerIdx = %d\n", PeerIdx);
                continue;
            }

            if(SBN.DataMsgBuf.Hdr.Type == SBN_COMMAND_ACK_MSG) {
                printf("Got ACK for packet %u\n", SBN.DataMsgBuf.Hdr.SequenceCount);
                SBN_ClearMsgBufBeforeSeq(&SBN.ProtoMsgBuf, &SBN.Peer[PeerIdx].SentMsgBuf);
            }
            else if(SBN.DataMsgBuf.Hdr.Type == SBN_COMMAND_NACK_MSG) {
                printf("Got NACK for packet %u\n", SBN.DataMsgBuf.Hdr.SequenceCount);
                SBN_RetransmitSeq(&SBN.Peer[PeerIdx].SentMsgBuf, 
                                  SBN.DataMsgBuf.Hdr.SequenceCount,
                                  PeerIdx);
            }
            else {
                missed = SBN_CheckForMissedPkts(HostIdx, PeerIdx); 
                if(missed == 0) {
                    SBN_ProcessNetAppMsg(status);
                    SBN.Peer[PeerIdx].RcvdCount++;
                    
                    /* see if this message closed a gap and send any deferred
                     * messages that immediately follow it */
                    if(SBN.Peer[PeerIdx].DeferredBuf.MsgCount != 0) {
                        SBN_SendConsecutiveFromBuf(&SBN.Peer[PeerIdx].DeferredBuf,
                                SBN.DataMsgBuf.Hdr.SequenceCount, PeerIdx);
                    }
                }
                //SBN.Peer[PeerIdx].RcvdCount++;
            }
        }
        else if(status == SBN_ERROR) {
            // TODO error message
            SBN.HkPkt.PeerAppMsgRecvErrCount[HostIdx]++;
        }
    }
}/* end SBN_RcvHostMsgs */

/**
 * Checks all interfaces for messages from peers.
 */
void SBN_CheckForNetAppMsgs(void) {
    int HostIdx;
    int32 priority;
   
    if(SBN.DebugOn == SBN_TRUE) {
        printf("SBN_CheckForNetAppMsgs\n");
    }

    priority = 0;

    while(priority < SBN_MAX_PEER_PRIORITY) {
        for(HostIdx = 0; HostIdx < SBN.NumHosts; HostIdx++) {
            if (SBN.Host[HostIdx]->IsValid != SBN_VALID)
                continue;

            if((int)SBN_GetPriorityFromQoS(SBN.Host[HostIdx]->QoS != priority))
                continue;

            SBN_ProcessNetAppMsgsFromHost(HostIdx);
        }
        priority++;
    }
}/* end SBN_CheckForNetAppMsgs */

void SBN_VerifyPeerInterfaces() {
    int32 PeerIdx, status;
    int32 ProtocolId;
    for(PeerIdx = 0; PeerIdx < SBN.NumPeers; PeerIdx++) {
        ProtocolId = SBN.Peer[PeerIdx].ProtocolId;
        status =
          SBN.IfOps[ProtocolId]->VerifyPeerInterface(SBN.Peer[PeerIdx].IfData,
                                                     SBN.Host, SBN.NumHosts);
        if(status == SBN_VALID) {
            SBN.Peer[PeerIdx].IfData->IsValid = SBN_VALID;
        }
        else {
            SBN.Peer[PeerIdx].IfData->IsValid = SBN_NOT_VALID;
            SBN.Peer[PeerIdx].InUse = SBN_NOT_IN_USE;
            /* TODO - send EVS Message */
            printf("Peer %s Not Valid\n", SBN.Peer[PeerIdx].IfData->Name);
        }
    }
}

void SBN_VerifyHostInterfaces() {
    int32 HostIdx, status, ProtocolId;
    for(HostIdx = 0; HostIdx < SBN.NumHosts; HostIdx++) {
        ProtocolId = SBN.Host[HostIdx]->ProtocolId;
        status =
          SBN.IfOps[ProtocolId]->VerifyHostInterface(SBN.Host[HostIdx],
              SBN.Peer, SBN.NumPeers);

        if(status == SBN_VALID) {
            SBN.Host[HostIdx]->IsValid = SBN_VALID;
        }
        else {
            SBN.Host[HostIdx]->IsValid = SBN_NOT_VALID;

            /* TODO - send EVS Message */
            printf("Host %s Not Valid\n", SBN.Host[HostIdx]->Name);
        }
    }
}

uint8 inline SBN_GetReliabilityFromQoS(uint8 QoS) {
    return ((QoS & 0xF0) >> 4);
}

uint8 inline SBN_GetPriorityFromQoS(uint8 QoS) {
    return (QoS & 0x0F);
}

uint8 inline SBN_GetPeerQoSReliability(const SBN_PeerData_t * peer) {
    uint8 peer_qos = peer->QoS;

    return SBN_GetReliabilityFromQoS(peer_qos);
}

uint8 inline SBN_GetPeerQoSPriority(const SBN_PeerData_t * peer) {
    uint8 peer_qos = peer->QoS;

    return SBN_GetPriorityFromQoS(peer_qos);
}

int SBN_ComparePeerQoS(const void * peer1, const void * peer2) {
    uint8 peer1_priority = SBN_GetPeerQoSPriority((SBN_PeerData_t*)peer1);
    uint8 peer2_priority = SBN_GetPeerQoSPriority((SBN_PeerData_t*)peer2);

    if(peer1_priority < peer2_priority)
        return 1;
    if(peer1_priority > peer2_priority)
        return -1;
    return 0;
}


>>>>>>> e2afa232589b8cb2d665d5cc6e7269fc626a6a30
