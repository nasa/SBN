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

#include "sbn_coveragetest_common.h"
#include "sbn_encode.h"
#include "sbn_pack.h"

uint8 SBN_UT_Codec_PackBuf[SBN_MAX_PACKED_MSG_SZ];

union
{
    SBN_TlmBase_t       Base;
    SBN_HkTlm_t         HkTlm;
    SBN_HkNetTlm_t      HkNetTlm;
    SBN_HkPeerTlm_t     HkPeerTlm;
    SBN_HkMySubsTlm_t   HkMySubsTlm;
    SBN_HkPeerSubsTlm_t HkPeerSubsTlm;
} SBN_UT_Codec_TlmBuf;

void Test_SBN_Codec_GetTlmBuffer(void)
{
    /* Test Case For:
     * SBN_TlmBase_t *SBN_Codec_GetTlmBuffer(size_t MsgSize);
     */
    UtAssert_NOT_NULL(SBN_Codec_GetTlmBuffer(sizeof(SBN_TlmBase_t)));
    UtAssert_NULL(SBN_Codec_GetTlmBuffer(sizeof(SBN_UT_Codec_TlmBuf) + 10));
}

void Test_SBN_Codec_ReleaseTlmBuffer(void)
{
    /* Test Case For:
     * void SBN_Codec_ReleaseTlmBuffer(SBN_TlmBase_t *MsgPtr);
     */
    UtAssert_VOIDCALL(SBN_Codec_ReleaseTlmBuffer(NULL));
}

void Test_SBN_Codec_PackTlm(void)
{
    /* Test Case For:
     * CFE_SB_Buffer_t *SBN_Codec_PackTlm(const SBN_TlmBase_t *MsgPtr);
     */
    SBN_TlmBase_t *TlmBase;
    void          *BufPtr;

    BufPtr  = SBN_UT_Codec_PackBuf;
    TlmBase = &SBN_UT_Codec_TlmBuf.Base;
    memset(&SBN_UT_Codec_TlmBuf, 0, sizeof(SBN_UT_Codec_TlmBuf));
    UtAssert_NULL(SBN_Codec_PackTlm(TlmBase));

    /* No buffer */
    memset(&SBN_UT_Codec_TlmBuf, 0, sizeof(SBN_UT_Codec_TlmBuf));
    TlmBase->TlmId = SBN_HK_CC;
    UtAssert_NULL(SBN_Codec_PackTlm(TlmBase));

    /* valid cases for each TLM type */
    memset(&SBN_UT_Codec_TlmBuf, 0, sizeof(SBN_UT_Codec_TlmBuf));
    UT_SetDataBuffer(UT_KEY(CFE_SB_AllocateMessageBuffer), &BufPtr, sizeof(BufPtr), false);
    TlmBase->TlmId = SBN_HK_CC;
    UtAssert_NOT_NULL(SBN_Codec_PackTlm(TlmBase));

    memset(&SBN_UT_Codec_TlmBuf, 0, sizeof(SBN_UT_Codec_TlmBuf));
    UT_SetDataBuffer(UT_KEY(CFE_SB_AllocateMessageBuffer), &BufPtr, sizeof(BufPtr), false);
    TlmBase->TlmId = SBN_HK_NET_CC;
    UtAssert_NOT_NULL(SBN_Codec_PackTlm(TlmBase));

    memset(&SBN_UT_Codec_TlmBuf, 0, sizeof(SBN_UT_Codec_TlmBuf));
    UT_SetDataBuffer(UT_KEY(CFE_SB_AllocateMessageBuffer), &BufPtr, sizeof(BufPtr), false);
    TlmBase->TlmId = SBN_HK_PEER_CC;
    UtAssert_NOT_NULL(SBN_Codec_PackTlm(TlmBase));

    memset(&SBN_UT_Codec_TlmBuf, 0, sizeof(SBN_UT_Codec_TlmBuf));
    UT_SetDataBuffer(UT_KEY(CFE_SB_AllocateMessageBuffer), &BufPtr, sizeof(BufPtr), false);
    TlmBase->TlmId                                 = SBN_HK_MYSUBS_CC;
    SBN_UT_Codec_TlmBuf.HkMySubsTlm.Payload.SubCnt = 5;
    UtAssert_NOT_NULL(SBN_Codec_PackTlm(TlmBase));

    memset(&SBN_UT_Codec_TlmBuf, 0, sizeof(SBN_UT_Codec_TlmBuf));
    UT_SetDataBuffer(UT_KEY(CFE_SB_AllocateMessageBuffer), &BufPtr, sizeof(BufPtr), false);
    TlmBase->TlmId                                   = SBN_HK_PEERSUBS_CC;
    SBN_UT_Codec_TlmBuf.HkPeerSubsTlm.Payload.SubCnt = 5;
    UtAssert_NOT_NULL(SBN_Codec_PackTlm(TlmBase));
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Codec_GetTlmBuffer);
    ADD_TEST(SBN_Codec_ReleaseTlmBuffer);
    ADD_TEST(SBN_Codec_PackTlm);
}
