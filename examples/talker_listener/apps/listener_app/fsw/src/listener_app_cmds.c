/**
 * \file
 *   This file contains the source code for the Listener App Ground Command-handling functions
 */

/*
** Include Files:
*/
#include "listener_app.h"
#include "listener_app_cmds.h"
#include "listener_app_msgids.h"
#include "listener_app_eventids.h"
#include "listener_app_version.h"
#include "listener_app_tbl.h"
#include "listener_app_utils.h"
#include "listener_app_msg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t LISTENER_APP_SendHkCmd(const LISTENER_APP_SendHkCmd_t *Msg)
{
    int i;

    /*
    ** Get command execution counters...
    */
    LISTENER_APP_Data.HkTlm.Payload.CommandErrorCounter = LISTENER_APP_Data.ErrCounter;
    LISTENER_APP_Data.HkTlm.Payload.CommandCounter      = LISTENER_APP_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TransmitMsg(CFE_MSG_PTR(LISTENER_APP_Data.HkTlm.TelemetryHeader), true);

    /*
    ** Manage any pending table loads, validations, etc.
    */
    for (i = 0; i < LISTENER_APP_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(LISTENER_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* LISTENER NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LISTENER_APP_NoopCmd(const LISTENER_APP_NoopCmd_t *Msg)
{
    LISTENER_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(LISTENER_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "LISTENER: NOOP command %s",
                      LISTENER_APP_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t LISTENER_APP_ResetCountersCmd(const LISTENER_APP_ResetCountersCmd_t *Msg)
{
    LISTENER_APP_Data.CmdCounter = 0;
    LISTENER_APP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(LISTENER_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "LISTENER: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function Process Ground Station Command                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t LISTENER_APP_ProcessCmd(const LISTENER_APP_ProcessCmd_t *Msg)
{
    CFE_Status_t               status;
    void *                     TblAddr;
    LISTENER_APP_ExampleTable_t *TblPtr;
    const char *               TableName = "LISTENER_APP.ExampleTable";

    /* Listener Use of Example Table */

    status = CFE_TBL_GetAddress(&TblAddr, LISTENER_APP_Data.TblHandles[0]);

    if (status < CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Listener App: Fail to get table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    TblPtr = TblAddr;
    CFE_ES_WriteToSysLog("Listener App: Example Table Value 1: %d  Value 2: %d", TblPtr->Int1, TblPtr->Int2);

    LISTENER_APP_GetCrc(TableName);

    status = CFE_TBL_ReleaseAddress(LISTENER_APP_Data.TblHandles[0]);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Listener App: Fail to release table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* A simple example command that displays a passed-in value                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LISTENER_APP_DisplayParamCmd(const LISTENER_APP_DisplayParamCmd_t *Msg)
{
    CFE_EVS_SendEvent(LISTENER_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "LISTENER_APP: ValU32=%lu, ValI16=%d, ValStr=%s", (unsigned long)Msg->Payload.ValU32,
                      (int)Msg->Payload.ValI16, Msg->Payload.ValStr);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Receive message from talker app and echo it back                           */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LISTENER_APP_Counter(const TALKER_APP_Counter_t *Msg)
{

    // relaying the talker counter-data to message - CKC
    LISTENER_APP_Data.counterMsg.Payload.counterRelay = Msg->Payload.counter;

    CFE_EVS_SendEvent(LISTENER_COUNTER_SENT, CFE_EVS_EventType_INFORMATION,
                      "LISTENER: sent back to talker a value of: %d", LISTENER_APP_Data.counterMsg.Payload.counterRelay);

    // Sending the message to the software bus - CKC
    CFE_SB_TransmitMsg(CFE_MSG_PTR(LISTENER_APP_Data.counterMsg.CommandHeader), true);

    CFE_EVS_SendEvent(LISTENER_COUNTER_SENT, CFE_EVS_EventType_INFORMATION,
                      "LISTENER: sent back to talker a value of: %d", LISTENER_APP_Data.counterMsg.Payload.counterRelay);


    // Logging event with cFE - CKC
    CFE_EVS_SendEvent(LISTENER_COUNTER_SENT, CFE_EVS_EventType_INFORMATION,
                      "LISTENER: Talker Msg Received with a value of: %d", Msg->Payload.counter);

    return CFE_SUCCESS;
}
