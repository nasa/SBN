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
#ifndef SBN_ENCODE_H
#define SBN_ENCODE_H

#include "sbn_msg.h"
#include "cfe_sb_api_typedefs.h"
#include "cfe_msg_hdr.h"

/*************************************************************** */
/**
 * @brief Gets a buffer for building a telemetry message
 *
 * This returns a block of memory suitable for temporarily storing
 * TLM message data as the message is constructed.  It may be a temporary
 * buffer or it may directly be a SB buffer, depending on the underlying
 * encoder.
 *
 * @note The caller must only use one buffer at a time, ensuring
 * that the buffer is released (either directly or by sending it on).
 * This function may return the same buffer for every call.
 *
 * @param   MsgSize  The size of the message to be stored
 * @returns Pointer to temporary buffer
 * @retval  NULL if no buffer is available
 */
SBN_TlmBase_t *SBN_Codec_GetTlmBuffer(size_t MsgSize);

/*************************************************************** */
/**
 * @brief Releases a TLM buffer without packing or sending it
 *
 * This releases a block previously obtained from SBN_Codec_GetTlmBuffer()
 * without sending the message.  It effectively drops the message.
 *
 * Callers must ensure that every call to SBN_Codec_GetTlmBuffer() is
 * followed by either SBN_Codec_ReleaseTlmBuffer() or SBN_Codec_PackTlm()
 *
 * @param   MsgPtr  Pointer from a previous call to SBN_Codec_GetTlmBuffer()
 */
void SBN_Codec_ReleaseTlmBuffer(SBN_TlmBase_t *MsgPtr);

/*************************************************************** */
/**
 * @brief Packs a TLM  buffer in order to send it to the software bus
 *
 * This prepares the temporary buffer previously obtained via
 * SBN_Codec_GetTlmBuffer() for sending to the software bus.  Data
 * in the temporary buffer is packed in a suitable form for forwarding.
 *
 * @note This also consumes/releases the temporary buffer.  The caller must
 * NOT access the temporary buffer in any way after a successful invocation
 * of this function.
 *
 * If this returns NULL due to an error, then the input buffer will not be
 * consumed/released.  The caller should still free the buffer after reporting
 * the error and/or taking any corrective action.
 *
 * @param   MsgPtr  Pointer from a previous call to SBN_Codec_GetTlmBuffer()
 * @returns Pointer to software bus buffer, populated with the packed data
 * @retval  NULL    if the software bus memory pool is empty
 */
CFE_SB_Buffer_t *SBN_Codec_PackTlm(const SBN_TlmBase_t *MsgPtr);

#endif
