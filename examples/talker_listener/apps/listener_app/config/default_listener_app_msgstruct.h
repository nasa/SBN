/**
 * @file
 *   Specification for the LISTENER_APP command and telemetry
 *   message data types.
 *
 * @note
 *   Constants and enumerated types related to these message structures
 *   are defined in listener_app_msgdefs.h.
 */
#ifndef LISTENER_APP_MSGSTRUCT_H
#define LISTENER_APP_MSGSTRUCT_H

/************************************************************************
 * Includes
 ************************************************************************/

#include "listener_app_mission_cfg.h"
#include "listener_app_msgdefs.h"
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
} LISTENER_APP_NoopCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} LISTENER_APP_ResetCountersCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} LISTENER_APP_ProcessCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t           CommandHeader; /**< \brief Command header */
    LISTENER_APP_DisplayParam_Payload_t Payload;
} LISTENER_APP_DisplayParamCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t           CommandHeader; /**< \brief Command header */
    LISTENER_APP_Counter_Payload Payload;
} LISTENER_APP_Counter_t;

/*************************************************************************/
/*
** Type definition (Listener App housekeeping)
*/

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} LISTENER_APP_SendHkCmd_t;

typedef struct
{
    CFE_MSG_TelemetryHeader_t  TelemetryHeader; /**< \brief Telemetry header */
    LISTENER_APP_HkTlm_Payload_t Payload;         /**< \brief Telemetry payload */
} LISTENER_APP_HkTlm_t;

#endif /* LISTENER_APP_MSGSTRUCT_H */
