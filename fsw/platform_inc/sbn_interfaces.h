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
  CFE_SB_MsgId_t    MsgId;
  CFE_SB_Qos_t      Qos;
} SBN_Subs_t;

typedef uint16 SBN_MsgSize_t;
typedef uint8 SBN_MsgType_t;
typedef uint32 SBN_CpuId_t;

/**
 * Byte arrays to prevent data structure alignment.
 */
typedef struct
{
    /** \brief Size of payload not including SBN header */
    uint8 MsgSizeBuf[sizeof(SBN_MsgSize_t)];

    /** \brief Type of SBN message */
    uint8 MsgTypeBuf[sizeof(SBN_MsgType_t)];

    /** \brief CpuId of sender */
    uint8 CpuIdBuf[sizeof(SBN_CpuId_t)];
}
SBN_PackedHdr_t;

typedef struct
{
    /** \brief CCSDS MsgID of sub */
    uint8 MsgId[sizeof(CFE_SB_MsgId_t)];

    /** \brief CCSDS Qos of sub */
    uint8 Qos[sizeof(CFE_SB_Qos_t)];
}
SBN_PackedSub_t;

/* note that the packed size is likely smaller than an in-memory's struct
 * size, as the CPU will align objects.
 * SBN headers are MsgSize + MsgType + CpuId
 * SBN subscription messages are MsgId + Qos
 */
#define SBN_PACKED_HDR_SIZE (sizeof(SBN_PackedHdr_t))
#define SBN_PACKED_SUB_SIZE (sizeof(SBN_PackedSub_t))
#define SBN_MAX_PACKED_MSG_SIZE (SBN_PACKED_HDR_SIZE + CFE_SB_MAX_SB_MSG_SIZE)

typedef union
{
    SBN_PackedSub_t SBNSub;
    char AnnounceMsg[CFE_SB_MAX_SB_MSG_SIZE];
    uint8 CCSDSMsgBuf[CFE_SB_MAX_SB_MSG_SIZE]; /* ensures enough allocation */
    CFE_SB_Msg_t CCSDSMsg; /* convenience for CCSDS header fields. */
}
SBN_Payload_t;

typedef struct
{
    SBN_PackedHdr_t Hdr;
    SBN_Payload_t Payload;
}
SBN_PackedMsg_t;

/**
 * \brief Used by modules to pack messages to send.
 * \param SBNMsgBuf[out] The buffer pointer to receive the packed message.
 * \param MsgSize[in] The size of the Msg parameter.
 * \param MsgType[in] The type of the Msg (app, sub/unsub, heartbeat, announce).
 * \param CpuId[in] The CPU ID of the sender (should be CFE_CPU_ID)
 * \param Msg[in] The SBN message payload (CCSDS message, sub/unsub)
 */
void SBN_PackMsg(SBN_PackedMsg_t *SBNMsgBuf, SBN_MsgSize_t MsgSize,
    SBN_MsgType_t MsgType, SBN_CpuId_t CpuId, SBN_Payload_t *Msg);

/**
 * \brief Used by modules to unpack messages received.
 * \param SBNMsgBuf[in] The buffer pointer containing the SBN message.
 * \param MsgSizePtr[out] The size of the Msg parameter.
 * \param MsgTypePtr[out] The type of the Msg (app, sub/unsub, heartbeat, announce).
 * \param CpuId[out] The CPU ID of the sender (should be CFE_CPU_ID)
 * \param Msg[out] The SBN message payload (CCSDS message, sub/unsub, ensure it is at least sizeof(SBN_Payload_t) in size.)
 */
void SBN_UnpackMsg(SBN_PackedMsg_t *SBNBuf, SBN_MsgSize_t *MsgSizePtr,
    SBN_MsgType_t *MsgTypePtr, SBN_CpuId_t *CpuIdPtr, SBN_Payload_t *Msg);

typedef struct {
    SBN_HostStatus_t *Status;

    /** \brief generic blob of bytes, interface-specific */
    uint8  InterfacePvt[128];
} SBN_HostInterface_t;

typedef struct {
    SBN_PeerStatus_t *Status;

    CFE_SB_PipeId_t Pipe;
    char PipeName[OS_MAX_API_NAME];
    SBN_Subs_t Subs[SBN_MAX_SUBS_PER_PEER + 1]; /* trailing empty */

    /** \brief generic blob of bytes, interface-specific */
    uint8 InterfacePvt[128];
} SBN_PeerInterface_t;

/**
 * This structure contains function pointers to interface-specific versions
 * of the key SBN functions.  Every interface module must have an equivalent
 * structure that points to the approprate functions for that interface.
 */
typedef struct {
#ifdef _osapi_confloader_
    int (*Load)(const char **, int, void *);
#else /* ! _osapi_confloader_ */
    /**
     * Parses a peer data file line into an interface-specific entry structure.
     * Information that is common to all interface types is captured in the SBN
     * and may be captured here or not, at the discretion of the interface
     * developer.
     *
     * @param Line Interface description line as read from file
     * @param LineNum The line number in the peer file
     * @param EntryBuffer The address of the entry struct to be loaded
     * @return SBN_SUCCESS if entry is parsed correctly, SBN_ERROR otherwise
     */
    int (*Parse)(char *Line, uint32 LineNum, void *EntryBuffer);
#endif /* _osapi_confloader_ */

    /**
     * Initializes the host interface.
     *
     * @param Host Struct pointer describing a single interface
     * @return SBN_SUCCESS on successful initialization
     *         SBN_ERROR otherwise
     */
    int (*InitHost)(SBN_HostInterface_t *Host);

    /**
     * Initializes the peer interface.
     *
     * @param Peer The peer interface to initialize
     * @return SBN_SUCCESS on successful initialization
     *         SBN_ERROR otherwise
     */
    int (*InitPeer)(SBN_PeerInterface_t *Peer);

    /**
     * Sends a message to a peer over the specified interface.
     * Both protocol and data message buffers are included in the parameters,
     * but only one is used at a time.  The data message buffer is used for
     * un/subscriptions and app messages.  The protocol message buffer is used
     * for announce and heartbeat messages/acks.
     *
     * @param Peer Interface data describing the intended peer recipient
     * @param MsgType The SBN message type
     * @param MsgSize The size of the SBN message payload
     * @param Payload The SBN message payload
     * @return Number of bytes sent on success, -1 on error
     */
    int (*Send)(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType,
        SBN_MsgSize_t MsgSize, SBN_Payload_t *Payload);

    /**
     * Receives a data message from the specified interface.
     *
     * @param Peer Peer interface from which to receive a message
     * @param MsgTypePtr SBN message type received.
     * @param MsgSizePtr Payload size received.
     * @param CpuIdPtr CpuId of the sender.
     * @param PayloadBuffer Payload buffer
     *                      (pass in a buffer of SBN_MAX_MSG_SIZE)
     * @return SBN_SUCCESS on success, SBN_ERROR on failure
     */
    int (*Recv)(SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr,
        SBN_MsgSize_t *MsgSizePtr, SBN_CpuId_t *CpuIdPtr,
        SBN_Payload_t *PayloadBuffer);

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

} SBN_InterfaceOperations_t;

#endif /* _sbn_interfaces_h_ */
