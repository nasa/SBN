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

/* ======== */
/* Includes */
/* ======== */

#include "cfe.h"
#include <string.h> /* memcpy */

#include "sbn_pack.h"

/* ================ */
/* Global Variables */
/* ================ */

SBN_PackState_t   PackBuffer;
SBN_UnpackState_t UnpackBuffer;

/* ==================== */
/* Function Definitions */
/* ==================== */

void SBN_Pack_InitBase(SBN_PackBase_t *PackPtr, uint8 *Buf, size_t BufSz)
{
    PackPtr->Buf     = Buf;
    PackPtr->BufSz   = BufSz;
    PackPtr->BufUsed = 0;
} /* end SBN_Pack_Init() */

bool SBN_Pack_Init(void *Buf, size_t BufSz, bool ClearFlag)
{
    SBN_Pack_InitBase(&(PackBuffer.Base) , (uint8 *)Buf, BufSz);
    if (ClearFlag)
    {
        memset(Buf, 0, BufSz);
    } /* end if */
    return true;
}

bool SBN_Unpack_Init(const void *Buf, size_t BufSz)
{
    /* Note, the unpack APIs will not write to the buffer, it is just stored internally as uint8* */
    SBN_Pack_InitBase(&(UnpackBuffer.Base), (uint8 *)Buf, BufSz);
    return true;
}

uint32 SBN_Pack_GetPosition(void)
{
    return PackBuffer.Base.BufUsed;
}

uint32 SBN_Unpack_GetPosition(void)
{
    return UnpackBuffer.Base.BufUsed;
}

bool SBN_Pack_Impl(const void *DataBuf, size_t DataBufSz)
{
    if (PackBuffer.Base.BufUsed + DataBufSz > PackBuffer.Base.BufSz)
    {
        /* print an error? */
        return false;
    }

    memcpy(PackBuffer.Base.Buf + PackBuffer.Base.BufUsed, DataBuf, DataBufSz);
    PackBuffer.Base.BufUsed += DataBufSz;

    return true;
} /* end SBN_Pack_Data() */

bool SBN_Pack_Data(const void *DataBuf, size_t DataBufSz)
{
    return SBN_Pack_Impl(DataBuf, DataBufSz);
}

bool SBN_Pack_UInt8(uint8 Data)
{
    return SBN_Pack_Data(&Data, sizeof(Data));
} /* end SBN_Pack_UInt8() */

bool SBN_Pack_UInt16(uint16 Data)
{
    uint16 D = CFE_MAKE_BIG16(Data);
    return SBN_Pack_Data(&D, sizeof(D));
} /* end SBN_Pack_UInt16() */

bool SBN_Pack_Int16(int16 Data)
{
    int16 D = CFE_MAKE_BIG16(Data);
    return SBN_Pack_Data(&D, sizeof(D));
} /* end SBN_Pack_Int16() */

bool SBN_Pack_UInt32(uint32 Data)
{
    uint32 D = CFE_MAKE_BIG32(Data);
    return SBN_Pack_Data(&D, sizeof(D));
} /* end SBN_Pack_UInt32() */

bool SBN_Pack_Time(OS_time_t Data)
{
    return SBN_Pack_Data(&Data, sizeof(Data));
} /* end SBN_Pack_Time() */

bool SBN_Pack_MsgID(CFE_SB_MsgId_t Data)
{
    uint32 D;
    D = CFE_MAKE_BIG32(CFE_SB_MsgIdToValue(Data));
    return SBN_Pack_Data(&D, sizeof(D));
} /* end SBN_Pack_MsgID() */

bool SBN_Unpack_Impl(void *DataBuf, size_t Sz)
{
    if (UnpackBuffer.Base.BufUsed + Sz > UnpackBuffer.Base.BufSz)
    {
        return false;
    }

    memcpy(DataBuf, UnpackBuffer.Base.Buf + UnpackBuffer.Base.BufUsed, Sz);

    UnpackBuffer.Base.BufUsed += Sz;

    return true;
} /* end SBN_Unpack_Data() */

bool SBN_Unpack_Data(void *DataBuf, size_t Sz)
{
    return SBN_Unpack_Impl(DataBuf, Sz);
}

bool SBN_Unpack_UInt8(uint8 *DataBuf)
{
    return SBN_Unpack_Data(DataBuf, sizeof(*DataBuf));
} /* end SBN_Unpack_UInt8() */

bool SBN_Unpack_UInt16(uint16 *DataBuf)
{
    uint16 D;
    if (!SBN_Unpack_Data(&D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG16(D);
    return true;
} /* end SBN_Unpack_UInt16() */

bool SBN_Unpack_Int16(int16 *DataBuf)
{
    int16 D;
    if (!SBN_Unpack_Data(&D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG16(D);
    return true;
} /* end SBN_Unpack_Int16() */

bool SBN_Unpack_UInt32(uint32 *DataBuf)
{
    uint32 D;
    if (!SBN_Unpack_Data(&D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG32(D);
    return true;
} /* end SBN_Unpack_UInt32() */

bool SBN_Unpack_Time(OS_time_t *DataBuf)
{
    OS_time_t D;
    if (!SBN_Unpack_Data(&D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = D;
    return true;
} /* end SBN_Unpack_Time() */

bool SBN_Unpack_MsgID(CFE_SB_MsgId_t *DataBuf)
{
        return false;
    *DataBuf = CFE_SB_ValueToMsgId(CFE_MAKE_BIG32(D));
    return true;
} /* end SBN_Unpack_MsgID() */
