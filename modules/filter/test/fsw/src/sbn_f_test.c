#include "sbn_types.h"
#include "cfe.h"

SBN_Status_t SBN_F_Test_Out(void *msg)
{
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    OS_printf("msg out StreamId=%d\n", CCSDS_RD_SID(*PriHdrPtr));
    return SBN_SUCCESS;
}/* end SBN_F_Test_Out() */

SBN_Status_t SBN_F_Test_In(void *msg)
{
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    OS_printf("msg in StreamId=%d\n", CCSDS_RD_SID(*PriHdrPtr));
    return SBN_SUCCESS;
}/* end SBN_F_Test_In() */

CFE_Status_t SBN_F_Test_LibInit(void)
{
    OS_printf("SBN_F_Test Lib Initialized.");
    return CFE_SUCCESS;
}/* end SBN_F_Test_LibInit() */
