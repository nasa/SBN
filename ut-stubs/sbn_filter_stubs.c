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

/**
 * @file
 *
 * Auto-Generated stub implementations for functions defined in sbn_filter header
 */

#include "sbn_filter.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Filter_Gatekeeper()
 * ----------------------------------------------------
 */
SBN_Status_t
SBN_Filter_Gatekeeper(const CFE_MSG_Message_t *MsgPtr, SBN_PeerInterface_t *Peer, SBN_Filter_Direction_Enum_t Dir)
{
    UT_GenStub_SetupReturnBuffer(SBN_Filter_Gatekeeper, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Filter_Gatekeeper, const CFE_MSG_Message_t *, MsgPtr);
    UT_GenStub_AddParam(SBN_Filter_Gatekeeper, SBN_PeerInterface_t *, Peer);
    UT_GenStub_AddParam(SBN_Filter_Gatekeeper, SBN_Filter_Direction_Enum_t, Dir);

    UT_GenStub_Execute(SBN_Filter_Gatekeeper, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Filter_Gatekeeper, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Filter_Mutation()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Filter_Mutation(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer, SBN_Filter_Direction_Enum_t Dir)
{
    UT_GenStub_SetupReturnBuffer(SBN_Filter_Mutation, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Filter_Mutation, SBN_BufferDesc_t *, BufD);
    UT_GenStub_AddParam(SBN_Filter_Mutation, SBN_PeerInterface_t *, Peer);
    UT_GenStub_AddParam(SBN_Filter_Mutation, SBN_Filter_Direction_Enum_t, Dir);

    UT_GenStub_Execute(SBN_Filter_Mutation, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Filter_Mutation, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Filter_Remap()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Filter_Remap(CFE_SB_MsgId_t *MsgIdPtr, SBN_PeerInterface_t *Peer)
{
    UT_GenStub_SetupReturnBuffer(SBN_Filter_Remap, SBN_Status_t);

    UT_GenStub_AddParam(SBN_Filter_Remap, CFE_SB_MsgId_t *, MsgIdPtr);
    UT_GenStub_AddParam(SBN_Filter_Remap, SBN_PeerInterface_t *, Peer);

    UT_GenStub_Execute(SBN_Filter_Remap, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Filter_Remap, SBN_Status_t);
}
