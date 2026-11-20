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

/*
 * Handlers for SBN pack routines, mainly needed for outputs for unpacking
 */

#include <string.h> /* for memcpy */

#include "sbn_stubs.h"
#include "utstubs.h"
#include "sbn_pack.h"

static void SBN_UT_Pack_Handler(const UT_StubContext_t *Context, UT_EntryKey_t FuncKey, const void *DataSrc, size_t Sz)
{
    SBN_PackState_t *PackPtr = UT_Hook_GetArgValueByName(Context, "PackPtr", SBN_PackState_t *);
    int32            status;
    bool             got_data;

    got_data = false;
    if (!UT_Stub_GetInt32StatusCode(Context, &status))
    {
        status = 0;
    }

    if (status == 0 && DataSrc != NULL)
    {
        if (UT_Stub_CopyFromLocal(FuncKey, DataSrc, Sz) == Sz)
        {
            got_data = true;
        }
        else if (PackPtr != NULL && PackPtr->Base.Buf != NULL && (PackPtr->Base.BufUsed + Sz) <= PackPtr->Base.BufSz)
        {
            /* this is basically the real thing */
            memcpy(&PackPtr->Base.Buf[PackPtr->Base.BufUsed], DataSrc, Sz);
            PackPtr->Base.BufUsed += Sz;
            got_data               = true;
        }
    }

    UT_Stub_SetReturnValue(FuncKey, got_data);
}

static void SBN_UT_Unpack_Handler(const UT_StubContext_t *Context, UT_EntryKey_t FuncKey, size_t Sz)
{
    SBN_UnpackState_t *PackPtr = UT_Hook_GetArgValueByName(Context, "PackPtr", SBN_UnpackState_t *);
    void              *DataBuf = UT_Hook_GetArgValueByName(Context, "DataBuf", void *);
    int32              status;
    bool               got_data;

    got_data = false;
    if (!UT_Stub_GetInt32StatusCode(Context, &status))
    {
        status = 0;
    }

    if (status == 0 && DataBuf != NULL)
    {
        if (UT_Stub_CopyToLocal(FuncKey, DataBuf, Sz) == Sz)
        {
            got_data = true;
        }
        else if (PackPtr != NULL && PackPtr->Base.Buf != NULL && (PackPtr->Base.BufUsed + Sz) <= PackPtr->Base.BufSz)
        {
            /* this is basically the real thing */
            memcpy(DataBuf, &PackPtr->Base.Buf[PackPtr->Base.BufUsed], Sz);
            PackPtr->Base.BufUsed += Sz;
            got_data               = true;
        }
    }

    UT_Stub_SetReturnValue(FuncKey, got_data);
}

static void SBN_UT_PackInit_Handler(const UT_StubContext_t *Context, UT_EntryKey_t FuncKey)
{
    SBN_PackBase_t *PackBase = UT_Hook_GetArgValueByName(Context, "PackPtr", SBN_PackBase_t *);
    void           *Buf      = UT_Hook_GetArgValueByName(Context, "Buf", void *);
    size_t          BufSz    = UT_Hook_GetArgValueByName(Context, "BufSz", size_t);
    int32           status;
    bool            result;

    result = false;
    if (!UT_Stub_GetInt32StatusCode(Context, &status))
    {
        status = 0;
    }

    /* This needs to do the "real-ish" thing because other things might depend on this pointer/sz */
    if (status == 0 && PackBase != NULL)
    {
        memset(PackBase, 0, sizeof(*PackBase));
        PackBase->Buf   = Buf;
        PackBase->BufSz = BufSz;

        result = true;
    }

    UT_Stub_SetReturnValue(FuncKey, result);
}

static void SBN_UT_GetPosition_Handler(const UT_StubContext_t *Context, UT_EntryKey_t FuncKey)
{
    SBN_PackBase_t *PackBase = UT_Hook_GetArgValueByName(Context, "PackPtr", SBN_PackBase_t *);
    uint32          RetVal;

    if (!UT_Stub_GetInt32StatusCode(Context, NULL) && PackBase != NULL)
    {
        RetVal = PackBase->BufUsed;
        UT_Stub_SetReturnValue(FuncKey, RetVal);
    }
}

void UT_DefaultHandler_SBN_Pack_Init(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_PackInit_Handler(Context, FuncKey);
}

void UT_DefaultHandler_SBN_Unpack_Init(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_PackInit_Handler(Context, FuncKey);
}

void UT_DefaultHandler_SBN_Pack_GetPosition(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_GetPosition_Handler(Context, FuncKey);
}

void UT_DefaultHandler_SBN_Unpack_GetPosition(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_GetPosition_Handler(Context, FuncKey);
}

void UT_DefaultHandler_SBN_Pack_Data(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = UT_Hook_GetArgValueByName(Context, "DataBufSz", size_t);
    SBN_UT_Pack_Handler(Context, FuncKey, UT_Hook_GetArgValueByName(Context, "DataBuf", const void *), Sz);
}

void UT_DefaultHandler_SBN_Pack_UInt8(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = sizeof(uint8);
    SBN_UT_Pack_Handler(Context, FuncKey, UT_Hook_GetArgPtr(Context, "Data", Sz), Sz);
}

void UT_DefaultHandler_SBN_Pack_UInt16(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = sizeof(uint16);
    SBN_UT_Pack_Handler(Context, FuncKey, UT_Hook_GetArgPtr(Context, "Data", Sz), Sz);
}

void UT_DefaultHandler_SBN_Pack_Int16(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = sizeof(int16);
    SBN_UT_Pack_Handler(Context, FuncKey, UT_Hook_GetArgPtr(Context, "Data", Sz), Sz);
}

void UT_DefaultHandler_SBN_Pack_UInt32(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = sizeof(uint32);
    SBN_UT_Pack_Handler(Context, FuncKey, UT_Hook_GetArgPtr(Context, "Data", Sz), Sz);
}

void UT_DefaultHandler_SBN_Pack_Time(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = sizeof(SBN_Timestamp_t);
    SBN_UT_Pack_Handler(Context, FuncKey, UT_Hook_GetArgPtr(Context, "Data", Sz), Sz);
}

void UT_DefaultHandler_SBN_Pack_MsgID(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = sizeof(CFE_SB_MsgId_t);
    SBN_UT_Pack_Handler(Context, FuncKey, UT_Hook_GetArgPtr(Context, "Data", Sz), Sz);
}

void UT_DefaultHandler_SBN_Unpack_Data(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    size_t Sz = UT_Hook_GetArgValueByName(Context, "Sz", size_t);
    SBN_UT_Unpack_Handler(Context, FuncKey, Sz);
}

void UT_DefaultHandler_SBN_Unpack_UInt8(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_Unpack_Handler(Context, FuncKey, sizeof(uint8));
}

void UT_DefaultHandler_SBN_Unpack_UInt16(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_Unpack_Handler(Context, FuncKey, sizeof(uint16));
}

void UT_DefaultHandler_SBN_Unpack_Int16(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_Unpack_Handler(Context, FuncKey, sizeof(uint16));
}

void UT_DefaultHandler_SBN_Unpack_UInt32(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_Unpack_Handler(Context, FuncKey, sizeof(uint32));
}

void UT_DefaultHandler_SBN_Unpack_Time(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_Unpack_Handler(Context, FuncKey, sizeof(SBN_Timestamp_t));
}

void UT_DefaultHandler_SBN_Unpack_MsgID(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_UT_Unpack_Handler(Context, FuncKey, sizeof(CFE_SB_MsgId_t));
}
