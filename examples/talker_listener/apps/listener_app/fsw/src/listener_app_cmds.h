/**
 * @file
 *   This file contains the prototypes for the Listener App Ground Command-handling functions
 */

#ifndef LISTENER_APP_CMDS_H
#define LISTENER_APP_CMDS_H

/*
** Required header files.
*/
#include "cfe_error.h"
#include "listener_app_msg.h"
#include "talker_app_msg.h"

CFE_Status_t LISTENER_APP_SendHkCmd(const LISTENER_APP_SendHkCmd_t *Msg);
CFE_Status_t LISTENER_APP_ResetCountersCmd(const LISTENER_APP_ResetCountersCmd_t *Msg);
CFE_Status_t LISTENER_APP_ProcessCmd(const LISTENER_APP_ProcessCmd_t *Msg);
CFE_Status_t LISTENER_APP_NoopCmd(const LISTENER_APP_NoopCmd_t *Msg);
CFE_Status_t LISTENER_APP_DisplayParamCmd(const LISTENER_APP_DisplayParamCmd_t *Msg);
CFE_Status_t LISTENER_APP_Counter(const TALKER_APP_Counter_t *Msg);

#endif /* LISTENER_APP_CMDS_H */
