/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file
 *   This file contains the source code for the Sample App.
 */

/*
** Include Files:
*/
#include "sbn_app.h"
#include "sbn_dispatch.h"
#include "sbn_cmds.h"
#include "sbn_eventids.h"
#include "sbn_msgids.h"
#include "sbn_msg.h"

#include "sbn_eds_dispatcher.h"
#include "sbn_eds_dictionary.h"

/*
 * Define a lookup table for SAMPLE app command codes
 */
/* clang-format off */
static const EdsDispatchTable_EdsComponent_SBN_Application_CFE_SB_Telecommand_t SBN_TC_DISPATCH_TABLE =
{
    .CMD =
    {
        .NoopCmd_indication = SBN_NoopCmd,
        .ReloadTblCmd_indication = SBN_ReloadTblCmd,
        .ResetCmd_indication = SBN_ResetCmd,
        .ResetPeerCmd_indication = SBN_ResetPeerCmd,
        .SendHkCmd_indication = SBN_SendHkCmd,
        .SendHkMySubsCmd_indication = SBN_SendHkMySubsCmd,
        .SendHkNetCmd_indication = SBN_SendHkNetCmd,
        .SendHkPeerCmd_indication = SBN_SendHkPeerCmd,
        .SendHkPeerSubsCmd_indication = SBN_SendHkPeerSubsCmd,
        .WakeupCmd_indication = SBN_WakeupCmd,
    },
};
/* clang-format on */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the SAMPLE    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void SBN_HandleCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_Status_t      Status;
    CFE_SB_MsgId_t    MsgId;
    CFE_MSG_Size_t    MsgSize;
    CFE_MSG_FcnCode_t MsgFc;

    Status = EdsDispatch_EdsComponent_SBN_Application_Telecommand(SBBufPtr, &SBN_TC_DISPATCH_TABLE);

    if (Status != CFE_SUCCESS)
    {
        CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
        CFE_MSG_GetSize(&SBBufPtr->Msg, &MsgSize);
        CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &MsgFc);
        ++SBN_AppData.CmdErrCnt;

        if (Status == CFE_STATUS_UNKNOWN_MSG_ID)
        {
            EVSSendErr(SBN_CMD_EID,
                       "SAMPLE: invalid command packet,MID = 0x%x",
                       (unsigned int)CFE_SB_MsgIdToValue(MsgId));
        }
        else if (Status == CFE_STATUS_WRONG_MSG_LENGTH)
        {
            EVSSendErr(SBN_CMD_EID,
                       "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u",
                       (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                       (unsigned int)MsgFc,
                       (unsigned int)MsgSize);
        }
        else
        {
            EVSSendErr(SBN_CMD_EID, "SAMPLE: Invalid ground command code: CC = %d", (int)MsgFc);
        }
    }
}
