#include "sbn_interfaces.h"
#include "cfe.h"

static SBN_Status_t In(void *msg, SBN_Filter_Ctx_t *Context)
{
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    OS_printf("msg in StreamId=%d\n", CCSDS_RD_SID(*PriHdrPtr));
    return SBN_SUCCESS;
}/* end In() */

static SBN_Status_t Out(void *msg, SBN_Filter_Ctx_t *Context)
{
    CCSDS_PriHdr_t *PriHdrPtr = msg;
    OS_printf("msg out StreamId=%d\n", CCSDS_RD_SID(*PriHdrPtr));
    return SBN_SUCCESS;
}/* end Out() */

CFE_Status_t SBN_F_Test_Init(void)
{
    OS_printf("SBN_F_Test Lib Initialized.");
    return CFE_SUCCESS;
}/* end Init() */

SBN_FilterInterface_t SBN_F_Test =
{
    In, Out, NULL
};
