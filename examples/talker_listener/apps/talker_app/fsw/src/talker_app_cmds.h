/**
 * @file
 *   This file contains the prototypes for the Talker App Ground Command-handling functions
 */

#ifndef TALKER_APP_CMDS_H
#define TALKER_APP_CMDS_H

/*
** Required header files.
*/
#include "cfe_error.h"
#include "talker_app_msg.h"
#include "listener_app_msg.h"

CFE_Status_t TALKER_APP_SendHkCmd(const TALKER_APP_SendHkCmd_t *Msg);
CFE_Status_t TALKER_APP_ResetCountersCmd(const TALKER_APP_ResetCountersCmd_t *Msg);
CFE_Status_t TALKER_APP_ProcessCmd(const TALKER_APP_ProcessCmd_t *Msg);
CFE_Status_t TALKER_APP_NoopCmd(const TALKER_APP_NoopCmd_t *Msg);
CFE_Status_t TALKER_APP_DisplayParamCmd(const TALKER_APP_DisplayParamCmd_t *Msg);
CFE_Status_t TALKER_APP_Send_Counter(void);
CFE_Status_t TALKER_APP_Listens(const LISTENER_APP_Counter_t *Msg);

#endif /* TALKER_APP_CMDS_H */
