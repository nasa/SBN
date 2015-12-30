#ifndef _sbn_interfaces_h_
#define _sbn_interfaces_h_

#include "cfe.h"
#include "sbn_constants.h"
#include "sbn_msg.h"
#include "sbn_lib_utils.h" /* in sbn_lib */

/*
** Other Structures
*/

/* used in local and peer subscription tables */
typedef struct {
  uint32            InUseCtr;
  CFE_SB_MsgId_t    MsgId;
  CFE_SB_Qos_t      Qos;
} SBN_Subs_t;

typedef struct {
  uint8             Sync[SBN_SYNC_LENGTH]; /* 4 byte sync word, used by modules (optional) */
  uint32            MsgSize; /* total size of message including header */
  uint32            Type;
  char              SrcCpuName[SBN_MAX_PEERNAME_LENGTH]; /* Protocol message originator */
  CFE_SB_SenderId_t MsgSender; /* This is the SB message originator metadata */
  uint16            SequenceCount;
  uint16            GapAfter;
  uint16            GapTo;
  uint16            Padding;
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

/* net msgs on proto port may be either announce, heartbeat, 
 * or cmd ack messages */
typedef struct {
  SBN_Hdr_t         Hdr;
} SBN_NetProtoMsg_t;

/* union used because buffer at data socket used for subs and app pkts*/
typedef union {
    SBN_Hdr_t         Hdr; /* this line to simplify references only */
    SBN_NetPkt_t      Pkt;
    SBN_NetSub_t      Sub;
}NetDataUnion;

typedef struct {
    NetDataUnion        Msgs[SBN_MSG_BUFFER_SIZE];
    int8                Retransmits[SBN_MSG_BUFFER_SIZE];
    int32               AddIndex;
    int32               MsgCount;
    int32               OldestIndex;
} SBN_PeerMsgBuf_t;

typedef struct {
    char   Name[SBN_MAX_PEERNAME_LENGTH];
    uint32 ProtocolId;      /* protocol id from SbnPeerData.dat file */
    uint32 ProcessorId;     /* cpu id from SbnPeerData.dat file */
    uint32 SpaceCraftId;    /* spacecraft id from SbnPeerData.dat file */
    uint8  QoS;             /* QoS from SbnPeerData.dat file */
    uint8  IsValid;         /* used by interfaces that require a match - 1 if match exists, 0 if not */
    int32  EntryData;       /* address of an interface's entry data structure */
    int32  HostData;        /* address of an interface's host data structure */
    int32  PeerData;        /* address of an interface's peer data structure */
} SBN_InterfaceData;


typedef struct {
    uint8             InUse;
    CFE_SB_PipeId_t   Pipe;
    char              PipeName[OS_MAX_API_NAME];
    char              Name[SBN_MAX_PEERNAME_LENGTH];
    uint8             QoS;       /* Quality of Service */
    uint16            SentCount; /* number of msgs sent to this peer */
    uint16            RcvdCount; /* number of msgs received from this peer */
    uint16            MissCount; /* number of msgs missed by this peer */
    uint16            RcvdInOrderCount; /* number of msgs received without misses */
    uint32            ProcessorId;
    uint32            ProtocolId;
    uint32            SpaceCraftId;
    uint32            State;
    uint32            SentLocalSubs;
    uint32            Timer;
    uint32            SubCnt;
    SBN_Subs_t        Sub[SBN_MAX_SUBS_PER_PEER];
    SBN_InterfaceData *IfData;
    SBN_PeerMsgBuf_t  SentMsgBuf;  /* buffer of messages sent over the data interface */
    SBN_PeerMsgBuf_t  DeferredBuf; /* buffer of messages deferred */
} SBN_PeerData_t;

/**
 * This structure contains function pointers to interface-specific versions
 * of the key SBN functions.  Every interface module must have an equivalent
 * structure that points to the approprate functions for that interface.
 */
typedef struct {

    /**
     * Parses a peer data file line into an interface-specific entry structure.
     * Information that is common to all interface types is captured in the SBN
     * and may be captured here or not, at the discretion of the interface
     * developer.
     *
     * @param char*   Interface description line as read from file
     * @param uint32  The line number in the peer file
     * @param int*    The address of the filled entry struct
     * @return SBN_OK if entry is parsed correctly, SBN_ERROR otherwise
     */
    int32 (*ParseInterfaceFileEntry)(char *, uint32, int*);

    /**
     * Initializes the interface, classifies each interface based as a host
     * or a peer.  Hosts are those interfaces that are on the current CPU.
     *
     * @param SBN_InterfaceData* Struct pointer describing a single interface
     * @return SBN_HOST if the interface is a host, SBN_PEER if a peer,
     *         SBN_ERROR otherwise
     */
    int32 (*InitPeerInterface)(SBN_InterfaceData*);

    /**
     * Sends a message to a peer over the specified interface.
     * Both protocol and data message buffers are included in the parameters,
     * but only one is used at a time.  The data message buffer is used for
     * un/subscriptions and app messages.  The protocol message buffer is used
     * for announce and heartbeat messages/acks.
     *
     * @param uint32                 Type of message
     * @param uint32                 Size of message
     * @param SBN_InterfaceData *[]  Array of all host interfaces in the SBN
     * @param int32                  Number of host interfaces in the SBN
     * @param CFE_SB_SenderId_t *    Sender information
     * @param SBN_InterfaceData *    Interface data describing the intended peer recipient
     * @param SBN_NetProtoMsg_t *    Buffer containing a protocol message to send
     * @param NetDataUnion *         Buffer containing a data message to send
     * @return  Number of bytes sent on success, SBN_ERROR on error
     */
    int32 (*SendNetMsg)(uint32, uint32, SBN_InterfaceData *[], int32, CFE_SB_SenderId_t *, SBN_InterfaceData *, SBN_NetProtoMsg_t *, NetDataUnion *);

    /**
     * Checks for a single protocol message.
     *
     * @param SBN_InterfaceData *  Peer from which a protocol message was expected
     * @param SBN_NetProtoMsg_t *  Pointer to the SBN's protocol message buffer
     *                             (received protocol message is put here)
     * @return SBN_TRUE for message received, SBN_NO_MSG for no message, SBN_ERROR for error
     */
    int32 (*CheckForNetProtoMsg)(SBN_InterfaceData *, SBN_NetProtoMsg_t *);

    /**
     * Receives a data message from the specified interface.
     *
     * @param SBN_InterfaceData * Host interface from which to receive a message
     * @param NetDataUnion *      SBN's data message buffer
     *                            (received data message goes here)
     * @return bytes received if message is present, or SBN_IF_EMPTY
     */
    int32 (*ReceiveMsg)(SBN_InterfaceData *, NetDataUnion *);

    /**
     * Iterates through the list of all host interfaces to see if there is a
     * match for the specified peer interface.  This function must be present,
     * but can return SBN_VALID for interfaces that don't require a match.
     *
     * @param SBN_InterfaceData *    Peer to verify
     * @param SBN_InterfaceData *[]  List of hosts to check against the peer
     * @param int32                  Number of hosts in the SBN
     * @return SBN_VALID if the required match exists, SBN_NOT_VALID if not
     */
    int32 (*VerifyPeerInterface)(SBN_InterfaceData *, SBN_InterfaceData *[], int32);

    /**
     * Iterates through the list of all peer interfaces to see if there is a
     * match for the specified host interface.  This function must be present,
     * but can return SBN_VALID for interfaces that don't require a match.
     *
     * @param SBN_InterfaceData *    Host to verify
     * @param SBN_PeerData_t *       List of peers to check against the host
     * @param int32                  Number of peers in the SBN
     * @return SBN_VALID if the required match exists, SBN_NOT_VALID if not
     */
    int32 (*VerifyHostInterface)(SBN_InterfaceData *, SBN_PeerData_t *, int32);

    /**
     * Reports the status of the module.  The status can be in a module-specific
     * format but must be no larger than SBN_MOD_STATUS_MSG_SIZE bytes (as
     * defined in sbn_platform_cfg.h).  The status packet is passed in
     * initialized (with message ID and size), the module fills it, and upon
     * return the SBN application sends the message over the software bus.
     *
     * @param SBN_ModuleStatusPacket_t *  Status packet to fill
     * @param SBN_InterfaceData *         Peer to report status
     * @param SBN_InterfaceData *[]       List of hosts that may match with peer
     * @param int32                       Number of hosts in the SBN
     * @return SBN_NOT_IMPLEMENTED if the module does not implement this
     *         function
     *         SBN_OK otherwise
     */
    int32 (*ReportModuleStatus)(SBN_ModuleStatusPacket_t *, SBN_InterfaceData *, SBN_InterfaceData *[], int32);

    /**
     * Resets a specific peer.
     * This function must be present, but can simply return SBN_NOT_IMPLEMENTED
     * if it is not used by or not applicable to a module.
     *
     * @param SBN_InterfaceData *         Peer to report status
     * @param SBN_InterfaceData *[]       List of hosts that may match with peer
     * @param int32                       Number of hosts in the SBN
     * @return  SBN_OK when the peer is reset correcly
     *          SBN_ERROR if the peer cannot be reset
     *          SBN_NOT_IMPLEMENTED if the module does not implement this
     *          function
     */
    int32 (*ResetPeer)(SBN_InterfaceData *, SBN_InterfaceData *[], int32);

} SBN_InterfaceOperations;


#endif /* _sbn_interfaces_h_ */



