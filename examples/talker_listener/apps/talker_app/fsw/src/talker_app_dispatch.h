/**
 * @file
 *
 * Main header file for the TALKER application
 */

#ifndef TALKER_APP_DISPATCH_H
#define TALKER_APP_DISPATCH_H

/*
** Required header files.
*/
#include "cfe.h"
#include "talker_app_msg.h"

void TALKER_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr);
void TALKER_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr);
bool TALKER_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

#endif /* TALKER_APP_DISPATCH_H */
