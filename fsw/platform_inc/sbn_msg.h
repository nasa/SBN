#ifndef _sbn_msg_h_
#define _sbn_msg_h_

#include "sbn_msgdefs.h"
#include "sbn_platform_cfg.h"
#include "sbn_constants.h"
#include "cfe.h"

/**
 * \brief No Arguments Command
 * For command details see #SBN_NOOP_CC, #SBN_RESET_CC
 * Also see #SBN_SEND_HK_CC
 */
typedef struct {
    uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
} SBN_NoArgsCmd_t;

/**
 * \brief Net Command
 */
typedef struct {
    uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
    uint8 NetIdx;
} SBN_NetCmd_t;

/**
 * \brief Peer Command
 */
typedef struct {
    uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
    uint8 NetIdx;
    uint8 PeerIdx;
} SBN_PeerCmd_t;

typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /** SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte

    uint8 QoS; // 1 + 1 = 2 bytes

    uint8 Padding[2]; // 2 + 2 = 4 bytes

    char Name[SBN_MAX_PEERNAME_LENGTH]; // 4 + 32 = 36 bytes

    uint32 ProcessorID; // 36 + 4 = 40 bytes

    /**
     * \brief Last time a message was sent to the peer (successfully or not).
     */
    OS_time_t LastSend; // 40 + 4 = 44 bytes

    /**
     * \brief Last time a message was received from the peer.
     */
    OS_time_t LastRecv; // 44 + 4 = 48 bytes

    uint16 SendCount; // 52 + 2 = 54 bytes

    uint16 RecvCount; // 54 + 2 = 56 bytes

    uint16 SendErrCount; // 56 + 2 = 58 bytes

    uint16 RecvErrCount; // 58 + 2 = 60 bytes

    uint16 SubCount; // 60 + 2 = 62 bytes

    uint8 IFData[32]; /* opaque, dependent on the interface module */
} SBN_PeerStatus_t;

typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /** SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte

    uint8 Padding[3]; // 1 + 3 = 4 bytes

    char Name[SBN_MAX_NET_NAME_LENGTH]; // 16 + 4 = 20 bytes

    uint8 ProtocolID; // 20 + 1 = 21 bytes

    uint8 PeerCount; // 21 + 1 = 22 bytes

    uint8 Padding2[2]; // 22 + 2 = 24 bytes

    uint8 IFData[32]; /* opaque, dependent on the interface module */
} SBN_NetStatus_t;

/**
 * \brief Housekeeping packet structure
 */
typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /** SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte

    uint8 CmdCount; // 1 + 1 = 2 bytes

    uint8 CmdErrCount; // 2 + 1 = 3 bytes

    uint8 Padding; // 3 + 1 = 4 bytes // 16-bit align below

    uint16 SubCount; // 4 + 2 = 6 bytes

    uint16 NetCount; // 6 + 2 = 8 bytes
} SBN_HkPacket_t;

typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    /** SBN sends all HK responses with the same MID, the CC of the request
     * is copied to the struct of the response to disambiguate what type
     * of response it is.
     */
    uint8 CC; // 1 byte

    uint8 NetIdx; // 1 + 1 = 2 bytes

    uint8 PeerIdx; // 2 + 1 = 3 bytes

    uint8 Padding; // 3 + 1 = 4 bytes

    uint16 SubCount; // 4 + 2 = 6 bytes

    uint16 Padding2[2]; // 6 + 2 = 8 bytes

    CFE_SB_MsgId_t Subs[SBN_MAX_SUBS_PER_PEER];
} SBN_HkSubsPkt_t;

/**
 * \brief Module status response packet structure
 */
typedef struct {
    uint8   TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint32  ProtocolID;
    uint8   ModuleStatus[SBN_MOD_STATUS_MSG_SIZE];
} SBN_ModuleStatusPacket_t;

#endif /* _sbn_msg_h_ */
