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

#include "sbn_pack.h"
#include <string.h> /* memcpy */

void SBN_Pack_InitBase(SBN_PackBase_t *PackPtr, uint8 *Buf, size_t BufSz)
{
    PackPtr->Buf     = Buf;
    PackPtr->BufSz   = BufSz;
    PackPtr->BufUsed = 0;
} /* end SBN_Pack_Init() */

bool SBN_Pack_Init(SBN_PackState_t *PackPtr, void *Buf, size_t BufSz, bool ClearFlag)
{
    SBN_Pack_InitBase(&PackPtr->Base, (uint8 *)Buf, BufSz);
    if (ClearFlag)
    {
        memset(Buf, 0, BufSz);
    } /* end if */
    return true;
}

bool SBN_Unpack_Init(SBN_UnpackState_t *PackPtr, const void *Buf, size_t BufSz)
{
    /* Note, the unpack APIs will not write to the buffer, it is just stored internally as uint8* */
    SBN_Pack_InitBase(&PackPtr->Base, (uint8 *)Buf, BufSz);
    return true;
}

bool SBN_Pack_Impl(SBN_PackBase_t *PackPtr, const void *DataBuf, size_t DataBufSz)
{
    if (PackPtr->BufUsed + DataBufSz > PackPtr->BufSz)
    {
        /* print an error? */
        return false;
    }

    memcpy(PackPtr->Buf + PackPtr->BufUsed, DataBuf, DataBufSz);
    PackPtr->BufUsed += DataBufSz;

    return true;
} /* end SBN_Pack_Data() */

bool SBN_Pack_Data(SBN_PackState_t *PackPtr, const void *DataBuf, size_t DataBufSz)
{
    return SBN_Pack_Impl(&PackPtr->Base, DataBuf, DataBufSz);
}

bool SBN_Pack_UInt8(SBN_PackState_t *PackPtr, uint8 Data)
{
    return SBN_Pack_Data(PackPtr, &Data, sizeof(Data));
} /* end SBN_Pack_UInt8() */

bool SBN_Pack_UInt16(SBN_PackState_t *PackPtr, uint16 Data)
{
    uint16 D = CFE_MAKE_BIG16(Data);
    return SBN_Pack_Data(PackPtr, &D, sizeof(D));
} /* end SBN_Pack_UInt16() */

bool SBN_Pack_Int16(SBN_PackState_t *PackPtr, int16 Data)
{
    int16 D = CFE_MAKE_BIG16(Data);
    return SBN_Pack_Data(PackPtr, &D, sizeof(D));
} /* end SBN_Pack_Int16() */

bool SBN_Pack_UInt32(SBN_PackState_t *PackPtr, uint32 Data)
{
    uint32 D = CFE_MAKE_BIG32(Data);
    return SBN_Pack_Data(PackPtr, &D, sizeof(D));
} /* end SBN_Pack_UInt32() */

bool SBN_Pack_Time(SBN_PackState_t *PackPtr, SBN_Timestamp_t Data)
{
    return SBN_Pack_Data(PackPtr, &Data, sizeof(Data));
} /* end SBN_Pack_Time() */

bool SBN_Pack_MsgID(SBN_PackState_t *PackPtr, CFE_SB_MsgId_t Data)
{
    uint32 D;
    D = CFE_MAKE_BIG32(CFE_SB_MsgIdToValue(Data));
    return SBN_Pack_Data(PackPtr, &D, sizeof(D));
} /* end SBN_Pack_MsgID() */

bool SBN_Unpack_Impl(SBN_PackBase_t *PackPtr, void *DataBuf, size_t Sz)
{
    if (PackPtr->BufUsed + Sz > PackPtr->BufSz)
    {
        return false;
    }

    memcpy(DataBuf, PackPtr->Buf + PackPtr->BufUsed, Sz);

    PackPtr->BufUsed += Sz;

    return true;
} /* end SBN_Unpack_Data() */

bool SBN_Unpack_Data(SBN_UnpackState_t *PackPtr, void *DataBuf, size_t Sz)
{
    return SBN_Unpack_Impl(&PackPtr->Base, DataBuf, Sz);
}

bool SBN_Unpack_UInt8(SBN_UnpackState_t *PackPtr, uint8 *DataBuf)
{
    return SBN_Unpack_Data(PackPtr, DataBuf, sizeof(*DataBuf));
} /* end SBN_Unpack_UInt8() */

bool SBN_Unpack_UInt16(SBN_UnpackState_t *PackPtr, uint16 *DataBuf)
{
    uint16 D;
    if (!SBN_Unpack_Data(PackPtr, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG16(D);
    return true;
} /* end SBN_Unpack_UInt16() */

bool SBN_Unpack_Int16(SBN_UnpackState_t *PackPtr, int16 *DataBuf)
{
    int16 D;
    if (!SBN_Unpack_Data(PackPtr, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG16(D);
    return true;
} /* end SBN_Unpack_Int16() */

bool SBN_Unpack_UInt32(SBN_UnpackState_t *PackPtr, uint32 *DataBuf)
{
    uint32 D;
    if (!SBN_Unpack_Data(PackPtr, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG32(D);
    return true;
} /* end SBN_Unpack_UInt32() */

bool SBN_Unpack_Time(SBN_UnpackState_t *PackPtr, SBN_Timestamp_t *DataBuf)
{
    SBN_Timestamp_t D;
    if (!SBN_Unpack_Data(PackPtr, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = D;
    return true;
} /* end SBN_Unpack_Time() */

bool SBN_Unpack_MsgID(SBN_UnpackState_t *PackPtr, CFE_SB_MsgId_t *DataBuf)
{
    uint32 D;
    if (!SBN_Unpack_Data(PackPtr, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_SB_ValueToMsgId(CFE_MAKE_BIG32(D));
    return true;
} /* end SBN_Unpack_MsgID() */
