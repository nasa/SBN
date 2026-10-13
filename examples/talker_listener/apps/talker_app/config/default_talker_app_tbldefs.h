/**
 * @file
 *   Specification for the TALKER_APP table related
 *   constant definitions.
 *
 * @note
 *   These Macro definitions have been put in this file (instead of
 *   talker_app_tbl.h). DO NOT PUT ANY TYPEDEFS OR
 *   STRUCTURE DEFINITIONS IN THIS FILE!
 *   ADD THEM TO talker_app_tbl.h IF NEEDED!
 */
#ifndef TALKER_APP_TBLDEFS_H
#define TALKER_APP_TBLDEFS_H

#include "common_types.h"
#include "talker_app_mission_cfg.h"

/*
** Example Table structure
*/
typedef struct
{
    uint16 Int1;
    uint16 Int2;
} TALKER_APP_ExampleTable_t;

#endif
