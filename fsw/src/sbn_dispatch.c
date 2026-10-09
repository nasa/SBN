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

/************************************************************************/
/** \brief Verify message length.
**
**  \par Description
**       Checks if the actual length of a software bus message matches
**       the expected length and sends an error event if a mismatch
**       occurs.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr        A #CFE_MSG_Message_t pointer that
**                              references the software bus message
**
**  \param [in]   ExpectedLen   The expected length of the message
**                              based upon the command code.
**
**  @param [in]   MsgName       Text name of the message expected.
**
**  \retval true The length is as expected.
**  \retval false The length is not as expected.
**
*************************************************************************/
static bool SBN_VerifyMsgLen(const CFE_MSG_Message_t *MsgPtr, uint16 ExpectedLen, const char *MsgName)
{
    CFE_MSG_Size_t    ActualLen = 0;
    CFE_MSG_FcnCode_t FcnCode   = 0;

    if (CFE_MSG_GetSize(MsgPtr, &ActualLen) != CFE_SUCCESS)
    {
        EVSSendErr(SBN_CMD_EID, "invalid hk message (Name=%s)", MsgName);
        return false;
    }

    if (ExpectedLen != ActualLen)
    {
        if (CFE_MSG_GetFcnCode(MsgPtr, &FcnCode) != CFE_SUCCESS)
        {
            EVSSendErr(SBN_CMD_EID, "unable to get FcnCode (Name=%s)", MsgName);
            return false;
        }

        if (FcnCode == SBN_HK_CC)
        {
            /*
            ** For a bad HK request, just send the event.  We only increment
            ** the error counter for ground commands and not internal messages.
            */
            EVSSendErr(SBN_CMD_EID,
                       "invalid hk message length (Name=%s ID=0x%04X "
                       "CC=%d Len=%d Expected=%d)",
                       MsgName,
                       FcnCode,
                       (int)FcnCode,
                       (int)ActualLen,
                       (int)ExpectedLen);
        }
        else
        {
            /*
            ** All other cases, increment error counter
            */
            EVSSendErr(SBN_CMD_EID,
                       "invalid message length (Name=%s ID=0x%04X CC=%d Len=%d Expected=%d)",
                       MsgName,
                       FcnCode,
                       (int)FcnCode,
                       (int)ActualLen,
                       (int)ExpectedLen);

            SBN_AppData.CmdErrCnt++;
        } /* end if */

        return false;
    } /* end if */

    return true;
} /* end VerifyMsgLen */

/*******************************************************************/
/*                                                                 */
/* Process a command pipe message                                  */
/*                                                                 */
/*******************************************************************/
void SBN_HandleCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t           MsgId;
    CFE_MSG_FcnCode_t        FcnCode = 0;
    const CFE_MSG_Message_t *MsgPtr;

    static CFE_SB_MsgId_t CMD_MID = CFE_SB_MSGID_RESERVED;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID = CFE_SB_ValueToMsgId(SBN_CMD_MID);
    }

    MsgPtr = &SBBufPtr->Msg;

    if (CFE_MSG_GetMsgId(MsgPtr, &MsgId) != CFE_SUCCESS)
    {
        SBN_AppData.CmdErrCnt++;
        EVSSendErr(SBN_CMD_EID, "invalid FcnCode");
        return;
    }

    if (!CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        SBN_AppData.CmdErrCnt++;
        EVSSendErr(SBN_CMD_EID, "invalid command pipe MsgId");
        return;
    } /* end if */

    if (CFE_MSG_GetFcnCode(MsgPtr, &FcnCode) != CFE_SUCCESS)
    {
        SBN_AppData.CmdErrCnt++;
        EVSSendErr(SBN_CMD_EID, "invalid FcnCode (FcnCode=0x%04X)", FcnCode);
        return;
    }

    switch (FcnCode)
    {
        case SBN_NOOP_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_NoopCmd_t), "no-op"))
            {
                SBN_NoopCmd((const SBN_NoopCmd_t *)MsgPtr);
            }
            else
            {
                /* this one had an extra special event with it */
                EVSSendErr(SBN_CMD_EID, "invalid no-op command");
            }
            break;

        case SBN_HK_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_SendHkCmd_t), "hk"))
            {
                SBN_SendHkCmd((const SBN_SendHkCmd_t *)MsgPtr);
            }
            break;
        case SBN_HK_NET_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_SendHkNetCmd_t), "hk net"))
            {
                SBN_SendHkNetCmd((const SBN_SendHkNetCmd_t *)MsgPtr);
            }
            break;
        case SBN_HK_PEER_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_SendHkPeerCmd_t), "hk peer"))
            {
                SBN_SendHkPeerCmd((const SBN_SendHkPeerCmd_t *)MsgPtr);
            }
            break;
        case SBN_HK_PEERSUBS_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_SendHkPeerSubsCmd_t), "peer subs"))
            {
                SBN_SendHkPeerSubsCmd((const SBN_SendHkPeerSubsCmd_t *)MsgPtr);
            }
            break;
        case SBN_HK_MYSUBS_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_SendHkMySubsCmd_t), "my subs"))
            {
                SBN_SendHkMySubsCmd((const SBN_SendHkMySubsCmd_t *)MsgPtr);
            }
            break;
        case SBN_HK_RESET_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_ResetCmd_t), "reset counters"))
            {
                SBN_ResetCmd((const SBN_ResetCmd_t *)MsgPtr);
            }
            break;
        case SBN_HK_RESET_PEER_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_ResetPeerCmd_t), "reset peer"))
            {
                SBN_ResetPeerCmd((const SBN_ResetPeerCmd_t *)MsgPtr);
            }
            break;

        case SBN_SCH_WAKEUP_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_WakeupCmd_t), "wakeup"))
            {
                SBN_WakeupCmd((const SBN_WakeupCmd_t *)MsgPtr);
            }
            break;
        case SBN_TBL_CC:
            if (SBN_VerifyMsgLen(MsgPtr, sizeof(SBN_ReloadTblCmd_t), "reloadtbl"))
            {
                SBN_ReloadTblCmd((const SBN_ReloadTblCmd_t *)MsgPtr);
            }
            break;
        default:
            SBN_AppData.CmdErrCnt++;
            EVSSendErr(SBN_CMD_EID, "invalid command code (ID=0x%04X, CC=%d)", FcnCode, FcnCode);
            break;
    } /* end switch */
} /* end SBN_HandleCommand */
