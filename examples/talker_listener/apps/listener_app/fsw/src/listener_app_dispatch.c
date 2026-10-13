/**
 * \file
 *   This file contains the source code for the Listener App.
 */

/*
** Include Files:
*/
#include "listener_app.h"
#include "listener_app_dispatch.h"
#include "listener_app_cmds.h"
#include "listener_app_eventids.h"
#include "listener_app_msgids.h"
#include "listener_app_msg.h"
#include "talker_app_msgids.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool LISTENER_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(LISTENER_APP_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        LISTENER_APP_Data.ErrCounter++;
    }

    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* LISTENER ground commands                                                     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void LISTENER_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process LISTENER app ground commands
    */
    switch (CommandCode)
    {
        case LISTENER_APP_NOOP_CC:
            if (LISTENER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LISTENER_APP_NoopCmd_t)))
            {
                LISTENER_APP_NoopCmd((const LISTENER_APP_NoopCmd_t *)SBBufPtr);
            }
            break;

        case LISTENER_APP_RESET_COUNTERS_CC:
            if (LISTENER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LISTENER_APP_ResetCountersCmd_t)))
            {
                LISTENER_APP_ResetCountersCmd((const LISTENER_APP_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case LISTENER_APP_PROCESS_CC:
            if (LISTENER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LISTENER_APP_ProcessCmd_t)))
            {
                LISTENER_APP_ProcessCmd((const LISTENER_APP_ProcessCmd_t *)SBBufPtr);
            }
            break;

        case LISTENER_APP_DISPLAY_PARAM_CC:
            if (LISTENER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LISTENER_APP_DisplayParamCmd_t)))
            {
                LISTENER_APP_DisplayParamCmd((const LISTENER_APP_DisplayParamCmd_t *)SBBufPtr);
            }
            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(LISTENER_APP_CC_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid ground command code: CC = %d",
                              CommandCode);
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the LISTENER    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void LISTENER_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case LISTENER_APP_CMD_MID:
            LISTENER_APP_ProcessGroundCommand(SBBufPtr);
            break;

        case LISTENER_APP_SEND_HK_MID:
            LISTENER_APP_SendHkCmd((const LISTENER_APP_SendHkCmd_t *)SBBufPtr);
            break;

        case TALKER_APP_COUNTER_MID:
            LISTENER_APP_Counter((const TALKER_APP_Counter_t*)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(LISTENER_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "LISTENER: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}
