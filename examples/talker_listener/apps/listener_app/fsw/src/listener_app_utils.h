/**
 * @file
 *   This file contains the prototypes for the Listener App utility functions
 */

#ifndef LISTENER_APP_UTILS_H
#define LISTENER_APP_UTILS_H

/*
** Required header files.
*/
#include "listener_app.h"

CFE_Status_t LISTENER_APP_TblValidationFunc(void *TblData);
void         LISTENER_APP_GetCrc(const char *TableName);

#endif /* LISTENER_APP_UTILS_H */
