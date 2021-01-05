#include "cfe_platform_cfg.h"
#include "fib_app_events.h"
#include "fib_app.h"

Fib_AppData_t Fib_AppData;

void FIB_AppMain(void)
{
    int32     status;
    fib_tlm_t tlm;

    Fib_AppData.prev1 = 1;
    Fib_AppData.prev2 = 1;

    CFE_SB_InitMsg((CFE_SB_Msg_t *)&tlm, FIB_TLM_MID, sizeof(tlm), true);

    CFE_ES_RegisterApp();

    CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

    Fib_AppData.RunStatus = CFE_ES_RunStatus_APP_RUN;

    if (CFE_SB_CreatePipe(&Fib_AppData.Pipe, FIB_PIPE_DEPTH, "FIB_CMD_PIPE") != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_ERROR, "error creating pipe");
        return;
    }

    if ((status = CFE_SB_Subscribe(FIB_CMD_MID, Fib_AppData.Pipe)) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_ERROR, "error subscribing to command");
        return;
    }

    CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_INFORMATION, "Fib App Initialized.");

    tlm.fib = 1;
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *)&tlm);
    CFE_SB_SendMsg((CFE_SB_Msg_t *)&tlm); /* send out the 1, 1; as is tradition */
    CFE_SB_SendMsg((CFE_SB_Msg_t *)&tlm);

    while (CFE_ES_RunLoop(&Fib_AppData.RunStatus) == true)
    {
        /* Pend on receipt of command packet */
        CFE_SB_MsgPtr_t MsgPtr;

        status = CFE_SB_RcvMsg(&MsgPtr, Fib_AppData.Pipe, CFE_SB_PEND_FOREVER);

        if (status == CFE_SUCCESS)
        {
            tlm.fib           = Fib_AppData.prev1 + Fib_AppData.prev2;
            Fib_AppData.prev2 = Fib_AppData.prev1;
            Fib_AppData.prev1 = tlm.fib;

            CFE_SB_TimeStampMsg((CFE_SB_Msg_t *)&tlm);
            CFE_SB_SendMsg((CFE_SB_Msg_t *)&tlm);
        }
        else
        {
            CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_ERROR, "FIB APP: SB Pipe Read Error, App Will Exit");

            Fib_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_ExitApp(Fib_AppData.RunStatus);
}
