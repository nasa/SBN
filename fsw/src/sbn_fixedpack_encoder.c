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
#include "sbn_error.h"
#include "sbn_pack.h"
#include "sbn_encode.h"

/*----------------------------------------------------------------
 *
 * Local helper function
 * Common handler for packing of the TLM headers
 * This also includes the SBN-specific ID that follows the CFS TLM header
 *
 *-----------------------------------------------------------------*/
static CFE_SB_Buffer_t *SBN_Codec_DoPackTlmInit(SBN_PackState_t *Pack, size_t BufSz, const SBN_TlmBase_t *Input)
{
    CFE_SB_Buffer_t *SbBufPtr;

    SbBufPtr = CFE_SB_AllocateMessageBuffer(BufSz);
    if (SbBufPtr != NULL)
    {
        SBN_Pack_Init(Pack, SbBufPtr, BufSz, true);

        /* Pass through the header as-is */
        SBN_Pack_Data(Pack, &Input->TelemetryHeader, sizeof(Input->TelemetryHeader));

        /* Pack the ID byte (always present) */
        SBN_Pack_UInt8(Pack, Input->TlmId);
    }

    return SbBufPtr;
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Handler for packing of the HK TLM structure
 *
 *-----------------------------------------------------------------*/
static void SBN_Codec_HkTlm(SBN_PackState_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkTlm_t *Input = (const SBN_HkTlm_t *)MsgPtr;

    SBN_Pack_UInt16(Pack, Input->Payload.CmdCnt);
    SBN_Pack_UInt16(Pack, Input->Payload.CmdErrCnt);
    SBN_Pack_UInt16(Pack, Input->Payload.SubCnt);
    SBN_Pack_UInt16(Pack, Input->Payload.NetCnt);
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Handler for packing of the HK Net TLM structure
 *
 *-----------------------------------------------------------------*/
static void SBN_Codec_HkNetTlm(SBN_PackState_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkNetTlm_t *Input = (const SBN_HkNetTlm_t *)MsgPtr;

    SBN_Pack_UInt8(Pack, Input->Payload.ProtocolIdx);
    SBN_Pack_UInt16(Pack, Input->Payload.PeerCnt);
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Handler for packing of the HK Peer TLM structure
 *
 *-----------------------------------------------------------------*/
static void SBN_Codec_HkPeerTlm(SBN_PackState_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkPeerTlm_t *Input = (const SBN_HkPeerTlm_t *)MsgPtr;

    SBN_Pack_UInt32(Pack, Input->Payload.ProcessorId);
    SBN_Pack_Time(Pack, Input->Payload.LastSend);
    SBN_Pack_Time(Pack, Input->Payload.LastRecv);
    SBN_Pack_UInt16(Pack, Input->Payload.SendCnt);
    SBN_Pack_UInt16(Pack, Input->Payload.RecvCnt);
    SBN_Pack_UInt16(Pack, Input->Payload.SendErrCnt);
    SBN_Pack_UInt16(Pack, Input->Payload.RecvErrCnt);
    SBN_Pack_UInt16(Pack, Input->Payload.SubCnt);
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Handler for packing of the HK MySubs TLM structure
 *
 *-----------------------------------------------------------------*/
static void SBN_Codec_HkMySubsTlm(SBN_PackState_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkMySubsTlm_t *Input = (const SBN_HkMySubsTlm_t *)MsgPtr;
    SBN_SubCnt_t             i;

    SBN_Pack_UInt16(Pack, Input->Payload.SubCnt);
    for (i = 0; i < SBN_AppData.SubCnt; i++)
    {
        SBN_Pack_MsgID(Pack, Input->Payload.Subs[i]);
    }
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Handler for packing of the HK PeerSubs TLM structure
 *
 *-----------------------------------------------------------------*/
static void SBN_Codec_HkPeerSubsTlm(SBN_PackState_t *Pack, const SBN_TlmBase_t *MsgPtr)
{
    const SBN_HkPeerSubsTlm_t *Input = (const SBN_HkPeerSubsTlm_t *)MsgPtr;
    SBN_SubCnt_t               i;

    SBN_Pack_UInt16(Pack, Input->Payload.NetIdx);
    SBN_Pack_UInt16(Pack, Input->Payload.PeerIdx);
    SBN_Pack_UInt16(Pack, Input->Payload.SubCnt);

    for (i = 0; i < Input->Payload.SubCnt; i++)
    {
        SBN_Pack_MsgID(Pack, Input->Payload.Subs[i]);
    }
}

/*
 * **************************************
 * EXTERNALLY-CALLED FUNCTIONS START HERE
 * **************************************
 */

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in declaration for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_TlmBase_t *SBN_Codec_GetTlmBuffer(size_t MsgSize)
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

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in declaration for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Codec_ReleaseTlmBuffer(SBN_TlmBase_t *TlmBuf)
{
    /* no-op, the buffers are statically allocated */
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in declaration for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_SB_Buffer_t *SBN_Codec_PackTlm(const SBN_TlmBase_t *MsgPtr)
{
    CFE_SB_Buffer_t *SbBuffer;
    SBN_PackState_t  Pack;
    size_t           OutLen;
    void (*EncodeFunc)(SBN_PackState_t *, const SBN_TlmBase_t *);

    switch (MsgPtr->TlmId)
    {
        case SBN_HK_CC:
            OutLen     = SBN_HK_LEN;
            EncodeFunc = SBN_Codec_HkTlm;
            break;
        case SBN_HK_NET_CC:
            OutLen     = SBN_HKNET_LEN;
            EncodeFunc = SBN_Codec_HkNetTlm;
            break;
        case SBN_HK_PEER_CC:
            OutLen     = SBN_HKPEER_LEN;
            EncodeFunc = SBN_Codec_HkPeerTlm;
            break;
        case SBN_HK_MYSUBS_CC:
            OutLen     = SBN_HKMYSUBS_LEN;
            EncodeFunc = SBN_Codec_HkMySubsTlm;
            break;
        case SBN_HK_PEERSUBS_CC:
            OutLen     = SBN_HKPEERSUBS_LEN;
            EncodeFunc = SBN_Codec_HkPeerSubsTlm;
            break;
        default:
            OutLen     = 0;
            EncodeFunc = NULL;
            break;
    }

    if (EncodeFunc != NULL)
    {
        SbBuffer = SBN_Codec_DoPackTlmInit(&Pack, OutLen, MsgPtr);
    }
    else
    {
        SbBuffer = NULL;
    }

    if (SbBuffer != NULL)
    {
        EncodeFunc(&Pack, MsgPtr);

        /* note: this does not need to release anything
           because the temp is a static buffer */
    }

    return SbBuffer;
}
