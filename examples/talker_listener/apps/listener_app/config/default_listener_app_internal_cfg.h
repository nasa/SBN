/**
 * @file
 *   LISTENER_APP Application Private Config Definitions
 *
 * This provides default values for configurable items that are internal
 * to this module and do NOT affect the interface(s) of this module.  Changes
 * to items in this file only affect the local module and will be transparent
 * to external entities that are using the public interface(s).
 *
 * @note This file may be overridden/superceded by mission-provided defintions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef LISTENER_APP_INTERNAL_CFG_H
#define LISTENER_APP_INTERNAL_CFG_H

/***********************************************************************/
#define LISTENER_APP_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define LISTENER_APP_NUMBER_OF_TABLES 1 /* Number of Example Table(s) */

#define LISTENER_APP_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define LISTENER_APP_TBL_ELEMENT_1_MAX 10

#endif
