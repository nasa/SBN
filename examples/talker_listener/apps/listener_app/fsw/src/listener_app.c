/**
 * \file
 *   This file contains the source code for the Listener App.
 */

/*
** Include Files:
*/
#include "listener_app.h"
#include "listener_app_cmds.h"
#include "listener_app_utils.h"
#include "listener_app_eventids.h"
#include "listener_app_dispatch.h"
#include "listener_app_tbl.h"
#include "listener_app_version.h"

#include "listener_app_msgids.h"
#include "talker_app_msgids.h"

#include <string.h>

/*
** global data
*/
LISTENER_APP_Data_t LISTENER_APP_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void LISTENER_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(LISTENER_APP_PERF_ID);

    /*
    ** Perform application-specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = LISTENER_APP_Init();
    if (status != CFE_SUCCESS)
    {
        LISTENER_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Listener App Runloop
    */
    while (CFE_ES_RunLoop(&LISTENER_APP_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(LISTENER_APP_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, LISTENER_APP_Data.CommandPipe, 500);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(LISTENER_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            LISTENER_APP_TaskPipe(SBBufPtr);
        }
        else
        {
            // CFE_EVS_SendEvent(LISTENER_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
            //                   "LISTENER APP: SB Pipe Read Error, App Will Exit");

            // LISTENER_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(LISTENER_APP_PERF_ID);

    CFE_ES_ExitApp(LISTENER_APP_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LISTENER_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[LISTENER_APP_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    memset(&LISTENER_APP_Data, 0, sizeof(LISTENER_APP_Data));

    LISTENER_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    LISTENER_APP_Data.counterNum = 0;

    /*
    ** Initialize app configuration data
    */
    LISTENER_APP_Data.PipeDepth = LISTENER_APP_PIPE_DEPTH;

    strncpy(LISTENER_APP_Data.PipeName, "LISTENER_APP_CMD_PIPE", sizeof(LISTENER_APP_Data.PipeName));
    LISTENER_APP_Data.PipeName[sizeof(LISTENER_APP_Data.PipeName) - 1] = 0;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Listener App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(LISTENER_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(LISTENER_APP_HK_TLM_MID),
                     sizeof(LISTENER_APP_Data.HkTlm));

        /*
         ** Initialize confirmation command packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(LISTENER_APP_Data.counterMsg.CommandHeader), CFE_SB_ValueToMsgId(LISTENER_APP_CONFIRM_MID),
                     sizeof(LISTENER_APP_Data.counterMsg));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&LISTENER_APP_Data.CommandPipe, LISTENER_APP_Data.PipeDepth, LISTENER_APP_Data.PipeName);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LISTENER_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Listener App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to Housekeeping request commands
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(LISTENER_APP_SEND_HK_MID), LISTENER_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LISTENER_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Listener App: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(LISTENER_APP_CMD_MID), LISTENER_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LISTENER_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Listener App: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to talker command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(TALKER_APP_COUNTER_MID), LISTENER_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LISTENER_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Listener App: Error Subscribing to Talker Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Register Example Table(s)
        */
        status = CFE_TBL_Register(&LISTENER_APP_Data.TblHandles[0], "ExampleTable", sizeof(LISTENER_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, LISTENER_APP_TblValidationFunc);

        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LISTENER_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Listener App: Error Registering Example Table, RC = 0x%08lX", (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(LISTENER_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, LISTENER_APP_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, LISTENER_APP_CFG_MAX_VERSION_STR_LEN, "Listener App", LISTENER_APP_VERSION,
                                    LISTENER_APP_BUILD_CODENAME, LISTENER_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(LISTENER_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "Listener App Initialized.%s",
                          VersionString);
    }

    return status;
}
