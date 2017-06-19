#ifndef _sbn_msg_h_
#define _sbn_msg_h_

#include "sbn_msgdefs.h"
#include "sbn_platform_cfg.h"
#include "sbn_constants.h"
#include "cfe.h"

/**
 * @brief No Arguments Command
 * For command details see #SBN_NOOP_CC, #SBN_RESET_CC
 * Also see #SBN_SEND_HK_CC
 */
typedef struct {
    uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
} SBN_NoArgsCmd_t;

/**
 * @brief Net Command
 */
typedef struct {
    uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
    uint8 NetIdx;
} SBN_NetCmd_t;

/**
 * @brief Peer Command
 */
typedef struct {
    uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
    uint8 NetIdx;
    uint8 PeerIdx;
} SBN_PeerCmd_t;

typedef struct {
    /**
     * This is a struct that will be sent (as-is) in a response to a HK command.
     */
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /**
     * SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte

    /** @brief Quality of Service (unused.) */
    uint8 QoS; // 1 + 1 = 2 bytes

    /** @brief Padding to align the struct to 32-bits.  */
    uint8 Padding[2]; // 2 + 2 = 4 bytes

    /** @brief Name of the peer.  */
    char Name[SBN_MAX_PEERNAME_LENGTH]; // 4 + 32 = 36 bytes

    /** @brief Processor ID of the peer.  */
    uint32 ProcessorID; // 36 + 4 = 40 bytes

    /**
     * @brief Last time a message was sent to the peer (successfully or not).
     */
    OS_time_t LastSend; // 40 + 4 = 44 bytes

    /** @brief Last time a message was received from the peer.  */
    OS_time_t LastRecv; // 44 + 4 = 48 bytes

    /** @brief Number of messages sent to the peer.  */
    uint16 SendCount; // 52 + 2 = 54 bytes

    /** @brief Number of messages received from the peer.  */
    uint16 RecvCount; // 54 + 2 = 56 bytes

    /**
     * @brief Number of times I've encountered an error trying to send
     * to the peer.
     */
    uint16 SendErrCount; // 56 + 2 = 58 bytes

    /**
     * @brief Number of times I've encountered an error trying to receive from
     * the peer.
     */
    uint16 RecvErrCount; // 58 + 2 = 60 bytes

    /**
     * @brief Number of MID's the peer has subscribed to.
     */
    uint16 SubCount; // 60 + 2 = 62 bytes

    /**
     * @brief Opaque data block for the interface module to use.
     */
    uint8 IFData[32];
} SBN_PeerStatus_t;

typedef struct {
    /**
     * This is a struct that will be sent (as-is) in a response to a HK command.
     */
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /**
     * SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte
 
    /** @brief Padding to align the struct to 32-bits.  */
    uint8 Padding[3]; // 1 + 3 = 4 bytes

    /** @brief Name of the network interface. */
    char Name[SBN_MAX_NET_NAME_LENGTH]; // 16 + 4 = 20 bytes

    /** @brief Protocol ID of the protocol used by this network. */
    uint8 ProtocolID; // 20 + 1 = 21 bytes

    /** @brief Number of peers configured for this network. */
    uint8 PeerCount; // 21 + 1 = 22 bytes

    /** @brief More alignment to 32-bits. */
    uint8 Padding2[2]; // 22 + 2 = 24 bytes

    /**
     * @brief Opaque data block for the interface module to use.
     */
    uint8 IFData[32];
} SBN_NetStatus_t;

/**
 * @brief Housekeeping packet structure
 */
typedef struct {
    /**
     * This is a struct that will be sent (as-is) in a response to a HK command.
     */
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /**
     * SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte

    /** @brief Number of commands received. */
    uint8 CmdCount; // 1 + 1 = 2 bytes

    /** @brief Number of command-related errors. */
    uint8 CmdErrCount; // 2 + 1 = 3 bytes

    /** @brief Padding to 32-bits */
    uint8 Padding; // 3 + 1 = 4 bytes

    /** @brief Number of subscriptions by local apps I'm sending to peers. */
    uint16 SubCount; // 4 + 2 = 6 bytes

    /** @brief Number of networks. */
    uint16 NetCount; // 6 + 2 = 8 bytes
} SBN_HkPacket_t;

typedef struct {
    /**
     * This is a struct that will be sent (as-is) in a response to a HK command.
     */
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /**
     * SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte

    /** @brief The network for this subscription. */
    uint8 NetIdx; // 1 + 1 = 2 bytes

    /** @brief The peer for this subscription. */
    uint8 PeerIdx; // 2 + 1 = 3 bytes

    /** @brief Padding to align to 32-bits. */
    uint8 Padding; // 3 + 1 = 4 bytes

    /** @brief Number of subscriptions. */
    uint16 SubCount; // 4 + 2 = 6 bytes

    /** @brief More padding to align to 32-bits. */
    uint16 Padding2[2]; // 6 + 2 = 8 bytes

    /** @brief The subscriptions for this peer. */
    CFE_SB_MsgId_t Subs[SBN_MAX_SUBS_PER_PEER];
} SBN_HkSubsPkt_t;

/**
 * @brief Module status response packet structure
 */
typedef struct {
    /**
     * This is a struct that will be sent (as-is) in a response to a HK command.
     */
    uint8   TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /** @brief The Protocol ID being queried. */
    uint32  ProtocolID;

    /** @brief The module status as returned by the module. */
    uint8   ModuleStatus[SBN_MOD_STATUS_MSG_SIZE];
} SBN_ModuleStatusPacket_t;

#endif /* _sbn_msg_h_ */
