#include "sbn_types.h"
#include "cfe.h"

SBN_Status_t SBN_Filter_Out(void *msg)
{
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    OS_printf("msg out StreamId=%d\n", CCSDS_RD_SID(*PriHdrPtr));
    return SBN_SUCCESS;
}/* end SBN_Filter_Out() */

SBN_Status_t SBN_Filter_In(void *msg)
{
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    OS_printf("msg in StreamId=%d\n", CCSDS_RD_SID(*PriHdrPtr));
    return SBN_SUCCESS;
}/* end SBN_Filter_In() */

CFE_Status_t SBN_Filter_LibInit(void)
{
    OS_printf("SBN_Filter Lib Initialized.");
    return CFE_SUCCESS;
}/* end SBN_Filter_LibInit() */
