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

    CFE_MSG_Init((CFE_MSG_Message_t *)&tlm, CFE_SB_ValueToMsgId(FIB_TLM_MID), sizeof(tlm));

    CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

    Fib_AppData.RunStatus = CFE_ES_RunStatus_APP_RUN;

    if (CFE_SB_CreatePipe(&Fib_AppData.Pipe, FIB_PIPE_DEPTH, "FIB_CMD_PIPE") != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_ERROR, "error creating pipe");
        return;
    }

    CFE_EVS_SendDbg(FIB_EID, "Subscribing to 0x%04x", FIB_CMD_MID);
    if ((status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(FIB_CMD_MID), Fib_AppData.Pipe)) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_ERROR, "error subscribing to command");
        return;
    }

    CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_INFORMATION, "Fib App Initialized.");

    tlm.fib = 1;
    CFE_SB_TimeStampMsg((CFE_MSG_Message_t *)&tlm);
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&tlm, true); /* send out the 1, 1; as is tradition */
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&tlm, true);

    while (CFE_ES_RunLoop(&Fib_AppData.RunStatus) == true)
    {
        /* Pend on receipt of command packet */
        CFE_SB_Buffer_t *MsgBuf;

        status = CFE_SB_ReceiveBuffer(&MsgBuf, Fib_AppData.Pipe, CFE_SB_PEND_FOREVER);

        if (status == CFE_SUCCESS)
        {
            tlm.fib           = Fib_AppData.prev1 + Fib_AppData.prev2;
            Fib_AppData.prev2 = Fib_AppData.prev1;
            Fib_AppData.prev1 = tlm.fib;

            CFE_SB_TimeStampMsg((CFE_MSG_Message_t *)&tlm);
            CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&tlm, true);
        }
        else
        {
            CFE_EVS_SendEvent(FIB_EID, CFE_EVS_EventType_ERROR, "FIB APP: SB Pipe Read Error, App Will Exit");

            Fib_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_ExitApp(Fib_AppData.RunStatus);
}
