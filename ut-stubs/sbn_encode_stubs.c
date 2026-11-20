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

/**
 * @file
 *
 * Auto-Generated stub implementations for functions defined in sbn_encode header
 */

#include "sbn_encode.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Codec_GetTlmBuffer()
 * ----------------------------------------------------
 */
SBN_TlmBase_t *SBN_Codec_GetTlmBuffer(size_t MsgSize)
{
    UT_GenStub_SetupReturnBuffer(SBN_Codec_GetTlmBuffer, SBN_TlmBase_t *);

    UT_GenStub_AddParam(SBN_Codec_GetTlmBuffer, size_t, MsgSize);

    UT_GenStub_Execute(SBN_Codec_GetTlmBuffer, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Codec_GetTlmBuffer, SBN_TlmBase_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Codec_PackTlm()
 * ----------------------------------------------------
 */
CFE_SB_Buffer_t *SBN_Codec_PackTlm(const SBN_TlmBase_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_Codec_PackTlm, CFE_SB_Buffer_t *);

    UT_GenStub_AddParam(SBN_Codec_PackTlm, const SBN_TlmBase_t *, MsgPtr);

    UT_GenStub_Execute(SBN_Codec_PackTlm, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Codec_PackTlm, CFE_SB_Buffer_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Codec_ReleaseTlmBuffer()
 * ----------------------------------------------------
 */
void SBN_Codec_ReleaseTlmBuffer(SBN_TlmBase_t *MsgPtr)
{
    UT_GenStub_AddParam(SBN_Codec_ReleaseTlmBuffer, SBN_TlmBase_t *, MsgPtr);

    UT_GenStub_Execute(SBN_Codec_ReleaseTlmBuffer, Basic, NULL);
}
