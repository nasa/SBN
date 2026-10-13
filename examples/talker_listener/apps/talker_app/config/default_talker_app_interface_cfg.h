/**
 * @file
 *   TALKER_APP Application Public Definitions
 *
 * This provides default values for configurable items that affect
 * the interface(s) of this module.  This includes the CMD/TLM message
 * interface, tables definitions, and any other data products that
 * serve to exchange information with other entities.
 *
 * @note This file may be overridden/superceded by mission-provided defintions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef TALKER_APP_INTERFACE_CFG_H
#define TALKER_APP_INTERFACE_CFG_H

/**
 * \brief Length of string buffer in the Display Value command
 *
 * The Display Value command offers an example of how to use command
 * parameters of different types.  This macro controls the length
 * of the string parameter.
 */
#define TALKER_APP_STRING_VAL_LEN 10

#endif
