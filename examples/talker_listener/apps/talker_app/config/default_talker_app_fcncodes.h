/**
 * @file
 *   Specification for the TALKER_APP command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef TALKER_APP_FCNCODES_H
#define TALKER_APP_FCNCODES_H

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Talker App command codes
*/
#define TALKER_APP_NOOP_CC           0
#define TALKER_APP_RESET_COUNTERS_CC 1
#define TALKER_APP_PROCESS_CC        2
#define TALKER_APP_DISPLAY_PARAM_CC  3

#endif
