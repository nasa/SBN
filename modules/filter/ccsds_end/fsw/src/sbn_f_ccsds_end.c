/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include "sbn_interfaces.h"
#include "sbn_f_ccsds_end_events.h"
#include "cfe.h"

SBN_EventID_t SBN_F_CCSDS_END_FIRST_EID;

static SBN_Status_t End(void *Msg, SBN_Filter_Ctx_t *Context)
{
    int CCSDSType = CCSDS_RD_TYPE(*((CCSDS_PriHdr_t *)Msg));
    if (CCSDSType == CCSDS_TLM)
    {
        CCSDS_TelemetryPacket_t *TlmPktPtr = (CCSDS_TelemetryPacket_t *)Msg;

        uint32 Seconds = CCSDS_RD_SEC_HDR_SEC(TlmPktPtr->Sec);
        Seconds        = CFE_MAKE_BIG32(Seconds);
        CCSDS_WR_SEC_HDR_SEC(TlmPktPtr->Sec, Seconds);

        /* SBN sends CCSDS telemetry messages with secondary headers in
         * big-endian order.
         */
        if (CCSDS_TIME_SIZE == 6)
        {
            uint16 SubSeconds = CCSDS_RD_SEC_HDR_SUBSEC(TlmPktPtr->Sec);
            SubSeconds        = CFE_MAKE_BIG16(SubSeconds);
            CCSDS_WR_SEC_HDR_SUBSEC(TlmPktPtr->Sec, SubSeconds);
        }
        else
        {
            uint32 SubSeconds = CCSDS_RD_SEC_HDR_SUBSEC(TlmPktPtr->Sec);
            SubSeconds        = CFE_MAKE_BIG32(SubSeconds);
            CCSDS_WR_SEC_HDR_SUBSEC(TlmPktPtr->Sec, SubSeconds);
        } /* end if */
    } /* end if */

    return SBN_SUCCESS;
} /* SBN_F_CCSDS_End() */

static CFE_Status_t Init(int Version, SBN_EventID_t BaseEID)
{
    SBN_F_CCSDS_END_FIRST_EID = BaseEID;

    if (Version != 1) /* TODO: define */
    {
        OS_printf("SBN_F_CCSDS_End version mismatch: expected %d, got %d\n", 1, Version);
        return CFE_ES_RunStatus_APP_ERROR;
    } /* end if */

    OS_printf("SBN_F_CCSDS_End Lib Initialized.\n");
    return CFE_SUCCESS;
} /* end Init() */

SBN_FilterInterface_t SBN_F_CCSDS_End = { Init, End, End, NULL };
