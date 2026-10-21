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
 * @file
 *
 * Main header file for the SAMPLE application
 */

#ifndef SBN_DISPATCH_H
#define SBN_DISPATCH_H

/*
** Required header files.
*/
#include "cfe.h"
#include "sbn_msg.h"

#ifdef jphfix
bool SBN_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);
void SBN_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr);
void SBN_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr);
#endif

/************************************************************************/
/** \brief Process a command pipe message
**
**  \par Description
**       Processes a single software bus command pipe message.  Checks
**       the message and command IDs and calls the appropriate routine
**       to handle the message.
**
**  \par Assumptions, External Events, and Notes:
**       None
**
**  \param [in]   SBBufPtr   A #CFE_SB_Buffer_t pointer that
**                         references the software bus message
**
**  \sa #CFE_SB_ReceiveBuffer
**
*************************************************************************/
void SBN_HandleCommand(const CFE_SB_Buffer_t *SBBufPtr);

#endif /* SBN_DISPATCH_H */
