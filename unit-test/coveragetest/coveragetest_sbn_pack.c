#include "sbn_coveragetest_common.h"
#include "cfe_msgids.h"
#include "sbn_pack.h"

uint8     Buf[20];
Pack_t    Pack;
OS_time_t Time = {0xa, 0xb};

void Test_Pack(void)
{
    UtAssert_True(Pack_Init(&Pack, Buf, sizeof(Buf), true), "pack init");
    UtAssert_True(Pack_UInt8(&Pack, (uint8)1), "pack uint8");               // 1 byte
    UtAssert_True(Pack_UInt8(&Pack, (uint8)255), "pack uint8");             // 2 bytes
    UtAssert_True(Pack_UInt16(&Pack, (uint16)2), "pack uint16");            // 4 bytes
    UtAssert_True(Pack_Int16(&Pack, (int16)-2), "pack int16");              // 6 bytes
    UtAssert_True(Pack_UInt32(&Pack, (uint32)3), "pack uint32");            // 10 bytes
    UtAssert_True(Pack_MsgID(&Pack, (CFE_SB_MsgId_t)0xdead), "pack msgid"); // 12 bytes
    UtAssert_True(Pack_Time(&Pack, Time), "pack time");                     // 20 bytes
    UtAssert_True(!Pack_Time(&Pack, Time), "pack time 2");                  // should fail, out of space

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
    UtAssert_True(Unpack_MsgID(&Pack, &MsgID), "unpack msgid"); // 12 bytes
    UtAssert_UINT32_EQ(MsgID, 0xdead);

    OS_time_t T;
    UtAssert_True(Unpack_UInt32(&Pack, &T.seconds), "unpack time"); // 16 bytes
    UtAssert_UINT32_EQ(T.seconds, 0xa);
    UtAssert_True(Unpack_UInt32(&Pack, &T.microsecs), "unpack time"); // 120 bytes
    UtAssert_UINT32_EQ(T.microsecs, 0xb);

    UtAssert_True(!Unpack_UInt8(&Pack, &u8), "unpack uint8");
    UtAssert_True(!Unpack_UInt16(&Pack, &u16), "unpack uint16");
    UtAssert_True(!Unpack_Int16(&Pack, &i16), "unpack int16");
    UtAssert_True(!Unpack_UInt32(&Pack, &u32), "unpack uint32");
    UtAssert_True(!Unpack_MsgID(&Pack, &MsgID), "unpack msgid");
} /* end Test_Pack() */

void UT_Setup(void) {} /* end UT_Setup() */

void UT_TearDown(void) {} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(Pack);
}
