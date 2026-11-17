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
#include "sbn_filter.h"

/*
 * API for a gatekeeper filter function - this does not modify its content
 */
typedef SBN_Status_t (*SBN_Filter_GateFunc_t)(const CFE_MSG_Message_t *Msg, const SBN_Filter_Ctx_t *Context);

/*
 * API for a mutation filter function - this may modify its content
 */
typedef SBN_Status_t (*SBN_Filter_MutateFunc_t)(void *MsgBuf, SBN_Filter_Ctx_t *Context);

/*
 * A local extension of the Filter_Ctxt struct that includes
 * information for a mutation filter
 */
typedef struct SBN_Filter_MutationCtxt
{
    SBN_Filter_Ctx_t            CtxtBase;
    SBN_Filter_Direction_Enum_t Dir;
    CFE_MSG_Message_t          *Msg;
} SBN_Filter_MutationCtxt_t;

/*
 * A local extension of the Filter_Ctxt struct that includes
 * information for a gatekeeper filter
 */
typedef struct SBN_Filter_GatekeeperCtxt
{
    SBN_Filter_Ctx_t            CtxtBase;
    SBN_Filter_Direction_Enum_t Dir;
    const CFE_MSG_Message_t    *Msg;
} SBN_Filter_GatekeeperCtxt_t;

/*
 * A local extension of the Filter_Ctxt struct that includes
 * information for a remap filter
 */
typedef struct SBN_Filter_RemapCtxt
{
    SBN_Filter_Ctx_t CtxtBase;
    CFE_SB_MsgId_t  *MsgIdPtr;
} SBN_Filter_RemapCtxt_t;

/*
 * API for a mutation filter function - this may modify its content
 */
typedef SBN_Status_t (*SBN_Filter_ProcFunc_t)(const SBN_FilterInterface_t *FilterIntf, void *Ctxt);

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets the function to invoke based on direction
 *
 *-----------------------------------------------------------------*/
