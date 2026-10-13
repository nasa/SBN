/**
 * \file
 *   This file contains the source code for the Talker App Ground Command-handling functions
 */

/*
** Include Files:
*/
#include "talker_app.h"
#include "talker_app_cmds.h"
#include "talker_app_msgids.h"
#include "talker_app_eventids.h"
#include "talker_app_version.h"
#include "talker_app_tbl.h"
#include "talker_app_utils.h"
#include "talker_app_msg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t TALKER_APP_SendHkCmd(const TALKER_APP_SendHkCmd_t *Msg)
{
    int i;

    /*
    ** Get command execution counters...
    */
    TALKER_APP_Data.HkTlm.Payload.CommandErrorCounter = TALKER_APP_Data.ErrCounter;
    TALKER_APP_Data.HkTlm.Payload.CommandCounter      = TALKER_APP_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TransmitMsg(CFE_MSG_PTR(TALKER_APP_Data.HkTlm.TelemetryHeader), true);

    /*
    ** Manage any pending table loads, validations, etc.
    */
    for (i = 0; i < TALKER_APP_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(TALKER_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* TALKER NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t TALKER_APP_NoopCmd(const TALKER_APP_NoopCmd_t *Msg)
{
    TALKER_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(TALKER_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "TALKER: NOOP command %s",
                      TALKER_APP_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t TALKER_APP_ResetCountersCmd(const TALKER_APP_ResetCountersCmd_t *Msg)
{
    TALKER_APP_Data.CmdCounter = 0;
    TALKER_APP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(TALKER_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "TALKER: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function Process Ground Station Command                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t TALKER_APP_ProcessCmd(const TALKER_APP_ProcessCmd_t *Msg)
{
    CFE_Status_t               status;
    void *                     TblAddr;
    TALKER_APP_ExampleTable_t *TblPtr;
    const char *               TableName = "TALKER_APP.ExampleTable";

    /* Talker Use of Example Table */

    status = CFE_TBL_GetAddress(&TblAddr, TALKER_APP_Data.TblHandles[0]);

    if (status < CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Talker App: Fail to get table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    TblPtr = TblAddr;
    CFE_ES_WriteToSysLog("Talker App: Example Table Value 1: %d  Value 2: %d", TblPtr->Int1, TblPtr->Int2);

    TALKER_APP_GetCrc(TableName);

    status = CFE_TBL_ReleaseAddress(TALKER_APP_Data.TblHandles[0]);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Talker App: Fail to release table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* A simple example command that displays a passed-in value                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t TALKER_APP_DisplayParamCmd(const TALKER_APP_DisplayParamCmd_t *Msg)
{
    CFE_EVS_SendEvent(TALKER_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "TALKER_APP: ValU32=%lu, ValI16=%d, ValStr=%s", (unsigned long)Msg->Payload.ValU32,
                      (int)Msg->Payload.ValI16, Msg->Payload.ValStr);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Send the current value of the counter                                      */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t TALKER_APP_Send_Counter(void)
{

    // Loading the counter data to message - CKC
    TALKER_APP_Data.counterMsg.Payload.counter = TALKER_APP_Data.counterNum;

    // Sending the message to the software bus - CKC
    CFE_SB_TransmitMsg(CFE_MSG_PTR(TALKER_APP_Data.counterMsg.CommandHeader), true);

    // Closing the gate - CKC
    TALKER_APP_Data.counterGate = false;

    // Logging event with cFE - CKC
    CFE_EVS_SendEvent(TALKER_COUNTER_SENT, CFE_EVS_EventType_INFORMATION,
                      "TALKER: Counter sent with value: %d", TALKER_APP_Data.counterMsg.Payload.counter);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Check that the value received matches the expectation                      */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t TALKER_APP_Listens(const LISTENER_APP_Counter_t *Msg)
{
    if (Msg->Payload.counterRelay != TALKER_APP_Data.counterNum){
        // Logging event with cFE - CKC
        CFE_EVS_SendEvent(TALKER_COUNTER_SENT, CFE_EVS_EventType_INFORMATION,
                        "TALKER: Talker and Listen mismatch: %d != %d", TALKER_APP_Data.counterNum, Msg->Payload.counterRelay);

        return CFE_SUCCESS;
    }

    // Logging event with cFE - CKC
    CFE_EVS_SendEvent(TALKER_COUNTER_SENT, CFE_EVS_EventType_INFORMATION,
                      "TALKER: Talker and Listen match: %d == %d\n", TALKER_APP_Data.counterNum, Msg->Payload.counterRelay);

    // Increment Talker-app counter - CKC
    TALKER_APP_Data.counterNum++;

    // Opening the gate - CKC
    TALKER_APP_Data.counterGate = true;

    return CFE_SUCCESS;
}
