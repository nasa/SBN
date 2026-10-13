/**
 * @file
 *   This file contains the prototypes for the Talker App utility functions
 */

#ifndef TALKER_APP_UTILS_H
#define TALKER_APP_UTILS_H

/*
** Required header files.
*/
#include "talker_app.h"

CFE_Status_t TALKER_APP_TblValidationFunc(void *TblData);
void         TALKER_APP_GetCrc(const char *TableName);

#endif /* TALKER_APP_UTILS_H */
