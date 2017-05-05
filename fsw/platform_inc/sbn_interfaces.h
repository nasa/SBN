#ifndef _sbn_interfaces_h_
#define _sbn_interfaces_h_

#include "cfe.h"
#include "sbn_constants.h"
#include "sbn_msg.h"

/*
** Other Structures
*/

/* used in local and peer subscription tables */
typedef struct {
  uint32            InUseCtr;
  CFE_SB_MsgId_t    MsgID;
  CFE_SB_Qos_t      Qos;
} SBN_Subs_t;

typedef uint16 SBN_MsgSize_t;
typedef uint8 SBN_MsgType_t;
typedef uint32 SBN_CpuID_t;

/**
 * Byte arrays to prevent data structure alignment.
 */
typedef struct
{
    /** \brief Size of payload not including SBN header */
    uint8 MsgSizeBuf[sizeof(SBN_MsgSize_t)];

    /** \brief Type of SBN message */
    uint8 MsgTypeBuf[sizeof(SBN_MsgType_t)];

    /** \brief CpuID of sender */
    uint8 CpuIDBuf[sizeof(SBN_CpuID_t)];
}
SBN_PackedHdr_t;

typedef struct
{
    /** \brief CCSDS MsgID of sub */
    uint8 MsgID[sizeof(CFE_SB_MsgId_t)];

    /** \brief CCSDS Qos of sub */
    uint8 Qos[sizeof(CFE_SB_Qos_t)];
}
SBN_PackedSub_t;

typedef struct
{
    char VersionHash[SBN_IDENT_LEN];
    uint16 SubCnt;
    SBN_PackedSub_t Subs[SBN_MAX_SUBS_PER_PEER];
}
SBN_PackedSubs_t;

/**
 * \note The packed size is likely smaller than an in-memory's struct
 * size, as the compiler will align objects.
 * SBN headers are MsgSize + MsgType + CpuID
 * SBN subscription messages are MsgID + Qos
 */
#define SBN_PACKED_HDR_SIZE (sizeof(SBN_PackedHdr_t))
#define SBN_MAX_PACKED_MSG_SIZE (SBN_PACKED_HDR_SIZE + CFE_SB_MAX_SB_MSG_SIZE)

typedef void *SBN_Payload_t;

typedef struct
{
    SBN_PackedHdr_t Hdr;
    uint8 Payload[CFE_SB_MAX_SB_MSG_SIZE];
}
SBN_PackedMsg_t;

/**
 * \brief Used by modules to pack messages to send.
 *
 * \param SBNMsgBuf[out] The buffer pointer to receive the packed message.
 * \param MsgSize[in] The size of the Msg parameter.
 * \param MsgType[in] The type of the Msg (app, sub/unsub, heartbeat, announce).
 * \param CpuID[in] The CPU ID of the sender (should be CFE_CPU_ID)
 * \param Msg[in] The SBN message payload (CCSDS message, sub/unsub)
 */
void SBN_PackMsg(SBN_PackedMsg_t *SBNMsgBuf, SBN_MsgSize_t MsgSize,
    SBN_MsgType_t MsgType, SBN_CpuID_t CpuID, SBN_Payload_t Msg);

/**
 * \brief Used by modules to unpack messages received.
 *
 * \param SBNMsgBuf[in] The buffer pointer containing the SBN message.
 * \param MsgSizePtr[out] The size of the Msg parameter.
 * \param MsgTypePtr[out] The type of the Msg (app, sub/unsub, heartbeat, announce).
 * \param CpuID[out] The CPU ID of the sender (should be CFE_CPU_ID)
 * \param Msg[out] The SBN message payload (CCSDS message, sub/unsub, ensure it is at least CFE_SB_MAX_SB_MSG_SIZE)
 */
void SBN_UnpackMsg(SBN_PackedMsg_t *SBNBuf, SBN_MsgSize_t *MsgSizePtr,
    SBN_MsgType_t *MsgTypePtr, SBN_CpuID_t *CpuIDPtr, SBN_Payload_t Msg);

typedef struct SBN_IfOps_s SBN_IfOps_t;
typedef struct SBN_NetInterface_s SBN_NetInterface_t;

typedef struct {
    SBN_PeerStatus_t Status;
    SBN_NetInterface_t *Net;

    #ifdef SBN_SEND_TASK

    uint32 SendTaskID;

    #endif /* SBN_SEND_TASK */

    #ifdef SBN_RECV_TASK

    uint32 RecvTaskID; /* for mesh nets */

    #endif /* SBN_RECV_TASK */

    CFE_SB_PipeId_t Pipe; /* PipeID */
    char PipeName[OS_MAX_API_NAME]; /* necessary to save? */
    SBN_Subs_t Subs[SBN_MAX_SUBS_PER_PEER + 1]; /* trailing empty */

    /** \brief generic blob of bytes, module-specific */
    uint8 ModulePvt[128];
} SBN_PeerInterface_t;

/**
 * \brief When a module detects a (re)connection, a full subscription update
 * should be sent.
 *
 * \param Peer[in] The peer to which to send the subs.
 */
void SBN_SendLocalSubsToPeer(SBN_PeerInterface_t *Peer);

/**
 * \brief For a given network and processor ID, get the peer interface.
 *
 * \param Net[in] The network to check.
 * \param ProcessorID[in] The processor of the peer.
 *
 * \return A pointer to the peer interface structure.
 */
SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, uint32 ProcessorID);

struct SBN_NetInterface_s {
    boolean Configured;

    #ifdef SBN_SEND_TASK

    uint32 SendTaskID, SendMutex; /* for star nets */

    #endif /* SBN_SEND_TASK */

    #ifdef SBN_RECV_TASK

    uint32 RecvTaskID; /* for star nets */

    #endif /* SBN_RECV_TASK */

    SBN_NetStatus_t Status;

    SBN_IfOps_t *IfOps; /* convenience */

    SBN_PeerInterface_t Peers[SBN_MAX_PEERS_PER_NET];

    /** \brief generic blob of bytes, module-specific */
    uint8  ModulePvt[128];
};

/**
 * This structure contains function pointers to interface-specific versions
 * of the key SBN functions.  Every interface module must have an equivalent
 * structure that points to the approprate functions for that interface.
 */
struct SBN_IfOps_s {
    /** TODO: Document */
    int (*LoadNet)(const char **Row, int FieldCount,
        SBN_NetInterface_t *Net);
    /** TODO: Document */
    int (*LoadPeer)(const char **Row, int FieldCount,
        SBN_PeerInterface_t *Peer);

    /**
     * Initializes the host interface.
     *
     * @param Net Struct pointer describing a single interface
     * @return SBN_SUCCESS on successful initialization
     *         SBN_ERROR otherwise
     */
    int (*InitNet)(SBN_NetInterface_t *Host);

    /**
     * Initializes the peer interface.
     *
     * @param Peer The peer interface to initialize
     * @return SBN_SUCCESS on successful initialization
     *         SBN_ERROR otherwise
     */
    int (*InitPeer)(SBN_PeerInterface_t *Peer);

    /**
     * SBN will poll any peer that does not have any messages to be sent
     * after a timeout period. This is for (re)establishing connections
     * and handshaking subscriptions.
     */
    int (*PollPeer)(SBN_PeerInterface_t *Peer);

    /**
     * Sends a message to a peer over the specified interface.
     * Both protocol and data message buffers are included in the parameters,
     * but only one is used at a time.  The data message buffer is used for
     * un/subscriptions and app messages.  The protocol message buffer is used
     * for announce and heartbeat messages/acks.
     *
     * @param Net Interface data for the network where this peer lives.
     * @param Peer Interface data describing the intended peer recipient.
     * @param MsgType The SBN message type.
     * @param MsgSize The size of the SBN message payload.
     * @param Payload The SBN message payload.
     * @return Number of bytes sent on success, -1 on error.
     */
    int (*Send)(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
        SBN_MsgSize_t MsgSize, SBN_Payload_t Payload);

    /**
     * Receives an individual message from the specified peer. Note, only
     * define this or the RecvFromNet method, not both!
     *
     * @param Net Interface data for the network where this peer lives.
     * @param Peer Interface data describing the intended peer recipient.
     * @param MsgTypePtr SBN message type received.
     * @param MsgSizePtr Payload size received.
     * @param CpuIDPtr CpuID of the sender.
     * @param PayloadBuffer Payload buffer
     *                      (pass in a buffer of CFE_SB_MAX_SB_MSG_SIZE)
     * @return SBN_SUCCESS on success, SBN_ERROR on failure
     */
    int (*RecvFromPeer)(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer,
        SBN_MsgType_t *MsgTypePtr, SBN_MsgSize_t *MsgSizePtr,
        SBN_CpuID_t *CpuIDPtr, SBN_Payload_t PayloadBuffer);

    /**
     * Receives an individual message from the network.
     *
     * @param Net Interface data for the network where this peer lives.
     * @param MsgTypePtr SBN message type received.
     * @param MsgSizePtr Payload size received.
     * @param CpuIDPtr CpuID of the sender.
     * @param PayloadBuffer Payload buffer
     *                      (pass in a buffer of CFE_SB_MAX_SB_MSG_SIZE)
     * @return SBN_SUCCESS on success, SBN_ERROR on failure
     */
    int (*RecvFromNet)(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr,
        SBN_MsgSize_t *MsgSizePtr, SBN_CpuID_t *CpuIDPtr,
        SBN_Payload_t PayloadBuffer);

    /**
     * Reports the status of the module.  The status can be in a module-specific
     * format but must be no larger than SBN_MOD_STATUS_MSG_SIZE bytes (as
     * defined in sbn_platform_cfg.h).  The status packet is passed in
     * initialized (with message ID and size), the module fills it, and upon
     * return the SBN application sends the message over the software bus.
     *
     * @param Buffer Status packet to fill
     * @param Peer Peer to report status
     *
     * @return SBN_NOT_IMPLEMENTED if the module does not implement this
     *         function
     *         SBN_SUCCESS otherwise
     */
    int (*ReportModuleStatus)(SBN_ModuleStatusPacket_t *Buffer);

    /**
     * Resets a specific peer.
     * This function must be present, but can simply return SBN_NOT_IMPLEMENTED
     * if it is not used by or not applicable to a module.
     *
     * @param Peer Peer to report status
     * @return  SBN_SUCCESS when the peer is reset correcly
     *          SBN_ERROR if the peer cannot be reset
     *          SBN_NOT_IMPLEMENTED if the module does not implement this
     *          function
     */
    int (*ResetPeer)(SBN_PeerInterface_t *Peer);
};

#endif /* _sbn_interfaces_h_ */
