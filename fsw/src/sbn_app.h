/******************************************************************************
** File: sbn_app.h
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
**      This header file contains prototypes for private functions and type
**      definitions for the Software Bus Network Application.
**
** Authors:   J. Wilmot/GSFC Code582
**            R. McGraw/SSI
**
** $Log: sbn_app.h  $
** Revision 1.3 2010/10/05 15:24:14EDT jmdagost 
** Cleaned up copyright symbol.
** Revision 1.2 2008/04/08 08:07:10EDT ruperera 
** Member moved from sbn_app.h in project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/cfs_sbn.pj to sbn_app.h in project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/fsw/src/project.pj.
** Revision 1.1 2008/04/08 07:07:10ACT ruperera 
** Initial revision
** Member added to project c:/MKSDATA/MKS-REPOSITORY/CFS-REPOSITORY/sbn/cfs_sbn.pj
** Revision 1.1 2007/11/30 17:18:03EST rjmcgraw 
** Initial revision
** Member added to project d:/mksdata/MKS-CFS-REPOSITORY/sbn/cfs_sbn.pj
** Revision 1.14 2007/04/19 15:52:53EDT rjmcgraw 
** DCR3052:7 Moved and renamed CFE_SB_MAX_NETWORK_PEERS to this file
** Revision 1.13 2007/03/13 13:18:50EST rjmcgraw 
** Added #define SBN_ITEMS_PER_FILE_LINE
** Revision 1.12 2007/03/07 11:07:23EST rjmcgraw 
** Removed references to channel
** Revision 1.11 2007/02/28 15:05:03EST rjmcgraw 
** Removed unused #defines
** Revision 1.10 2007/02/27 09:37:42EST rjmcgraw 
** Minor cleanup
** Revision 1.9 2007/02/23 14:53:41EST rjmcgraw 
** Moved subscriptions from protocol port to data port
** Revision 1.8 2007/02/22 15:15:50EST rjmcgraw 
** Changed internal structs to exclude mac address
** Revision 1.7 2007/02/21 14:49:39EST rjmcgraw 
** Debug events changed to OS_printfs
**
******************************************************************************/


#ifndef _sbn_app_
#define _sbn_app_

#include "osconfig.h"


/*
** Defines
*/

#define SBN_OK                        0
#define SBN_ERROR                     (-1)

#define SBN_FALSE                     0
#define SBN_TRUE                      1

#define SBN_NOT_IN_USE                0
#define SBN_IN_USE                    1

#define SBN_MSGID_FOUND               0
#define SBN_MSGID_NOT_FOUND           1

#define SBN_MAX_MSG_SIZE              1400
#define SBN_MAX_SUBS_PER_PEER         256 
#define SBN_MAX_PEERNAME_LENGTH       8
#define SBN_DONT_CARE                 0

#define SBN_ETHERNET                  0
#define SBN_SPACEWIRE                 1
#define SBN_1553                      2
#define SBN_NA_BUS                    (-1)

#define SBN_MAIN_LOOP_DELAY           100 // milli-seconds
#define SBN_TIMEOUT_CYCLES            2
#define SBN_PEER_PIPE_DEPTH           64
#define SBN_DEFAULT_MSG_LIM           8
#define SBN_ITEMS_PER_FILE_LINE       6

#define SBN_SUB_PIPE_DEPTH            256
#define SBN_MAX_ONESUB_PKTS_ON_PIPE   256
#define SBN_MAX_ALLSUBS_PKTS_ON_PIPE  64

#define SBN_VOL_PEER_FILENAME         "/ram/apps/SbnPeerData.dat"
#define SBN_NONVOL_PEER_FILENAME      "/cf/apps/SbnPeerData.dat"
#define SBN_PEER_FILE_LINE_SIZE       128
#define SBN_MAX_NETWORK_PEERS         6
#define SBN_NETWORK_MSG_MARGIN        SBN_MAX_NETWORK_PEERS  /* Double it to handle clock skews where a peer */
                                                             /* can send two messages in my single cycle */

#define SBN_SCH_PIPE_DEPTH  10

/* SBN States */
#define SBN_INIT                      0
#define SBN_ANNOUNCING                1
#define SBN_PEER_SYNC                 2
#define SBN_HEARTBEATING              3


/* Message types definitions */
#define SBN_NO_MSG                    0
#define SBN_ANNOUNCE_MSG              0x0010
#define SBN_ANNOUNCE_ACK_MSG          0x0011
#define SBN_HEARTBEAT_MSG             0x0020
#define SBN_HEARTBEAT_ACK_MSG         0x0021
#define SBN_SUBSCRIBE_MSG             0x0030
#define SBN_UN_SUBSCRIBE_MSG          0x0040
#define SBN_APP_MSG                   0x0050

