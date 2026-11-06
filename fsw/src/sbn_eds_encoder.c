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
#include "sbn_encode.h"

/*
 * For the EDS implementation of the packing/unpacking it is very
 * simple.  In the EDS model everything on the software bus is in
 * the native format, so no packing is done here at all.  If the
 * message gets sent outside the current processor, it will be encoded
 * at that point.
 */

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in declaration for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_TlmBase_t *SBN_Codec_GetTlmBuffer(size_t MsgSize)
{
    /* In EDS we can just directly use an SB buffer */
    return (SBN_TlmBase_t *)CFE_SB_AllocateMessageBuffer(MsgSize);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in declaration for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Codec_ReleaseTlmBuffer(SBN_TlmBase_t *TlmBuf)
{
    /* As this was obtained from the SB pool, it needs to be returned to the pool */
    CFE_SB_ReleaseMessageBuffer((CFE_SB_Buffer_t *)TlmBuf);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in declaration for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_SB_Buffer_t *SBN_Codec_PackTlm(const SBN_TlmBase_t *MsgPtr)
{
    /* Nothing to do here - Just pass it through */
    return (CFE_SB_Buffer_t *)MsgPtr;
}
