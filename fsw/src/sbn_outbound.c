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

/*
 ** Include Files
 */
#include <string.h>

#include "sbn_error.h"
#include "sbn_app.h"
#include "sbn_outbound.h"
#include "sbn_filter.h"

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Outbound_GetMsg(SBN_BufferDesc_t *BufD, CFE_SB_PipeId_t PeerPipe, int32 Timeout)
{
    const CFE_SB_Buffer_t *SbBufPtr;
    CFE_MSG_Size_t         MsgSz;
    SBN_Status_t           SBN_Status;
    CFE_Status_t           CFE_Status;

    /* if peer data is not in use, go to next peer */
    SbBufPtr   = NULL;
    CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&SbBufPtr, PeerPipe, Timeout);

    if (CFE_Status == CFE_SB_TIME_OUT || CFE_Status == CFE_SB_NO_MESSAGE)
    {
        /* This is OK */
        SBN_Status = SBN_IF_EMPTY;
    }
    else if (CFE_Status != CFE_SUCCESS)
    {
        /* This is a problem */
        SBN_Status = SBN_ERROR;
    }
    else if (CFE_MSG_GetSize(&SbBufPtr->Msg, &MsgSz) != CFE_SUCCESS || MsgSz > BufD->MaxDataSz)
    {
        SBN_Status = SBN_ERROR;
    }
    else
    {
        /* Got a valid buffer and size */

        /* For now simply copy it so we can change/modify the message as needed without violating
         * the const-ness of the software bus buffer.  In the future this should be more integrated
         * with encoding/packing so its not an "extra" copy */
        memcpy(BufD->DataPtr, &SbBufPtr->Msg, MsgSz);
        BufD->ActualDataSz = MsgSz;

        SBN_Status = SBN_SUCCESS;
    }

    return SBN_Status;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Outbound_SendToPeer(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer)
{
    SBN_NetInterface_t *Net;
    SBN_Status_t        SBN_Status;

    Net = Peer->Net;

    SBN_Status = Net->IfOps->Send(Peer, BufD->MsgType, BufD->ActualDataSz, BufD->DataPtr);

    if (SBN_Status != SBN_SUCCESS)
    {
        Peer->Counts.SendErrCnt++;
    }
    else
    {
        Peer->Counts.SendCnt++;
    }

    /* for clients that need a poll or heartbeat, update time even when failing */
    OS_GetLocalTime(&Peer->LastSend);

    /* Reset the buffer because this message was sent */
    BufD->MsgType      = SBN_NO_MSG;
    BufD->ActualDataSz = 0;

    return SBN_Status;
}
