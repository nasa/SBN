#include "sbn_pack.h"
#include <string.h> /* memcpy */

#ifdef SOFTWARE_BIG_BIT_ORDER
    #define CFE_MAKE_BIG32(n) (n)
#else /* !SOFTWARE_BIG_BIT_ORDER */
    #define CFE_MAKE_BIG32(n) ( (((n) << 24) & 0xFF000000) | (((n) << 8) & 0x00FF0000) | (((n) >> 8) & 0x0000FF00) | (((n) >> 24) & 0x000000FF) )
#endif /* SOFTWARE_BIG_BIT_ORDER */

bool Pack_Init(Pack_t *PackPtr, void *Buf, size_t BufSz, bool ClearFlag)
{
    PackPtr->Buf = Buf;
    PackPtr->BufSz = BufSz;
    PackPtr->BufUsed = 0;
    if(ClearFlag)
    {
        memset(Buf, 0, BufSz);
    }/* end if */
    return true;
}/* end Pack_Init() */

bool Pack_Data(Pack_t *PackPtr, void *DataBuf, size_t DataBufSz)
{
    if(PackPtr->BufUsed + DataBufSz > PackPtr->BufSz)
    {
        /* print an error? */
        return false;
    }

    memcpy((uint8 *)PackPtr->Buf + PackPtr->BufUsed, DataBuf, DataBufSz);
    PackPtr->BufUsed += DataBufSz;

    return true;
}/* end Pack_Data() */

bool Pack_UInt8(Pack_t *PackPtr, uint8 Data)
{
    return Pack_Data(PackPtr, &Data, sizeof(Data));
}/* end Pack_UInt8() */

bool Pack_UInt16(Pack_t *PackPtr, uint16 Data)
{
    uint16 D = CFE_MAKE_BIG16(Data);
    return Pack_Data(PackPtr, &D, sizeof(D));
}/* end Pack_UInt16() */

bool Pack_Int16(Pack_t *PackPtr, int16 Data)
{
    int16 D = CFE_MAKE_BIG16(Data);
    return Pack_Data(PackPtr, &D, sizeof(D));
}/* end Pack_Int16() */

bool Pack_UInt32(Pack_t *PackPtr, uint32 Data)
{
    uint32 D = CFE_MAKE_BIG32(Data);
    return Pack_Data(PackPtr, &D, sizeof(D));
}/* end Pack_UInt32() */

bool Pack_Time(Pack_t *PackPtr, OS_time_t Data)
{
    OS_time_t D;
    D.seconds = CFE_MAKE_BIG32(Data.seconds);
    D.microsecs = CFE_MAKE_BIG32(Data.microsecs);
    return Pack_Data(PackPtr, &D, sizeof(D));
}/* end Pack_Time() */

bool Pack_MsgID(Pack_t *PackPtr, CFE_SB_MsgId_t Data)
{
    CFE_SB_MsgId_t D;
    D = CFE_MAKE_BIG16(Data);
    return Pack_Data(PackPtr, &D, sizeof(D));
}/* end Pack_MsgID() */

void *Pack_Get(Pack_t *PackPtr)
{
    return PackPtr->Buf;
}/* end Pack_Get() */

bool Unpack_Data(Pack_t *Pack, void *DataBuf, size_t Sz)
{
    if(Pack->BufUsed + Sz > Pack->BufSz)
    {
        return false;
    }

    memcpy(DataBuf, (uint8 *)Pack->Buf + Pack->BufUsed, Sz);

    Pack->BufUsed += Sz;

    return true;
}/* end Unpack_Data() */

bool Unpack_UInt8(Pack_t *Pack, uint8 *DataBuf)
{
    return Unpack_Data(Pack, DataBuf, sizeof(*DataBuf));
}/* end Unpack_UInt8() */

bool Unpack_UInt16(Pack_t *Pack, uint16 *DataBuf)
{
    uint16 D;
    if (!Unpack_Data(Pack, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG16(D);
    return true;
}/* end Unpack_UInt16() */

bool Unpack_Int16(Pack_t *Pack, int16 *DataBuf)
{
    int16 D;
    if (!Unpack_Data(Pack, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG16(D);
    return true;
}/* end Unpack_Int16() */

bool Unpack_UInt32(Pack_t *Pack, uint32 *DataBuf)
{
    uint32 D;
    if (!Unpack_Data(Pack, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG32(D);
    return true;
}/* end Unpack_UInt32() */

bool Unpack_MsgID(Pack_t *Pack, CFE_SB_MsgId_t *DataBuf)
{
    CFE_SB_MsgId_t D;
    if (!Unpack_Data(Pack, &D, sizeof(D)))
    {
        return false;
    }
    *DataBuf = CFE_MAKE_BIG16(D);
    return true;
}/* end Unpack_MsgID() */
