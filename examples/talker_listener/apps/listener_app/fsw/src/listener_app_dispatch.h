/**
 * @file
 *
 * Main header file for the LISTENER application
 */

#ifndef LISTENER_APP_DISPATCH_H
#define LISTENER_APP_DISPATCH_H

/*
** Required header files.
*/
#include "cfe.h"
#include "listener_app_msg.h"

void LISTENER_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr);
void LISTENER_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr);
bool LISTENER_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

#endif /* LISTENER_APP_DISPATCH_H */
