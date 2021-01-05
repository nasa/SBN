#ifndef _fib_app_h_
#define _fib_app_h_

#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "fib_app_msgids.h"
#include "fib_app_msg.h"

#define FIB_PIPE_DEPTH 32

typedef struct
{
    uint32 RunStatus;
    uint32 prev1, prev2;

    CFE_SB_PipeId_t Pipe;
} Fib_AppData_t;

void FIB_AppMain(void);

#endif
