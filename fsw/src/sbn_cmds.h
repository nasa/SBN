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

#ifndef _sbn_cmds_h_
#define _sbn_cmds_h_

#include "cfe_error.h"
#include "sbn_msg.h"

/*************************************************************************
** Function Prototypes
*************************************************************************/

/************************************************************************/
/** \brief Noop command
**
**  \par Description
**       Processes a noop ground command.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_NOOP_CC
**
*************************************************************************/
CFE_Status_t SBN_NoopCmd(const SBN_NoopCmd_t *MsgPtr);

/************************************************************************/
/** \brief Reset counters command
**
**  \par Description
**       Processes a reset counters command which will reset the
**       following SBN application counters to zero:
**         - Command counter
**         - Command error counter
**         - App messages sent counter for each peer
**         - App message send error counter for each peer
**         - App messages received counter for each peer
**         - App message receive error counter for each peer
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_HK_RESET_CC
**
*************************************************************************/
CFE_Status_t SBN_ResetCmd(const SBN_ResetCmd_t *MsgPtr);

/************************************************************************/
/** \brief Reset Peer counters command
**
**  \par Description
**       Reset the counters for a specified peer.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
**                       references the software bus message
**
**  \sa #SBN_HK_RESET_PEER_CC
**
*************************************************************************/
CFE_Status_t SBN_ResetPeerCmd(const SBN_ResetPeerCmd_t *MsgPtr);

/************************************************************************/
/** \brief Housekeeping request command
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 */
CFE_Status_t SBN_SendHkCmd(const SBN_SendHkCmd_t *MsgPtr);

/************************************************************************/
/** \brief Request for housekeeping for one network.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 */
CFE_Status_t SBN_SendHkNetCmd(const SBN_SendHkNetCmd_t *MsgPtr);

/************************************************************************/
/** \brief Request for housekeeping for one peer.
 *
 *  \par Description
 *       Processes an on-board housekeeping request message.
 *
 *  \par Assumptions, External Events, and Notes:
 *       This message does not affect the command execution counter
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 */
CFE_Status_t SBN_SendHkPeerCmd(const SBN_SendHkPeerCmd_t *MsgPtr);

/************************************************************************/
/** \brief Send My Subscriptions
 *
 *  \par Assumptions, External Events, and Notes:
 *       None
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 *  \sa #SBN_HK_MYSUBS_CC
 */
CFE_Status_t SBN_SendHkMySubsCmd(const SBN_SendHkMySubsCmd_t *MsgPtr);

/************************************************************************/
/** \brief Reloads the config table.
 *
 *  \param [in]   MsgPtr A #CFE_MSG_Message_t pointer that
 *                       references the software bus message
 *
 *  \sa #SBN_TBL_CC
 */
CFE_Status_t SBN_ReloadTblCmd(const SBN_ReloadTblCmd_t *MsgPtr);

/************************************************************************/
/** \brief Send A Peer's Subscriptions
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   MsgPtr   A #CFE_MSG_Message_t pointer that
**                         references the software bus message
**
**  \sa #SBN_HK_MYSUBS_CC
**
*************************************************************************/
CFE_Status_t SBN_SendHkPeerSubsCmd(const SBN_SendHkPeerSubsCmd_t *MsgPtr);

/* Placeholder */
CFE_Status_t SBN_WakeupCmd(const SBN_WakeupCmd_t *MsgPtr);

#endif /* _sbn_cmds_h_ */
