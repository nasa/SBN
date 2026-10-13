/**
 * @file
 *   Specification for the TALKER_APP command and telemetry
 *   message constant definitions.
 *
 *  For TALKER_APP this is only the function/command code definitions
 */
#ifndef TALKER_APP_MSGDEFS_H
#define TALKER_APP_MSGDEFS_H

#include "common_types.h"
#include "talker_app_fcncodes.h"

typedef struct TALKER_APP_DisplayParam_Payload
{
    uint32 ValU32;                            /**< 32 bit unsigned integer value */
    int16  ValI16;                            /**< 16 bit signed integer value */
    char   ValStr[TALKER_APP_STRING_VAL_LEN]; /**< An example string */
} TALKER_APP_DisplayParam_Payload_t;

/*************************************************************************/
/*
** Type definition (Talker App housekeeping)
*/

typedef struct TALKER_APP_HkTlm_Payload
{
    uint8 CommandErrorCounter;
    uint8 CommandCounter;
    uint8 spare[2];
} TALKER_APP_HkTlm_Payload_t;

typedef struct TALKER_APP_Counter_Payload
{
    uint8 counter;
} TALKER_APP_Counter_Payload;

#endif
