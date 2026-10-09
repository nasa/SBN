/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "sbn_app.h"
#include "sbn_pack.h"

/* NOTE: These lengths are only used for the non-EDS/direct packing mode.
 * When built with EDS the library handles all of this on the FSW side.  However
 * for unit testing purposes it still packs using this method. */
#define SBN_CMD_NET_LEN sizeof(CFE_MSG_CommandHeader_t) + sizeof(SBN_NetIdx_t)

#define SBN_CMD_PEER_LEN sizeof(CFE_MSG_CommandHeader_t) + sizeof(SBN_NetIdx_t) + sizeof(SBN_PeerIdx_t)

/** @brief CC, CmdCnt, CmdErrCnt, SubCnt, NetCnt */
#define SBN_HK_LEN (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + (sizeof(SBN_HkCounter_t) * 4))

/** @brief CC, SBN_SubCnt_t SubCnt, CFE_SB_MsgId_t Subs[SBN_MISSION_MAX_SUBS_PER_PEER] */
#define SBN_HKMYSUBS_LEN                                                      \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_SubCnt_t) \
     + SBN_MISSION_MAX_SUBS_PER_PEER * sizeof(CFE_SB_MsgId_t))

/** @brief CC, NetIdx, PeerIdx, SubCnt, Subs[SBN_MISSION_MAX_SUBS_PER_PEER] */
#define SBN_HKPEERSUBS_LEN                                                                            \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_NetIdx_t) + sizeof(SBN_PeerIdx_t) \
     + sizeof(SBN_SubCnt_t) + SBN_MISSION_MAX_SUBS_PER_PEER * sizeof(CFE_SB_MsgId_t))

/** @brief CC, SubCnt, ProcessorID, LastSend, LastRecv, SendCnt, RecvCnt, SendErrCnt, RecvErrCnt */
#define SBN_HKPEER_LEN                                                                                    \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_SubCnt_t) + sizeof(SBN_ProcessorID_t) \
     + sizeof(SBN_Timestamp_t) * 2 + sizeof(SBN_HkCounter_t) * 4)

/** @brief CC, ProtocolID, PeerCnt */
#define SBN_HKNET_LEN \
    (sizeof(CFE_MSG_TelemetryHeader_t) + sizeof(uint8) + sizeof(SBN_ModuleIdx_t) + sizeof(SBN_PeerIdx_t))

SBN_TlmBase_t *SBN_Encode_GetBuffer(size_t MsgSize)
{
    /* instantiate a local buffer big enough to hold any TLM message we generate */
    static union
    {
        SBN_TlmBase_t       TlmBase;
        SBN_HkTlm_t         HkTlmBuf;
        SBN_HkNetTlm_t      HkNetTlmBuf;
        SBN_HkPeerTlm_t     HkPeerTlmBuf;
        SBN_HkMySubsTlm_t   HkMySubsTlmBuf;
        SBN_HkPeerSubsTlm_t HkPeerSubsTlmBuf;
    } LocalTempBuffer;

    if (MsgSize > sizeof(LocalTempBuffer))
    {
        return NULL;
    }

    return &LocalTempBuffer.TlmBase;
}

void SBN_Encode_ReleaseBuffer(const SBN_TlmBase_t *TlmBuf)
{
    /* no-op, the buffers are statically allocated */
}

static CFE_SB_Buffer_t *SBN_Encode_DoPackInit(Pack_t *Pack, size_t BufSz, const SBN_TlmBase_t *Input)
{
    CFE_SB_Buffer_t *SbBufPtr;

    SbBufPtr = CFE_SB_AllocateMessageBuffer(BufSz);
    if (SbBufPtr != NULL)
    {
        Pack_Init(Pack, SbBufPtr, BufSz, true);

        /* Pass through the header as-is */
        Pack_Data(Pack, &Input->TelemetryHeader, sizeof(Input->TelemetryHeader));

        /* Pack the ID byte (always present) */
        Pack_UInt8(Pack, Input->TlmId);
    }

    return SbBufPtr;
}

