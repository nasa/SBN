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
 * Auto-Generated stub implementations for functions defined in sbn_pack header
 */

#include "sbn_pack.h"
#include "utgenstub.h"

void UT_DefaultHandler_SBN_Pack_Data(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_GetPosition(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_Init(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_Int16(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_MsgID(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_Time(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_UInt16(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_UInt32(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Pack_UInt8(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_Data(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_GetPosition(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_Init(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_Int16(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_MsgID(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_Time(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_UInt16(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_UInt32(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_Unpack_UInt8(void *, UT_EntryKey_t, const UT_StubContext_t *);

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_Data()
 * ----------------------------------------------------
 */
bool SBN_Pack_Data(SBN_PackState_t *PackPtr, const void *DataBuf, size_t DataBufSz)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_Data, bool);

    UT_GenStub_AddParam(SBN_Pack_Data, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_Data, const void *, DataBuf);
    UT_GenStub_AddParam(SBN_Pack_Data, size_t, DataBufSz);

    UT_GenStub_Execute(SBN_Pack_Data, Basic, UT_DefaultHandler_SBN_Pack_Data);

    return UT_GenStub_GetReturnValue(SBN_Pack_Data, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_GetPosition()
 * ----------------------------------------------------
 */
uint32 SBN_Pack_GetPosition(SBN_PackState_t *PackPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_GetPosition, uint32);

    UT_GenStub_AddParam(SBN_Pack_GetPosition, SBN_PackState_t *, PackPtr);

    UT_GenStub_Execute(SBN_Pack_GetPosition, Basic, UT_DefaultHandler_SBN_Pack_GetPosition);

    return UT_GenStub_GetReturnValue(SBN_Pack_GetPosition, uint32);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_Init()
 * ----------------------------------------------------
 */
bool SBN_Pack_Init(SBN_PackState_t *PackPtr, void *Buf, size_t BufSz, bool ClearFlag)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_Init, bool);

    UT_GenStub_AddParam(SBN_Pack_Init, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_Init, void *, Buf);
    UT_GenStub_AddParam(SBN_Pack_Init, size_t, BufSz);
    UT_GenStub_AddParam(SBN_Pack_Init, bool, ClearFlag);

    UT_GenStub_Execute(SBN_Pack_Init, Basic, UT_DefaultHandler_SBN_Pack_Init);

    return UT_GenStub_GetReturnValue(SBN_Pack_Init, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_Int16()
 * ----------------------------------------------------
 */
bool SBN_Pack_Int16(SBN_PackState_t *PackPtr, int16 Data)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_Int16, bool);

    UT_GenStub_AddParam(SBN_Pack_Int16, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_Int16, int16, Data);

    UT_GenStub_Execute(SBN_Pack_Int16, Basic, UT_DefaultHandler_SBN_Pack_Int16);

    return UT_GenStub_GetReturnValue(SBN_Pack_Int16, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_MsgID()
 * ----------------------------------------------------
 */
bool SBN_Pack_MsgID(SBN_PackState_t *PackPtr, CFE_SB_MsgId_t Data)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_MsgID, bool);

    UT_GenStub_AddParam(SBN_Pack_MsgID, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_MsgID, CFE_SB_MsgId_t, Data);

    UT_GenStub_Execute(SBN_Pack_MsgID, Basic, UT_DefaultHandler_SBN_Pack_MsgID);

    return UT_GenStub_GetReturnValue(SBN_Pack_MsgID, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_Time()
 * ----------------------------------------------------
 */
bool SBN_Pack_Time(SBN_PackState_t *PackPtr, SBN_Timestamp_t Data)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_Time, bool);

    UT_GenStub_AddParam(SBN_Pack_Time, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_Time, SBN_Timestamp_t, Data);

    UT_GenStub_Execute(SBN_Pack_Time, Basic, UT_DefaultHandler_SBN_Pack_Time);

    return UT_GenStub_GetReturnValue(SBN_Pack_Time, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_UInt16()
 * ----------------------------------------------------
 */
bool SBN_Pack_UInt16(SBN_PackState_t *PackPtr, uint16 Data)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_UInt16, bool);

    UT_GenStub_AddParam(SBN_Pack_UInt16, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_UInt16, uint16, Data);

    UT_GenStub_Execute(SBN_Pack_UInt16, Basic, UT_DefaultHandler_SBN_Pack_UInt16);

    return UT_GenStub_GetReturnValue(SBN_Pack_UInt16, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_UInt32()
 * ----------------------------------------------------
 */
bool SBN_Pack_UInt32(SBN_PackState_t *PackPtr, uint32 Data)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_UInt32, bool);

    UT_GenStub_AddParam(SBN_Pack_UInt32, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_UInt32, uint32, Data);

    UT_GenStub_Execute(SBN_Pack_UInt32, Basic, UT_DefaultHandler_SBN_Pack_UInt32);

    return UT_GenStub_GetReturnValue(SBN_Pack_UInt32, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Pack_UInt8()
 * ----------------------------------------------------
 */
bool SBN_Pack_UInt8(SBN_PackState_t *PackPtr, uint8 Data)
{
    UT_GenStub_SetupReturnBuffer(SBN_Pack_UInt8, bool);

    UT_GenStub_AddParam(SBN_Pack_UInt8, SBN_PackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Pack_UInt8, uint8, Data);

    UT_GenStub_Execute(SBN_Pack_UInt8, Basic, UT_DefaultHandler_SBN_Pack_UInt8);

    return UT_GenStub_GetReturnValue(SBN_Pack_UInt8, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_Data()
 * ----------------------------------------------------
 */
bool SBN_Unpack_Data(SBN_UnpackState_t *PackPtr, void *DataBuf, size_t Sz)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_Data, bool);

    UT_GenStub_AddParam(SBN_Unpack_Data, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_Data, void *, DataBuf);
    UT_GenStub_AddParam(SBN_Unpack_Data, size_t, Sz);

    UT_GenStub_Execute(SBN_Unpack_Data, Basic, UT_DefaultHandler_SBN_Unpack_Data);

    return UT_GenStub_GetReturnValue(SBN_Unpack_Data, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_GetPosition()
 * ----------------------------------------------------
 */
uint32 SBN_Unpack_GetPosition(SBN_UnpackState_t *PackPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_GetPosition, uint32);

    UT_GenStub_AddParam(SBN_Unpack_GetPosition, SBN_UnpackState_t *, PackPtr);

    UT_GenStub_Execute(SBN_Unpack_GetPosition, Basic, UT_DefaultHandler_SBN_Unpack_GetPosition);

    return UT_GenStub_GetReturnValue(SBN_Unpack_GetPosition, uint32);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_Init()
 * ----------------------------------------------------
 */
bool SBN_Unpack_Init(SBN_UnpackState_t *PackPtr, const void *Buf, size_t BufSz)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_Init, bool);

    UT_GenStub_AddParam(SBN_Unpack_Init, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_Init, const void *, Buf);
    UT_GenStub_AddParam(SBN_Unpack_Init, size_t, BufSz);

    UT_GenStub_Execute(SBN_Unpack_Init, Basic, UT_DefaultHandler_SBN_Unpack_Init);

    return UT_GenStub_GetReturnValue(SBN_Unpack_Init, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_Int16()
 * ----------------------------------------------------
 */
bool SBN_Unpack_Int16(SBN_UnpackState_t *PackPtr, int16 *DataBuf)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_Int16, bool);

    UT_GenStub_AddParam(SBN_Unpack_Int16, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_Int16, int16 *, DataBuf);

    UT_GenStub_Execute(SBN_Unpack_Int16, Basic, UT_DefaultHandler_SBN_Unpack_Int16);

    return UT_GenStub_GetReturnValue(SBN_Unpack_Int16, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_MsgID()
 * ----------------------------------------------------
 */
bool SBN_Unpack_MsgID(SBN_UnpackState_t *PackPtr, CFE_SB_MsgId_t *DataBuf)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_MsgID, bool);

    UT_GenStub_AddParam(SBN_Unpack_MsgID, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_MsgID, CFE_SB_MsgId_t *, DataBuf);

    UT_GenStub_Execute(SBN_Unpack_MsgID, Basic, UT_DefaultHandler_SBN_Unpack_MsgID);

    return UT_GenStub_GetReturnValue(SBN_Unpack_MsgID, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_Time()
 * ----------------------------------------------------
 */
bool SBN_Unpack_Time(SBN_UnpackState_t *PackPtr, SBN_Timestamp_t *DataBuf)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_Time, bool);

    UT_GenStub_AddParam(SBN_Unpack_Time, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_Time, SBN_Timestamp_t *, DataBuf);

    UT_GenStub_Execute(SBN_Unpack_Time, Basic, UT_DefaultHandler_SBN_Unpack_Time);

    return UT_GenStub_GetReturnValue(SBN_Unpack_Time, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_UInt16()
 * ----------------------------------------------------
 */
bool SBN_Unpack_UInt16(SBN_UnpackState_t *PackPtr, uint16 *DataBuf)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_UInt16, bool);

    UT_GenStub_AddParam(SBN_Unpack_UInt16, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_UInt16, uint16 *, DataBuf);

    UT_GenStub_Execute(SBN_Unpack_UInt16, Basic, UT_DefaultHandler_SBN_Unpack_UInt16);

    return UT_GenStub_GetReturnValue(SBN_Unpack_UInt16, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_UInt32()
 * ----------------------------------------------------
 */
bool SBN_Unpack_UInt32(SBN_UnpackState_t *PackPtr, uint32 *DataBuf)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_UInt32, bool);

    UT_GenStub_AddParam(SBN_Unpack_UInt32, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_UInt32, uint32 *, DataBuf);

    UT_GenStub_Execute(SBN_Unpack_UInt32, Basic, UT_DefaultHandler_SBN_Unpack_UInt32);

    return UT_GenStub_GetReturnValue(SBN_Unpack_UInt32, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Unpack_UInt8()
 * ----------------------------------------------------
 */
bool SBN_Unpack_UInt8(SBN_UnpackState_t *PackPtr, uint8 *DataBuf)
{
    UT_GenStub_SetupReturnBuffer(SBN_Unpack_UInt8, bool);

    UT_GenStub_AddParam(SBN_Unpack_UInt8, SBN_UnpackState_t *, PackPtr);
    UT_GenStub_AddParam(SBN_Unpack_UInt8, uint8 *, DataBuf);

    UT_GenStub_Execute(SBN_Unpack_UInt8, Basic, UT_DefaultHandler_SBN_Unpack_UInt8);

    return UT_GenStub_GetReturnValue(SBN_Unpack_UInt8, bool);
}
