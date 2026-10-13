/**
 * \file
 *   This file contains the source code for the Talker App.
 */

/*
** Include Files:
*/
#include "talker_app.h"
#include "talker_app_cmds.h"
#include "talker_app_utils.h"
#include "talker_app_eventids.h"
#include "talker_app_dispatch.h"
#include "talker_app_tbl.h"
#include "talker_app_version.h"

#include "talker_app_msgids.h"
#include "listener_app_msgids.h"

/*
** global data
*/
TALKER_APP_Data_t TALKER_APP_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void TALKER_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(TALKER_APP_PERF_ID);

    /*
    ** Perform application-specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = TALKER_APP_Init();
    if (status != CFE_SUCCESS)
    {
        TALKER_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Talker App Runloop
    */
    while (CFE_ES_RunLoop(&TALKER_APP_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(TALKER_APP_PERF_ID);

        /* Pend on receipt of command packet */

	// Set to a fixed time by Corey and Ivan to make the app not wait
	// forever.
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, TALKER_APP_Data.CommandPipe, 500);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(TALKER_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            TALKER_APP_TaskPipe(SBBufPtr);
        }

        if (TALKER_APP_Data.counterGate) {
            OS_TaskDelay(5000);
            TALKER_APP_Send_Counter();
	}
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(TALKER_APP_PERF_ID);

    CFE_ES_ExitApp(TALKER_APP_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t TALKER_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[TALKER_APP_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    memset(&TALKER_APP_Data, 0, sizeof(TALKER_APP_Data));

    TALKER_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    // Setting counter to zero - CKC
    TALKER_APP_Data.counterNum = 1;

    // Setting gate bool to initial state - CKC
    TALKER_APP_Data.counterGate = true;

    /*
    ** Initialize app configuration data
    */
    TALKER_APP_Data.PipeDepth = TALKER_APP_PIPE_DEPTH;

    strncpy(TALKER_APP_Data.PipeName, "TALKER_APP_CMD_PIPE", sizeof(TALKER_APP_Data.PipeName));
    TALKER_APP_Data.PipeName[sizeof(TALKER_APP_Data.PipeName) - 1] = 0;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Talker App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(TALKER_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(TALKER_APP_HK_TLM_MID),
                     sizeof(TALKER_APP_Data.HkTlm));

        /*
         ** Initialize talker commands for listener (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(TALKER_APP_Data.counterMsg.CommandHeader), CFE_SB_ValueToMsgId(TALKER_APP_COUNTER_MID),
                     sizeof(TALKER_APP_Data.counterMsg));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&TALKER_APP_Data.CommandPipe, TALKER_APP_Data.PipeDepth, TALKER_APP_Data.PipeName);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TALKER_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Talker App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to Housekeeping request commands
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(TALKER_APP_SEND_HK_MID), TALKER_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TALKER_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Talker App: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(TALKER_APP_CMD_MID), TALKER_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TALKER_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Talker App: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to listener command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(LISTENER_APP_CONFIRM_MID), TALKER_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TALKER_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Talker App: Error Subscribing to Listener Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Register Example Table(s)
        */
        status = CFE_TBL_Register(&TALKER_APP_Data.TblHandles[0], "ExampleTable", sizeof(TALKER_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, TALKER_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TALKER_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Talker App: Error Registering Example Table, RC = 0x%08lX", (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(TALKER_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, TALKER_APP_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, TALKER_APP_CFG_MAX_VERSION_STR_LEN, "Talker App", TALKER_APP_VERSION,
                                    TALKER_APP_BUILD_CODENAME, TALKER_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(TALKER_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "Talker App Initialized.%s",
                          VersionString);
    }

    return status;
}