static void SBN_Encode_HkTlm(Pack_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkTlm_t *Input = (const SBN_HkTlm_t *)MsgPtr;

    Pack_UInt16(Pack, Input->Payload.CmdCnt);
    Pack_UInt16(Pack, Input->Payload.CmdErrCnt);
    Pack_UInt16(Pack, Input->Payload.SubCnt);
    Pack_UInt16(Pack, Input->Payload.NetCnt);

} /* end HKCmd */

static void SBN_Encode_HkNetTlm(Pack_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkNetTlm_t *Input = (const SBN_HkNetTlm_t *)MsgPtr;

    Pack_UInt8(Pack, Input->Payload.ProtocolIdx);
    Pack_UInt16(Pack, Input->Payload.PeerCnt);

} /* end HKNetCmd */

static void SBN_Encode_HkPeerTlm(Pack_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkPeerTlm_t *Input = (const SBN_HkPeerTlm_t *)MsgPtr;

    Pack_UInt32(Pack, Input->Payload.ProcessorId);
    Pack_Time(Pack, Input->Payload.LastSend);
    Pack_Time(Pack, Input->Payload.LastRecv);
    Pack_UInt16(Pack, Input->Payload.SendCnt);
    Pack_UInt16(Pack, Input->Payload.RecvCnt);
    Pack_UInt16(Pack, Input->Payload.SendErrCnt);
    Pack_UInt16(Pack, Input->Payload.RecvErrCnt);
    Pack_UInt16(Pack, Input->Payload.SubCnt);

} /* end HKPeerCmd */

static void SBN_Encode_HkMySubsTlm(Pack_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkMySubsTlm_t *Input = (const SBN_HkMySubsTlm_t *)MsgPtr;
    SBN_SubCnt_t             i;

    Pack_UInt16(Pack, Input->Payload.SubCnt);
    for (i = 0; i < SBN_AppData.SubCnt; i++)
    {
        Pack_MsgID(Pack, Input->Payload.Subs[i]);
    }
} /* end MySubsCmd */

static void SBN_Encode_HkPeerSubsTlm(Pack_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkPeerSubsTlm_t *Input = (const SBN_HkPeerSubsTlm_t *)MsgPtr;
    SBN_SubCnt_t               i;

    Pack_UInt16(Pack, Input->Payload.NetIdx);
    Pack_UInt16(Pack, Input->Payload.PeerIdx);
    Pack_UInt16(Pack, Input->Payload.SubCnt);

    for (i = 0; i < Input->Payload.SubCnt; i++)
    {
        Pack_MsgID(Pack, Input->Payload.Subs[i]);
    }
} /* end PeerSubsCmd */

CFE_SB_Buffer_t *SBN_Encode_Output(const SBN_TlmBase_t *MsgPtr)
{
    CFE_SB_Buffer_t *SbBuffer;
    Pack_t           Pack;
    size_t           OutLen;
    void (*EncodeFunc)(Pack_t *, const SBN_TlmBase_t *);

    switch (MsgPtr->TlmId)
    {
        case SBN_HK_CC:
            OutLen     = SBN_HK_LEN;
            EncodeFunc = SBN_Encode_HkTlm;
            break;
        case SBN_HK_NET_CC:
            OutLen     = SBN_HKNET_LEN;
            EncodeFunc = SBN_Encode_HkNetTlm;
            break;
        case SBN_HK_PEER_CC:
            OutLen     = SBN_HKPEER_LEN;
            EncodeFunc = SBN_Encode_HkPeerTlm;
            break;
        case SBN_HK_MYSUBS_CC:
            OutLen     = SBN_HKMYSUBS_LEN;
            EncodeFunc = SBN_Encode_HkMySubsTlm;
            break;
        case SBN_HK_PEERSUBS_CC:
            OutLen     = SBN_HKPEERSUBS_LEN;
            EncodeFunc = SBN_Encode_HkPeerSubsTlm;
            break;
        default:
            OutLen     = 0;
            EncodeFunc = NULL;
            break;
    }

    if (EncodeFunc != NULL)
    {
        SbBuffer = SBN_Encode_DoPackInit(&Pack, OutLen, MsgPtr);
    }
    else
    {
        SbBuffer = NULL;
    }

    if (SbBuffer != NULL)
    {
        EncodeFunc(&Pack, MsgPtr);
    }

    return SbBuffer;
}
