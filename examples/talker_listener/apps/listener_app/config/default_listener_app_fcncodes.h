/**
 * @file
 *   Specification for the LISTENER_APP command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef LISTENER_APP_FCNCODES_H
#define LISTENER_APP_FCNCODES_H

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Listener App command codes
*/
#define LISTENER_APP_NOOP_CC           0
#define LISTENER_APP_RESET_COUNTERS_CC 1
#define LISTENER_APP_PROCESS_CC        2
#define LISTENER_APP_DISPLAY_PARAM_CC  3

#endif
