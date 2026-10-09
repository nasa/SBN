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

#ifndef _sbn_pack_h_
#define _sbn_pack_h_

/**
 * SBN messages are transmitted over the wire with SBN-specific headers which are
 * network-order packed binary values. These functions are utilities for packing
 * and unpacking those headers.
 *
 * General pattern is:
 *
 * ```
 * Pack_t Pack;
 * uint8 Buffer[BUFSZ];
 * void packfn()
 * {
 *    Pack_Init(&Pack, Buffer, sizeof(Buffer), true);
 *    Pack_UInt32(&Pack, 0x1234);
 * }
 *
 * void unpackfn()
 * {
 *    uint32 f;
 *    Pack_Init(&Pack, Buffer, sizeof(Buffer), false);
 *    Unpack_UInt32(&Pack, &f);
 * }
 * ```
 */

#include <stdlib.h> /* size_t */
#include "cfe.h"
#include "cfe_endian.h"
#include "sbn_extern_typedefs.h"

typedef struct
{
    void  *Buf;
    size_t BufSz, BufUsed;
} Pack_t;

/**
 * Initialize the packing management structure.
 *
 * @param[in,out] PackPtr The pointer to the management structure to initialize.
 * @param[in] Buf The buffer for storing packed data.
 * @param[in] BufSz The size of the buffer.
 * @param[in] ClearFlag If true, zero the buffer.
 *
 * @return true if the initialization succeeded.
 *
 * @sa #Pack_Data, #Pack_UInt8, #Pack_Int16, #Pack_UInt16, #Pack_UInt32, #Pack_Time, #Pack_MsgID
 * @sa #Unpack_Data, #Unpack_UInt8, #Unpack_Int16, #Unpack_UInt16, #Unpack_UInt32, #Unpack_Time, #Unpack_MsgID
 */
bool Pack_Init(Pack_t *PackPtr, void *Buf, size_t BufSz, bool ClearFlag);

/**
 * Pack data into the buffer. This does the grunt-work of managing the buffer,
 * and has no idea what the data contains.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] DataBuf The buffer to pack into the buffer.
 * @param[in] DataBufSz The size of the data buffer.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Init, #Unpack_Data
 */
bool Pack_Data(Pack_t *PackPtr, const void *DataBuf, size_t DataBufSz);

/**
 * Pack an unsigned 8-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Data
 */
bool Pack_UInt8(Pack_t *PackPtr, uint8 Data);

/**
 * Pack an unsigned 16-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Data
 */
bool Pack_UInt16(Pack_t *PackPtr, uint16 Data);

/**
 * Pack a signed 16-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Data
 */
bool Pack_Int16(Pack_t *PackPtr, int16 Data);

/**
 * Pack an unsigned 32-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Data
 */
bool Pack_UInt32(Pack_t *PackPtr, uint32 Data);

/**
 * Pack a time datum into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Data
 */
bool Pack_Time(Pack_t *PackPtr, SBN_Timestamp_t Data);

/**
 * Pack a CFE Software Bus message identifier into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Data
 */
bool Pack_MsgID(Pack_t *PackPtr, CFE_SB_MsgId_t Data);

/**
 * Unpacks data from the existing packed buffer. This does the grunt-work of managing the buffer,
 * and has no idea what the data contains.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The buffer to pack from the buffer.
 * @param[in] Sz The number of bytes to read from the buffer into DataBuf.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Pack_Init, #Pack_Data
 */
bool Unpack_Data(Pack_t *PackPtr, void *DataBuf, size_t Sz);

/**
 * Unpack an unsigned 8-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_UInt8(Pack_t *PackPtr, uint8 *DataBuf);

/**
 * Unpack an unsigned 16-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_UInt16(Pack_t *PackPtr, uint16 *DataBuf);

/**
 * Unpack a signed 16-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_Int16(Pack_t *PackPtr, int16 *DataBuf);

/**
 * Unpack an unsigned 32-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_UInt32(Pack_t *PackPtr, uint32 *DataBuf);

/**
 * Unpack a time datum from the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_Time(Pack_t *PackPtr, SBN_Timestamp_t *DataBuf);

/**
 * Unpack a CFE software bus message identifier from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_MsgID(Pack_t *PackPtr, CFE_SB_MsgId_t *DataBuf);

#endif /* _sbn_pack_h_ */
