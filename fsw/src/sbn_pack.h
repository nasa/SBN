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
#include "osconfig.h"
#include "cfe.h"

typedef struct
{   
    void *Buf;
    size_t BufSz, BufUsed;
} Pack_t;

/**
 * Initialize the packing management structure.
 *
 * @param PackPtr[in/out] The pointer to the management structure to initialize.
 * @param Buf[in] The buffer for storing packed data.
 * @param BufSz[in] The size of the buffer.
 * @param ClearFlag[in] If true, zero the buffer.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param DataBuf[in] The buffer to pack into the buffer.
 * @param DataBufSz[in] The size of the data buffer.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Init, #Unpack_Data
 */
bool Pack_Data(Pack_t *PackPtr, void *DataBuf, size_t DataBufSz);

/**
 * Pack an unsigned 8-bit integer into the buffer.
 *
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param Data[in] The value to pack.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param Data[in] The value to pack.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param Data[in] The value to pack.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param Data[in] The value to pack.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param Data[in] The value to pack.
 *
 * @return true if the data was successfully packed into the buffer, false if
 *         it failed (likely due to running out of available space in the buffer.)
 *
 * @sa #Pack_Data
 */
bool Pack_Time(Pack_t *PackPtr, OS_time_t Data);

/**
 * Pack a CFE Software Bus message identifier into the buffer.
 *
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param Data[in] The value to pack.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param DataBuf[out] The buffer to pack from the buffer.
 * @param DataBufSz[in] The number of bytes to read from the buffer into DataBuf.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param DataBuf[out] The pointer to where the data should be stored.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param DataBuf[out] The pointer to where the data should be stored.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param DataBuf[out] The pointer to where the data should be stored.
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
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param DataBuf[out] The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_UInt32(Pack_t *PackPtr, uint32 *DataBuf);

/**
 * Unpack a CFE software bus message identifier from the pack buffer.
 *
 * @param PackPtr[in/out] The pointer to the management structure.
 * @param DataBuf[out] The pointer to where the data should be stored.
 *
 * @return true if the data was successfully read from the buffer, false if
 *         it failed (likely due to no more data in the buffer.)
 *
 * @sa #Unpack_Data
 */
bool Unpack_MsgID(Pack_t *PackPtr, CFE_SB_MsgId_t *DataBuf);

#endif /* _sbn_pack_h_ */
