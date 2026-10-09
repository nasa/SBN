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

#include "sbn_coveragetest_common.h"
#include "cfe_msgids.h"
#include "sbn_pack.h"

uint8  Buf[22];
Pack_t Pack;

void Test_Pack(void)
{
    UtAssert_True(Pack_Init(&Pack, Buf, sizeof(Buf), true), "pack init");
    UtAssert_True(Pack_UInt8(&Pack, (uint8)1), "pack uint8");                    // 1 byte
    UtAssert_True(Pack_UInt8(&Pack, (uint8)255), "pack uint8");                  // 2 bytes
    UtAssert_True(Pack_UInt16(&Pack, (uint16)2), "pack uint16");                 // 4 bytes
    UtAssert_True(Pack_Int16(&Pack, (int16)-2), "pack int16");                   // 6 bytes
    UtAssert_True(Pack_UInt32(&Pack, (uint32)3), "pack uint32");                 // 10 bytes
    UtAssert_True(Pack_MsgID(&Pack, CFE_SB_ValueToMsgId(0xdead)), "pack msgid"); // 14 bytes
    UtAssert_True(Pack_Time(&Pack, 123456), "pack time");                        // 22 bytes
    UtAssert_True(!Pack_Time(&Pack, 123456),
                  "pack time 2"); // should fail, out of space

    UtAssert_True(Pack_Init(&Pack, Buf, sizeof(Buf), false), "pack init 2");

    uint8 u8;
    UtAssert_True(Unpack_UInt8(&Pack, &u8), "unpack uint8"); // 1 byte
    UtAssert_UINT32_EQ(u8, 1);

    UtAssert_True(Unpack_UInt8(&Pack, &u8), "unpack uint8"); // 2 bytes
    UtAssert_UINT32_EQ(u8, 255);

    uint16 u16;
    UtAssert_True(Unpack_UInt16(&Pack, &u16), "unpack uint16"); // 4 bytes
    UtAssert_UINT32_EQ(u16, 2);

    int16 i16;
    UtAssert_True(Unpack_Int16(&Pack, &i16), "unpack int16"); // 6 bytes
    UtAssert_INT32_EQ(i16, -2);

    uint32 u32;
    UtAssert_True(Unpack_UInt32(&Pack, &u32), "unpack uint32"); // 10 bytes
    UtAssert_UINT32_EQ(u32, 3);

    CFE_SB_MsgId_t MsgID;
    UtAssert_True(Unpack_MsgID(&Pack, &MsgID), "unpack msgid"); // 14 bytes
    UtAssert_UINT32_EQ(MsgID.Value, 0xdead);

    SBN_Timestamp_t T;
    UtAssert_True(Unpack_Time(&Pack, &T), "unpack time"); // 22 bytes
    UtAssert_EQ(SBN_Timestamp_t, T, 123456);

    UtAssert_True(!Unpack_UInt8(&Pack, &u8), "unpack uint8");
    UtAssert_True(!Unpack_UInt16(&Pack, &u16), "unpack uint16");
    UtAssert_True(!Unpack_Int16(&Pack, &i16), "unpack int16");
    UtAssert_True(!Unpack_UInt32(&Pack, &u32), "unpack uint32");
    UtAssert_True(!Unpack_MsgID(&Pack, &MsgID), "unpack msgid");
    UtAssert_True(!Unpack_Time(&Pack, &T), "unpack time");
} /* end Test_Pack() */

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(Pack);
}
