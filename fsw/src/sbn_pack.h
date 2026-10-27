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
 * uint8 Buffer[BUFSZ];
 * void packfn()
 * {
 *    SBN_PackState_t Pack;
 *    SBN_Pack_Init(&Pack, Buffer, sizeof(Buffer), true);
 *    SBN_Pack_UInt32(&Pack, 0x1234);
 * }
 *
 * void unpackfn()
 * {
 *    SBN_UnpackState_t Pack;
 *    uint32 f;
 *    SBN_Unpack_Init(&Pack, Buffer, sizeof(Buffer));
 *    SBN_Unpack_UInt32(&Pack, &f);
 * }
 * ```
 */

#include <stdlib.h> /* size_t */
#include "cfe.h"
#include "cfe_endian.h"

/*
 * About the pack structs:
 * There is just one basic pack state, which works on both packing and unpacking
 * However this makes it difficult to track the intent when reviewing code, specifically
 * which buffer is supposed to be input and which is supposed to be output.  One could also
 * call both Pack and Unpack on the same buffer which wouldn't make much sense and probably
 * would not yield the intended result.
 *
 * To make this clearer, we will intentionally break this into two state structs, depending
 * on the intent.  The "unpack" APIs should also have the input as const.
 */

/**
 * The base structure for both packing and unpacking.
 *
 * This should not be used directly.
 */
typedef struct
{
    uint8 *Buf;
    size_t BufSz;
    size_t BufUsed;
} SBN_PackBase_t;

/**
 * The state structure for packing
 *
 * This should be instantiated whenever it is needed to pack bits
 */
typedef struct
{
    SBN_PackBase_t Base;
} SBN_PackState_t;

/**
 * The state structure for unpacking
 *
 * This should be instantiated whenever it is needed to unpack bits
 */
typedef struct
{
    SBN_PackBase_t Base;
} SBN_UnpackState_t;

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
 * @sa #SBN_Pack_Data, #SBN_Pack_UInt8, #SBN_Pack_Int16, #SBN_Pack_UInt16, #SBN_Pack_UInt32, #SBN_Pack_Time,
 * #SBN_Pack_MsgID
 */
bool SBN_Pack_Init(SBN_PackState_t *PackPtr, void *Buf, size_t BufSz, bool ClearFlag);

/**
 * Initialize the unpacking management structure.
 *
 * @param[in,out] PackPtr The pointer to the management structure to initialize.
 * @param[in] Buf The buffer for reading packed data.
 * @param[in] BufSz The size of the buffer.
 *
 * @return true if the initialization succeeded.
 *
 * @sa #SBN_Unpack_Data, #SBN_Unpack_UInt8, #SBN_Unpack_Int16, #SBN_Unpack_UInt16, #SBN_Unpack_UInt32, #SBN_Unpack_Time,
 * #SBN_Unpack_MsgID
 */
bool SBN_Unpack_Init(SBN_UnpackState_t *PackPtr, const void *Buf, size_t BufSz);

/**
 * Get the current position of the packed data
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @returns current packed position within the packing buffer
 */
uint32 SBN_Pack_GetPosition(SBN_PackState_t *PackPtr);

/**
 * Get the current position within the packed data
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @returns current packed position within the unpacking buffer
 */
uint32 SBN_Unpack_GetPosition(SBN_UnpackState_t *PackPtr);

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
 * @sa #SBN_Pack_Init, #SBN_Unpack_Data
 */
bool SBN_Pack_Data(SBN_PackState_t *PackPtr, const void *DataBuf, size_t DataBufSz);

/**
 * Pack an unsigned 8-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #SBN_Pack_Data
 */
bool SBN_Pack_UInt8(SBN_PackState_t *PackPtr, uint8 Data);

/**
 * Pack an unsigned 16-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #SBN_Pack_Data
 */
bool SBN_Pack_UInt16(SBN_PackState_t *PackPtr, uint16 Data);

/**
 * Pack a signed 16-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #SBN_Pack_Data
 */
bool SBN_Pack_Int16(SBN_PackState_t *PackPtr, int16 Data);

/**
 * Pack an unsigned 32-bit integer into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #SBN_Pack_Data
 */
bool SBN_Pack_UInt32(SBN_PackState_t *PackPtr, uint32 Data);

/**
 * Pack a time datum into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #SBN_Pack_Data
 */
bool SBN_Pack_Time(SBN_PackState_t *PackPtr, OS_time_t Data);

/**
 * Pack a CFE Software Bus message identifier into the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[in] Data The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #SBN_Pack_Data
 */
bool SBN_Pack_MsgID(SBN_PackState_t *PackPtr, CFE_SB_MsgId_t Data);

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
 * @sa #SBN_Pack_Init, #SBN_Pack_Data
 */
bool SBN_Unpack_Data(SBN_UnpackState_t *PackPtr, void *DataBuf, size_t Sz);

/**
 * Unpack an unsigned 8-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #SBN_Unpack_Data
 */
bool SBN_Unpack_UInt8(SBN_UnpackState_t *PackPtr, uint8 *DataBuf);

/**
 * Unpack an unsigned 16-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #SBN_Unpack_Data
 */
bool SBN_Unpack_UInt16(SBN_UnpackState_t *PackPtr, uint16 *DataBuf);

/**
 * Unpack a signed 16-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #SBN_Unpack_Data
 */
bool SBN_Unpack_Int16(SBN_UnpackState_t *PackPtr, int16 *DataBuf);

/**
 * Unpack an unsigned 32-bit integer from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #SBN_Unpack_Data
 */
bool SBN_Unpack_UInt32(SBN_UnpackState_t *PackPtr, uint32 *DataBuf);

/**
 * Unpack a time datum from the buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #SBN_Unpack_Data
 */
bool SBN_Unpack_Time(SBN_UnpackState_t *PackPtr, OS_time_t *DataBuf);

/**
 * Unpack a CFE software bus message identifier from the pack buffer.
 *
 * @param[in,out] PackPtr The pointer to the management structure.
 * @param[out] DataBuf The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #SBN_Unpack_Data
 */
bool SBN_Unpack_MsgID(SBN_UnpackState_t *PackPtr, CFE_SB_MsgId_t *DataBuf);

#endif /* _sbn_pack_h_ */
