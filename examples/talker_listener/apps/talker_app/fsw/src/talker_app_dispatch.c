/**
 * \file
 *   This file contains the source code for the Talker App.
 */

/*
** Include Files:
*/
#include "talker_app.h"
#include "talker_app_dispatch.h"
#include "talker_app_cmds.h"
#include "talker_app_eventids.h"
#include "talker_app_msgids.h"
#include "talker_app_msg.h"
#include "listener_app_msgids.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool TALKER_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
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

        CFE_EVS_SendEvent(TALKER_APP_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        TALKER_APP_Data.ErrCounter++;
    }

    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* TALKER ground commands                                                     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void TALKER_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process TALKER app ground commands
    */
    switch (CommandCode)
    {
        case TALKER_APP_NOOP_CC:
            if (TALKER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(TALKER_APP_NoopCmd_t)))
            {
                TALKER_APP_NoopCmd((const TALKER_APP_NoopCmd_t *)SBBufPtr);
            }
            break;

        case TALKER_APP_RESET_COUNTERS_CC:
            if (TALKER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(TALKER_APP_ResetCountersCmd_t)))
            {
                TALKER_APP_ResetCountersCmd((const TALKER_APP_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case TALKER_APP_PROCESS_CC:
            if (TALKER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(TALKER_APP_ProcessCmd_t)))
            {
                TALKER_APP_ProcessCmd((const TALKER_APP_ProcessCmd_t *)SBBufPtr);
            }
            break;

        case TALKER_APP_DISPLAY_PARAM_CC:
            if (TALKER_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(TALKER_APP_DisplayParamCmd_t)))
            {
                TALKER_APP_DisplayParamCmd((const TALKER_APP_DisplayParamCmd_t *)SBBufPtr);
            }
            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(TALKER_APP_CC_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid ground command code: CC = %d",
                              CommandCode);
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the TALKER    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void TALKER_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case TALKER_APP_CMD_MID:
            TALKER_APP_ProcessGroundCommand(SBBufPtr);
            break;

        case TALKER_APP_SEND_HK_MID:
            TALKER_APP_SendHkCmd((const TALKER_APP_SendHkCmd_t *)SBBufPtr);
            break;

	case LISTENER_APP_CONFIRM_MID:
            TALKER_APP_Listens((const LISTENER_APP_Counter_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(TALKER_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "TALKER: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}
