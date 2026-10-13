/**
 * \file
 *   This file contains the source code for the Listener App utility functions
 */

/*
** Include Files:
*/
#include "listener_app.h"
#include "listener_app_eventids.h"
#include "listener_app_tbl.h"
#include "listener_app_utils.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Verify contents of First Example Table buffer contents                  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t LISTENER_APP_TblValidationFunc(void *TblData)
{
    CFE_Status_t               ReturnCode = CFE_SUCCESS;
    LISTENER_APP_ExampleTable_t *TblDataPtr = (LISTENER_APP_ExampleTable_t *)TblData;

    /*
    ** Listener Example Table Validation
    */
    if (TblDataPtr->Int1 > LISTENER_APP_TBL_ELEMENT_1_MAX)
    {
        /* First element is out of range, return an appropriate error code */
        ReturnCode = LISTENER_APP_TABLE_OUT_OF_RANGE_ERR_CODE;
    }

    return ReturnCode;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Output CRC                                                      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void LISTENER_APP_GetCrc(const char *TableName)
{
    CFE_Status_t   status;
    uint32         Crc;
    CFE_TBL_Info_t TblInfoPtr;

    status = CFE_TBL_GetInfo(&TblInfoPtr, TableName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Listener App: Error Getting Example Table Info");
    }
    else
    {
        Crc = TblInfoPtr.Crc;
        CFE_ES_WriteToSysLog("Listener App: CRC: 0x%08lX\n\n", (unsigned long)Crc);
    }
}
