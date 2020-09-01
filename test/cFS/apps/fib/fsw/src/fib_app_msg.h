#ifndef _fib_app_msg_h_
#define _fib_app_msg_h_

typedef struct {
    uint8 TlmHdr[CFE_SB_TLM_HDR_SIZE];
    uint32 fib;
} fib_tlm_t;

#endif
