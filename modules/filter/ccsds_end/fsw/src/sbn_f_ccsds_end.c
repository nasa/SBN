#include "sbn_types.h"
#include "cfe.h"

SBN_Status_t SBN_F_CCSDS_End(void *Msg)
{
    int CCSDSType = CCSDS_RD_TYPE(*((CCSDS_PriHdr_t *)Msg));
    if(CCSDSType == CCSDS_TLM)
    {
        CCSDS_TlmPkt_t *TlmPktPtr = (CCSDS_TlmPkt_t *)Msg;

        uint32 Seconds = CCSDS_RD_SEC_HDR_SEC(TlmPktPtr->Sec);
        Seconds = CFE_MAKE_BIG32(Seconds);
        CCSDS_WR_SEC_HDR_SEC(TlmPktPtr->Sec, Seconds);

        /* SBN sends CCSDS telemetry messages with secondary headers in
         * big-endian order.
         */
        if(CCSDS_TIME_SIZE == 6)
        {
            uint16 SubSeconds = CCSDS_RD_SEC_HDR_SUBSEC(TlmPktPtr->Sec);
            SubSeconds = CFE_MAKE_BIG16(SubSeconds);
            CCSDS_WR_SEC_HDR_SUBSEC(TlmPktPtr->Sec, SubSeconds);
        }
        else
        {
            uint32 SubSeconds = CCSDS_RD_SEC_HDR_SUBSEC(TlmPktPtr->Sec);
            SubSeconds = CFE_MAKE_BIG32(SubSeconds);
            CCSDS_WR_SEC_HDR_SUBSEC(TlmPktPtr->Sec, SubSeconds);
        }/* end if */
    }/* end if */

    return SBN_SUCCESS;
}/* SBN_F_CCSDS_End() */

CFE_Status_t SBN_F_CCSDS_End_LibInit(void)
{
    OS_printf("SBN_F_CCSDS_End Lib Initialized.");
    return CFE_SUCCESS;
}/* end SBN_F_CCSDS_End_LibInit() */
