#ifndef _fib_app_msg_h_
#define _fib_app_msg_h_

typedef struct
{
    uint8  TlmHdr[sizeof(CFE_MSG_TelemetryHeader_t)];
    uint32 fib;
} fib_tlm_t;

#endif
