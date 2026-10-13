/**
 * @file
 *   Specification for the TALKER_APP command and telemetry
 *   message data types.
 *
 * @note
 *   Constants and enumerated types related to these message structures
 *   are defined in talker_app_msgdefs.h.
 */
#ifndef TALKER_APP_MSGSTRUCT_H
#define TALKER_APP_MSGSTRUCT_H

/************************************************************************
 * Includes
 ************************************************************************/

#include "talker_app_mission_cfg.h"
#include "talker_app_msgdefs.h"
#include "cfe_msg_hdr.h"

/*************************************************************************/

/*
** The following commands all share the "NoArgs" format
**
** They are each given their own type name matching the command name, which
** allows them to change independently in the future without changing the prototype
** of the handler function
*/
typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TALKER_APP_NoopCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TALKER_APP_ResetCountersCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TALKER_APP_ProcessCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t           CommandHeader; /**< \brief Command header */
    TALKER_APP_DisplayParam_Payload_t Payload;
} TALKER_APP_DisplayParamCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t           CommandHeader; /**< \brief Command header */
    TALKER_APP_Counter_Payload Payload;
} TALKER_APP_Counter_t;

/*************************************************************************/
/*
** Type definition (Talker App housekeeping)
*/

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TALKER_APP_SendHkCmd_t;

typedef struct
{
    CFE_MSG_TelemetryHeader_t  TelemetryHeader; /**< \brief Telemetry header */
    TALKER_APP_HkTlm_Payload_t Payload;         /**< \brief Telemetry payload */
} TALKER_APP_HkTlm_t;

#endif /* TALKER_APP_MSGSTRUCT_H */