/*
** Event IDs
*/
#define SBN_INIT_EID                  1
#define SBN_MSGID_ERR_EID             2
#define SBN_FILE_ERR_EID              3
#define SBN_PROTO_INIT_ERR_EID        4
#define SBN_VOL_FAILED_EID            5
#define SBN_NONVOL_FAILED_EID         6
#define SBN_INV_LINE_EID              7
#define SBN_FILE_OPENED_EID           8
#define SBN_CPY_ERR_EID               9
#define SBN_PEERPIPE_CR_ERR_EID       10

#define SBN_SOCK_FAIL_EID             12
#define SBN_BIND_FAIL_EID             13
#define SBN_PEERPIPE_CR_EID           14
#define SBN_SND_APPMSG_ERR_EID        15
#define SBN_APPMSG_SENT_EID           16
#define SBN_PEER_ALIVE_EID            17
#define SBN_HB_LOST_EID               18
#define SBN_STATE_ERR_EID             19

#define SBN_MSG_TRUNCATED_EID         21
#define SBN_LSC_ERR1_EID              22
#define SBN_SUBTYPE_ERR_EID           23
#define SBN_NET_RCV_PROTO_ERR_EID     24
#define SBN_SRCNAME_ERR_EID           25
#define SBN_ANN_RCVD_EID              26
#define SBN_ANN_ACK_RCVD_EID          27
#define SBN_HB_RCVD_EID               28
#define SBN_HB_ACK_RCVD_EID           29
#define SBN_SUB_RCVD_EID              30
#define SBN_UNSUB_RCVD_EID            31
#define SBN_PEERIDX_ERR1_EID          32
#define SBN_SUB_ERR_EID               33
#define SBN_DUP_SUB_EID               34
#define SBN_PEERIDX_ERR2_EID          35
#define SBN_NO_SUBS_EID               36
#define SBN_SUB_NOT_FOUND_EID         37
#define SBN_SOCK_RCV_APP_ERR_EID      38
#define SBN_SB_SEND_ERR_EID           39
#define SBN_MSGTYPE_ERR_EID           40
#define SBN_LSC_ERR2_EID              41
#define SBN_ENTRY_ERR_EID             42
#define SBN_PEERIDX_ERR3_EID          43
#define SBN_UNSUB_CNT_EID             44
#define SBN_PROTO_SENT_EID            45
#define SBN_PROTO_SEND_ERR_EID        46

#define SBN_PIPE_ERR_EID              47


/*
** Structures
*/

/* used in local and peer subscription tables */
typedef struct {
  uint32            InUseCtr;
  CFE_SB_MsgId_t    MsgId;
  CFE_SB_Qos_t      Qos;  
}SBN_Subs_t;

typedef struct {
  uint32            Type;
  char              SrcCpuName[SBN_MAX_PEERNAME_LENGTH]; /* Protocol message originator */
  CFE_SB_SenderId_t MsgSender; /* This is the SB message originator metadata */
} SBN_Hdr_t;

typedef struct {
  SBN_Hdr_t         Hdr;
  uint8             Data[SBN_MAX_MSG_SIZE];
} SBN_NetPkt_t;

typedef struct {
  SBN_Hdr_t         Hdr;
  CFE_SB_MsgId_t    MsgId;
  CFE_SB_Qos_t      Qos;
} SBN_NetSub_t;

/* net msgs on proto port may be either announce or heartbeat messages */
typedef struct {
  SBN_Hdr_t         Hdr;
}SBN_NetProtoMsg_t;

typedef struct {
    char            Name[SBN_MAX_PEERNAME_LENGTH];
    uint32          ProcessorId;
    char            Addr[16];
    int             DataPort;
    int             ProtoPort;
} SBN_FileEntry_t;

typedef struct {
    uint8           InUse;
    CFE_SB_PipeId_t Pipe;
    char            PipeName[OS_MAX_API_NAME]; 
    char            Name[SBN_MAX_PEERNAME_LENGTH];
    uint32          ProcessorId;   
    char            Addr[16];
    int             ProtoSockId;/* sending/rcving proto msgs (announce/heartbeat)*/
    int             DataPort;
    int             ProtoRcvPort;
    uint32          State;
    uint32          SentLocalSubs;
    uint32          Timer;    
    uint32          SubCnt;
    SBN_Subs_t      Sub[SBN_MAX_SUBS_PER_PEER];
}SBN_PeerData_t;


