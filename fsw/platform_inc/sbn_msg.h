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
    uint8 NetIdx;
    uint8 PeerIdx;
} SBN_PeerIdxArgsCmd_t;

typedef struct {
    uint8 QoS, State, Padding[2]; // 4 bytes
    char Name[SBN_MAX_PEERNAME_LENGTH]; // 4 + 32 = 36 bytes
    uint32 ProcessorID; // 36 + 4 = 40 bytes
    OS_time_t LastSend, LastRecv; // 40 + 8 = 48 bytes
    uint16 SendCount, RecvCount, // 48 + 4 = 52 bytes
        SendErrCount, RecvErrCount, SubCount; // 52 + 6 = 58 bytes
    uint8 IFData[32]; /* opaque, dependent on the interface module */
} SBN_PeerStatus_t;

typedef struct {
    char Name[SBN_MAX_NET_NAME_LENGTH];
    uint8 Idx, ProtocolID, PeerCount; // 4 bytes
    SBN_PeerStatus_t PeerStatus[SBN_MAX_PEERS_PER_NET];
    uint8 IFData[32]; /* opaque, dependent on the interface module */
} SBN_NetStatus_t;

/**
 * \brief Housekeeping packet structure
 */
typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    uint8 CC; // 1 byte

    uint8 CmdCount; // 1 + 1 = 2 bytes
    uint8 CmdErrCount; // 2 + 1 = 3 bytes

    uint8 Padding; // 3 + 1 = 4 bytes // 16-bit align below

    uint16 SubCount; // 4 + 2 = 6 bytes

    uint16 NetCount; // 6 + 2 = 8 bytes

    /* SBN Module Stats */
    SBN_NetStatus_t NetStatus[SBN_MAX_NETS];
} SBN_HkPacket_t;

typedef struct {
    uint8  TlmHeader[CFE_SB_TLM_HDR_SIZE];

    uint8 CC;
    uint8 NetIdx;
    uint8 PeerIdx;
    uint16 SubCount;

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
