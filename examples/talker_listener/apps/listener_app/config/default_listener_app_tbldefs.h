/**
 * @file
 *   Specification for the LISTENER_APP table related
 *   constant definitions.
 *
 * @note
 *   These Macro definitions have been put in this file (instead of
 *   listener_app_tbl.h). DO NOT PUT ANY TYPEDEFS OR
 *   STRUCTURE DEFINITIONS IN THIS FILE!
 *   ADD THEM TO listener_app_tbl.h IF NEEDED!
 */
#ifndef LISTENER_APP_TBLDEFS_H
#define LISTENER_APP_TBLDEFS_H

#include "common_types.h"
#include "listener_app_mission_cfg.h"

/*
** Example Table structure
*/
typedef struct
{
    uint16 Int1;
    uint16 Int2;
} LISTENER_APP_ExampleTable_t;

#endif
