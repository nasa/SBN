/**
 * @file
 *   Specification for the LISTENER_APP command and telemetry
 *   message constant definitions.
 *
 *  For LISTENER_APP this is only the function/command code definitions
 */
#ifndef LISTENER_APP_MSGDEFS_H
#define LISTENER_APP_MSGDEFS_H

#include "common_types.h"
#include "listener_app_fcncodes.h"

typedef struct LISTENER_APP_DisplayParam_Payload
{
    uint32 ValU32;                            /**< 32 bit unsigned integer value */
    int16  ValI16;                            /**< 16 bit signed integer value */
    char   ValStr[LISTENER_APP_STRING_VAL_LEN]; /**< An example string */
} LISTENER_APP_DisplayParam_Payload_t;

/*************************************************************************/
/*
** Type definition (Listener App housekeeping)
*/

typedef struct LISTENER_APP_HkTlm_Payload
{
    uint8 CommandErrorCounter;
    uint8 CommandCounter;
    uint8 spare[2];
} LISTENER_APP_HkTlm_Payload_t;

typedef struct LISTENER_APP_Counter_Payload
{
    uint8 counterRelay;
} LISTENER_APP_Counter_Payload;

#endif
