/**
 * @file
 *
 * TALKER_APP Application Platform Configuration Header File
 *
 * This is a compatibility header for the "platform_cfg.h" file that has
 * traditionally provided both public and private config definitions
 * for each CFS app.
 *
 * These definitions are now provided in two separate files, one for
 * the public/mission scope and one for internal scope.
 *
 * @note This file may be overridden/superceded by mission-provided defintions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef TALKER_APP_PLATFORM_CFG_H
#define TALKER_APP_PLATFORM_CFG_H

#include "talker_app_mission_cfg.h"
#include "talker_app_internal_cfg.h"

#endif