typedef struct {
  int               ProtoSockId; /* sending/rcving data msgs (subs/unsubs/pkts) */
  int               ProtoXmtPort;
  SBN_NetProtoMsg_t ProtoMsgBuf;/* rcving proto msgs (announce/heartbeat)*/
  int               DataSockId; /* sending/rcving data msgs (subs/unsubs/pkts) */
  int               DataRcvPort;

  union NetDataUnion{/* union used because buffer at data socket used for subs and app pkts*/
    SBN_Hdr_t         Hdr;/* this line to simplify references only */
    SBN_NetPkt_t      Pkt;
    SBN_NetSub_t      Sub;
  }DataMsgBuf;

/*  SBN_NetDataMsg_u  DataMsgBuf; sending/rcving data msgs (subs/unsubs/pkts) */ 
  uint32            AppId;
  CFE_SB_PipeId_t   SubPipe;
  CFE_SB_PipeId_t   CmdPipe;
  SBN_PeerData_t    Peer[SBN_MAX_NETWORK_PEERS];
  uint32            LocalSubCnt;
  SBN_Subs_t        LocalSubs[SBN_MAX_SUBS_PER_PEER];
  SBN_FileEntry_t   FileData[SBN_MAX_NETWORK_PEERS + 1];/* +1 for "!" terminator */ 
  uint8             DebugOn;

  /* CFE scheduling pipe */
  CFE_SB_PipeId_t  SchPipeId;
  uint16           usSchPipeDepth;
  char             cSchPipeName[OS_MAX_API_NAME];

}sbn_t;


/*
** Prototypes
*/
void  SBN_AppMain(void);
int32 SBN_Init(void);
int32 SBN_RcvMsg(int32 iTimeOut);
int32 SBN_InitProtocol(void);
void  SBN_InitPeerVariables(void);

int32 SBN_CreatePipe4Peer(uint32 PeerIdx);
int32 SBN_SendNetMsg(uint32 MsgType,uint32 MsgSize,uint32 PeerIdx, CFE_SB_SenderId_t *SenderPtr);
void  SBN_RcvNetMsgs(void);
void  SBN_RunProtocol(void);
void  SBN_SendLocalSubsToPeer(uint32 PeerIdx);
void  SBN_CheckPeerPipes(void);
void  SBN_CheckSubscriptionPipe(void);

void  SBN_ProcessNetProtoMsg(void);
void  SBN_ProcessSubFromPeer(uint32 PeerIdx);
void  SBN_ProcessUnsubFromPeer(uint32 PeerIdx);

void  SBN_ProcessNetAppMsg(int MsgLength);
void  SBN_CheckCmdPipe(void);
void  SBN_ProcessLocalSub(CFE_SB_SubEntries_t *Ptr);
void  SBN_ProcessLocalUnsub(CFE_SB_SubEntries_t *Ptr);
void  SBN_ProcessAllSubscriptions(CFE_SB_PrevSubMsg_t *Ptr);
int32 SBN_CheckLocSubs4MsgId(uint32 *IdxPtr,CFE_SB_MsgId_t MsgId);
int32 SBN_CheckPeerSubs4MsgId(uint32 *SubIdxPtr,CFE_SB_MsgId_t MsgId, uint32 PeerIdx);
void  SBN_RemoveAllSubsFromPeer(uint32 PeerIdx);
int32 SBN_GetPeerIndex (char *NamePtr);
void  SBN_ShowStates(void);
char  *SBN_StateNum2Str(uint32 StateNum);
void  SBN_ShowPeerSubs(uint32 PeerIdx);
void  SBN_ShowLocSubs(void);
void  SBN_ShowAllSubs(void);
void  SBN_ShowPeerData(void);
int32 SBN_GetPeerFileData(void);

void  SBN_SendFileOpenedEvent(char *Filename);


void  SBN_NetMsgSendDbgEvt(uint32 MsgType,uint32 PeerIdx,int Status);
void  SBN_NetMsgSendErrEvt(uint32 MsgType,uint32 PeerIdx,int Status);
void  SBN_NetMsgRcvDbgEvt(uint32 MsgType,uint32 PeerIdx,int Status);
void  SBN_NetMsgRcvErrEvt(uint32 MsgType,uint32 PeerIdx,int Status);
char  *SBN_GetMsgName(uint32 MsgType);
void  SBN_SendWakeUpDebugMsg(void);
void  SBN_DebugOn(void);
void  SBN_DebugOff(void);
int32  CFE_SB_SendMsgFull(CFE_SB_Msg_t   *MsgPtr, uint32 TlmCntIncrements, uint32 CopyMode, CFE_SB_SenderId_t *SenderPtr);


#endif /* _sbn_app_ */
/*****************************************************************************/
