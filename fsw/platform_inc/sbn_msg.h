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
 * \brief Reset Peer Command
 * For command details see #SBN_RESET_PEER_CC
 */
typedef struct {
    uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
    uint8 PeerIdx;
} SBN_PeerIdxArgsCmd_t;

typedef struct {
    uint8 InUse, QoS, ProtocolId, State; // 4 bytes
    char Name[SBN_MAX_PEERNAME_LENGTH]; // 4 + 8 = 12 bytes
    uint32 ProcessorId, SpaceCraftId; // 12 + 8 = 20 bytes
    OS_time_t LastSent, LastReceived; // 20 + 8 = 28 bytes
    uint16 SentCount, RecvCount, // 28 + 4 = 32 bytes
        SentErrCount, RecvErrCount, SubCount; // 32 + 6 = 38 bytes
    uint8 Padding[2]; // 38 + 2 = 40 bytes
    uint8 IFData[32]; /* opaque, dependent on the interface module */
} SBN_PeerStatus_t;

/**
 * \brief Housekeeping packet structure
 */
typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    uint8 CC; // 1 byte
    uint8 Padding[3]; // 1 + 3 = 4 bytes

    uint16 CmdCount; // 4 + 2 = 6 bytes
    uint16 CmdErrCount; // 6 + 2 = 8 bytes

    uint16 SubCount; // 8 + 2 = 10 bytes

    uint16 EntryCount; // 10 + 2 = 12 bytes
    uint16 HostCount; // 12 + 2 = 14 bytes
    uint16 PeerCount; // 14 + 2 = 16 bytes // want to 32-bit align below

    /* SBN Module Stats */
    SBN_PeerStatus_t PeerStatus[SBN_MAX_NETWORK_PEERS];
} SBN_HkPacket_t;

typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    uint8 CC; // 1 byte
    uint8 Padding[3]; // 1 + 3 = 4 bytes
    uint16 PeerIdx; // 4 + 2 = 6 bytes

    uint32 SubCount; // 6 + 4 = 10 bytes
    uint8 Padding2[2]; // 10  + 2 = 12 bytes // want to 32-bit align below

    CFE_SB_MsgId_t Subs[SBN_MAX_SUBS_PER_PEER];
} SBN_HkSubsPkt_t;
/**
 * \brief Module status response packet structure
 */
typedef struct {
    uint8   TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint32  ProtocolId;
    uint8   ModuleStatus[SBN_MOD_STATUS_MSG_SIZE];
} SBN_ModuleStatusPacket_t;

#endif /* _sbn_msg_h_ */
