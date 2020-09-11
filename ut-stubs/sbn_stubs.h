#ifndef _sbn_stubs_h_
#define _sbn_stubs_h_

#include "sbn_interfaces.h"

typedef struct SBN_Unpack_Buf
{
    SBN_MsgSz_t       MsgSz;
    SBN_MsgType_t     MsgType;
    CFE_ProcessorID_t ProcessorID;
    uint8             MsgBuf[256]; /* TODO: use a defined buffer size? */
} SBN_Unpack_Buf_t;

#endif /* _sbn_stubs_h_ */
