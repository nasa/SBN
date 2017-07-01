/******************************************************************************
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
 /* #include "app_msgids.h" */
#include <network_includes.h>
#include <string.h>
#include <errno.h>


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
      SBN.DataMsgBuf.Hdr.MsgSender.ProcessorId = SenderPtr->ProcessorId;

    case SBN_SUBSCRIBE_MSG:
    case SBN_UN_SUBSCRIBE_MSG:


      s_addr.sin_port = htons(SBN.Peer[PeerIdx].DataPort);
    
      /* Initialize the SBN hdr of the outgoing network message */
      strncpy((char *)&SBN.DataMsgBuf.Hdr.SrcCpuName,CFE_CPU_NAME,SBN_MAX_PEERNAME_LENGTH);
 
      SBN.DataMsgBuf.Hdr.Type = MsgType;
    
      status = sendto(SBN.DataSockId, (char *)&SBN.DataMsgBuf, MsgSize,
                      0, (struct sockaddr *) &s_addr, sizeof(s_addr) );    

       break;
      
    case SBN_ANNOUNCE_MSG:
    case SBN_ANNOUNCE_ACK_MSG:
    case SBN_HEARTBEAT_MSG:
    case SBN_HEARTBEAT_ACK_MSG:
 
      s_addr.sin_port = htons(SBN.ProtoXmtPort); /* dest port is always the same for each IP addr*/
    
      ProtoMsgBuf.Hdr.Type = MsgType;
      strncpy(ProtoMsgBuf.Hdr.SrcCpuName,CFE_CPU_NAME,SBN_MAX_PEERNAME_LENGTH);

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
     return SBN_TRUE; /* Message available and no errors */
    else
     if ( (status <=0) && (errno != EWOULDBLOCK) ) {
       CFE_EVS_SendEvent(SBN_NET_RCV_PROTO_ERR_EID,CFE_EVS_ERROR,
                         "%s:Socket recv err in CheckForNetProtoMsgs stat=%d,err=%d",
                         CFE_CPU_NAME, status, errno);
       SBN.ProtoMsgBuf.Hdr.Type = SBN_NO_MSG;
       return (-1);
     }

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

    
    
   