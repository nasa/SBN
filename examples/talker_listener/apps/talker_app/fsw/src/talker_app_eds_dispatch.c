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

#include "talker_app_eds_dispatcher.h"
#include "talker_app_eds_dictionary.h"

/*
 * Define a lookup table for TALKER app command codes
 */
static const EdsDispatchTable_TALKER_APP_Application_CFE_SB_Telecommand_t TALKER_TC_DISPATCH_TABLE = {
    .CMD     = {.NoopCmd_indication          = TALKER_APP_NoopCmd,
            .ResetCountersCmd_indication = TALKER_APP_ResetCountersCmd,
            .ProcessCmd_indication       = TALKER_APP_ProcessCmd,
            .DisplayParamCmd_indication  = TALKER_APP_DisplayParamCmd},
    .SEND_HK = {.indication = TALKER_APP_SendHkCmd}};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the TALKER    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void TALKER_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_Status_t      Status;
    CFE_SB_MsgId_t    MsgId;
    CFE_MSG_Size_t    MsgSize;
    CFE_MSG_FcnCode_t MsgFc;

    Status = EdsDispatch_TALKER_APP_Application_Telecommand(SBBufPtr, &TALKER_TC_DISPATCH_TABLE);

    if (Status != CFE_SUCCESS)
    {
        CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
        CFE_MSG_GetSize(&SBBufPtr->Msg, &MsgSize);
        CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &MsgFc);
        ++TALKER_APP_Data.ErrCounter;

        if (Status == CFE_STATUS_UNKNOWN_MSG_ID)
        {
            CFE_EVS_SendEvent(TALKER_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "TALKER: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
        }
        else if (Status == CFE_STATUS_WRONG_MSG_LENGTH)
        {
            CFE_EVS_SendEvent(TALKER_APP_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u",
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)MsgFc, (unsigned int)MsgSize);
        }
        else
        {
            CFE_EVS_SendEvent(TALKER_APP_CC_ERR_EID, CFE_EVS_EventType_ERROR,
                              "TALKER: Invalid ground command code: CC = %d", (int)MsgFc);
        }
    }
}
