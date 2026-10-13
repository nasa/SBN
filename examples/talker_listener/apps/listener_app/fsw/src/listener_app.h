/**
 * @file
 *
 * Main header file for the Listener application
 */

#ifndef LISTENER_APP_H
#define LISTENER_APP_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_config.h"

#include "listener_app_mission_cfg.h"
#include "listener_app_platform_cfg.h"

#include "listener_app_perfids.h"
#include "listener_app_msgids.h"
#include "listener_app_msg.h"

/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    // Current counter value - CKC
    uint8 counterNum;

    // Gate for incrementing counter
    bool  counterGate;

    // Counter message data - CKC
    LISTENER_APP_Counter_t counterMsg;

    /*
    ** Housekeeping telemetry packet...
    */
    LISTENER_APP_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;

    CFE_TBL_Handle_t TblHandles[LISTENER_APP_NUMBER_OF_TABLES];
} LISTENER_APP_Data_t;

/*
** Global data structure
*/
extern LISTENER_APP_Data_t LISTENER_APP_Data;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (LISTENER_APP_Main), these
**       functions are not called from any other source module.
*/
void         LISTENER_APP_Main(void);
CFE_Status_t LISTENER_APP_Init(void);

#endif /* LISTENER_APP_H */
