#include "sbn_types.h"
#include "cfe.h"

SBN_Status_t SBN_Filter_Test(void *msg)
{
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    OS_printf("msg StreamId=%d\n", CCSDS_RD_SID(*PriHdrPtr));
    return SBN_SUCCESS;
}/* end SBN_UDP_Init() */

CFE_Status_t SBN_Filter_LibInit(void)
{
    OS_printf("SBN_Filter Lib Initialized.");
    return CFE_SUCCESS;
}/* end SBN_UDP_LibInit() */