static SBN_Filter_MutateFunc_t SBN_Filter_GetMutation(const SBN_FilterInterface_t *FilterIntf,
                                                      SBN_Filter_Direction_Enum_t  Dir)
{
    SBN_Filter_MutateFunc_t Result;

    Result = NULL;
    if (FilterIntf != NULL)
    {
        switch (Dir)
        {
            case SBN_Filter_Direction_INBOUND:
                Result = FilterIntf->FilterRecv;
                break;
            case SBN_Filter_Direction_OUTBOUND:
                Result = FilterIntf->FilterSend;
                break;
            default:
                break;
        }
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets the function to invoke based on direction
 *
 *-----------------------------------------------------------------*/
static SBN_Filter_GateFunc_t SBN_Filter_GetGatekeeper(const SBN_FilterInterface_t *FilterIntf,
                                                      SBN_Filter_Direction_Enum_t  Dir)
{
    SBN_Filter_GateFunc_t Result;

    Result = NULL;
    if (FilterIntf != NULL)
    {
        switch (Dir)
        {
            case SBN_Filter_Direction_INBOUND:
                Result = FilterIntf->CheckInbound;
                break;
            case SBN_Filter_Direction_OUTBOUND:
                Result = FilterIntf->CheckOutbound;
                break;
            default:
                break;
        }
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets the function to invoke based on direction
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Filter_DoGatekeeper(const SBN_FilterInterface_t *FilterIntf, void *CtxtBase)
{
    SBN_Filter_GatekeeperCtxt_t *Ctxt = CtxtBase;
    SBN_Filter_GateFunc_t        Func;
    SBN_Status_t                 Status;

    Func = SBN_Filter_GetGatekeeper(FilterIntf, Ctxt->Dir);

    if (Func != NULL)
    {
        Status = Func(Ctxt->Msg, &Ctxt->CtxtBase);
    }
    else
    {
        Status = SBN_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets the function to invoke based on direction
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Filter_DoMutation(const SBN_FilterInterface_t *FilterIntf, void *CtxtBase)
{
    SBN_Filter_MutationCtxt_t *Ctxt = CtxtBase;
    SBN_Filter_MutateFunc_t    Func;
    SBN_Status_t               Status;

    Func = SBN_Filter_GetMutation(FilterIntf, Ctxt->Dir);

    if (Func != NULL)
    {
        Status = Func(Ctxt->Msg, &Ctxt->CtxtBase);
    }
    else
    {
        Status = SBN_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets the function to invoke based on direction
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Filter_DoRemap(const SBN_FilterInterface_t *FilterIntf, void *CtxtBase)
{
    SBN_Filter_RemapCtxt_t *Ctxt = CtxtBase;
    SBN_Status_t            Status;

    if (FilterIntf != NULL && FilterIntf->RemapMID != NULL)
    {
        Status = FilterIntf->RemapMID(Ctxt->MsgIdPtr, &Ctxt->CtxtBase);
    }
    else
    {
        Status = SBN_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets the function to invoke based on direction
 *
 *-----------------------------------------------------------------*/
static SBN_Status_t SBN_Filter_DoForAll(SBN_Filter_ProcFunc_t         ProcFunc,
                                        void                         *Ctxt,
                                        const SBN_FilterInterface_t **FilterIntfSet,
                                        SBN_ModuleIdx_t               NumFilters)
{
    SBN_ModuleIdx_t               FilterIdx;
    SBN_Status_t                  Status;
    const SBN_FilterInterface_t **CurrFilter;

    CurrFilter = FilterIntfSet;
    FilterIdx  = NumFilters;
    Status     = SBN_SUCCESS;

    while (FilterIdx > 0 && Status == SBN_SUCCESS)
    {
        --FilterIdx;

        Status = ProcFunc(*CurrFilter, Ctxt);

        ++CurrFilter;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets the function to invoke based on direction
 *
 *-----------------------------------------------------------------*/
static void SBN_Filter_SetupCtxt(SBN_Filter_Ctx_t *Ctxt, SBN_PeerInterface_t *Peer)
{
    Ctxt->MyProcessorID    = CFE_PSP_GetProcessorId();
    Ctxt->MySpacecraftID   = CFE_PSP_GetSpacecraftId();
    Ctxt->PeerProcessorID  = Peer->ProcessorID;
    Ctxt->PeerSpacecraftID = Peer->SpacecraftID;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Filter_Mutation(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer, SBN_Filter_Direction_Enum_t Dir)
{
    SBN_Filter_MutationCtxt_t Ctxt;

    memset(&Ctxt, 0, sizeof(Ctxt));
    SBN_Filter_SetupCtxt(&Ctxt.CtxtBase, Peer);
    Ctxt.Dir = Dir;
    Ctxt.Msg = BufD->DataPtr;

    return SBN_Filter_DoForAll(SBN_Filter_DoMutation, &Ctxt, Peer->Filters, Peer->FilterCnt);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t
SBN_Filter_Gatekeeper(const CFE_MSG_Message_t *MsgPtr, SBN_PeerInterface_t *Peer, SBN_Filter_Direction_Enum_t Dir)
{
    SBN_Filter_GatekeeperCtxt_t Ctxt;

    memset(&Ctxt, 0, sizeof(Ctxt));
    SBN_Filter_SetupCtxt(&Ctxt.CtxtBase, Peer);
    Ctxt.Dir = Dir;
    Ctxt.Msg = MsgPtr;

    return SBN_Filter_DoForAll(SBN_Filter_DoGatekeeper, &Ctxt, Peer->Filters, Peer->FilterCnt);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Filter_Remap(CFE_SB_MsgId_t *MsgIdPtr, SBN_PeerInterface_t *Peer)
{
    SBN_Filter_RemapCtxt_t Ctxt;

    memset(&Ctxt, 0, sizeof(Ctxt));
    SBN_Filter_SetupCtxt(&Ctxt.CtxtBase, Peer);
    Ctxt.MsgIdPtr = MsgIdPtr;

    return SBN_Filter_DoForAll(SBN_Filter_DoRemap, &Ctxt, Peer->Filters, Peer->FilterCnt);
}
