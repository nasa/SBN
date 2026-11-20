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
 * Auto-Generated stub implementations for functions defined in sbn_resource header
 */

#include "sbn_resource.h"
#include "utgenstub.h"

void UT_DefaultHandler_SBN_ForEachConfiguredNet(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_ForEachConfiguredPeer(void *, UT_EntryKey_t, const UT_StubContext_t *);

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_CheckNetIdSlotUsed()
 * ----------------------------------------------------
 */
bool SBN_CheckNetIdSlotUsed(CFE_ResourceId_t CheckId)
{
    UT_GenStub_SetupReturnBuffer(SBN_CheckNetIdSlotUsed, bool);

    UT_GenStub_AddParam(SBN_CheckNetIdSlotUsed, CFE_ResourceId_t, CheckId);

    UT_GenStub_Execute(SBN_CheckNetIdSlotUsed, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_CheckNetIdSlotUsed, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_CheckPeerIdSlotUsed()
 * ----------------------------------------------------
 */
bool SBN_CheckPeerIdSlotUsed(CFE_ResourceId_t CheckId)
{
    UT_GenStub_SetupReturnBuffer(SBN_CheckPeerIdSlotUsed, bool);

    UT_GenStub_AddParam(SBN_CheckPeerIdSlotUsed, CFE_ResourceId_t, CheckId);

    UT_GenStub_Execute(SBN_CheckPeerIdSlotUsed, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_CheckPeerIdSlotUsed, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_ForEachConfiguredNet()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_ForEachConfiguredNet(SBN_NetFunc_t Func, void *Arg)
{
    UT_GenStub_SetupReturnBuffer(SBN_ForEachConfiguredNet, SBN_Status_t);

    UT_GenStub_AddParam(SBN_ForEachConfiguredNet, SBN_NetFunc_t, Func);
    UT_GenStub_AddParam(SBN_ForEachConfiguredNet, void *, Arg);

    UT_GenStub_Execute(SBN_ForEachConfiguredNet, Basic, UT_DefaultHandler_SBN_ForEachConfiguredNet);

    return UT_GenStub_GetReturnValue(SBN_ForEachConfiguredNet, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_ForEachConfiguredPeer()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_ForEachConfiguredPeer(SBN_PeerFunc_t Func, void *Arg)
{
    UT_GenStub_SetupReturnBuffer(SBN_ForEachConfiguredPeer, SBN_Status_t);

    UT_GenStub_AddParam(SBN_ForEachConfiguredPeer, SBN_PeerFunc_t, Func);
    UT_GenStub_AddParam(SBN_ForEachConfiguredPeer, void *, Arg);

    UT_GenStub_Execute(SBN_ForEachConfiguredPeer, Basic, UT_DefaultHandler_SBN_ForEachConfiguredPeer);

    return UT_GenStub_GetReturnValue(SBN_ForEachConfiguredPeer, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_LocateNetRecordByExtIdx()
 * ----------------------------------------------------
 */
SBN_NetRecord_t *SBN_LocateNetRecordByExtIdx(SBN_NetIdx_t Idx)
{
    UT_GenStub_SetupReturnBuffer(SBN_LocateNetRecordByExtIdx, SBN_NetRecord_t *);

    UT_GenStub_AddParam(SBN_LocateNetRecordByExtIdx, SBN_NetIdx_t, Idx);

    UT_GenStub_Execute(SBN_LocateNetRecordByExtIdx, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_LocateNetRecordByExtIdx, SBN_NetRecord_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_LocateNetRecordByID()
 * ----------------------------------------------------
 */
SBN_NetRecord_t *SBN_LocateNetRecordByID(SBN_NetId_t NetId)
{
    UT_GenStub_SetupReturnBuffer(SBN_LocateNetRecordByID, SBN_NetRecord_t *);

    UT_GenStub_AddParam(SBN_LocateNetRecordByID, SBN_NetId_t, NetId);

    UT_GenStub_Execute(SBN_LocateNetRecordByID, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_LocateNetRecordByID, SBN_NetRecord_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_LocatePeerRecordByID()
 * ----------------------------------------------------
 */
SBN_PeerRecord_t *SBN_LocatePeerRecordByID(SBN_PeerId_t PeerId)
{
    UT_GenStub_SetupReturnBuffer(SBN_LocatePeerRecordByID, SBN_PeerRecord_t *);

    UT_GenStub_AddParam(SBN_LocatePeerRecordByID, SBN_PeerId_t, PeerId);

    UT_GenStub_Execute(SBN_LocatePeerRecordByID, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_LocatePeerRecordByID, SBN_PeerRecord_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_LocatePeerRecordInNet()
 * ----------------------------------------------------
 */
SBN_PeerRecord_t *SBN_LocatePeerRecordInNet(SBN_NetRecord_t *NetRec, SBN_PeerIdx_t Idx)
{
    UT_GenStub_SetupReturnBuffer(SBN_LocatePeerRecordInNet, SBN_PeerRecord_t *);

    UT_GenStub_AddParam(SBN_LocatePeerRecordInNet, SBN_NetRecord_t *, NetRec);
    UT_GenStub_AddParam(SBN_LocatePeerRecordInNet, SBN_PeerIdx_t, Idx);

    UT_GenStub_Execute(SBN_LocatePeerRecordInNet, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_LocatePeerRecordInNet, SBN_PeerRecord_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_NetId_AsInt()
 * ----------------------------------------------------
 */
int SBN_NetId_AsInt(SBN_NetId_t NetId)
{
    UT_GenStub_SetupReturnBuffer(SBN_NetId_AsInt, int);

    UT_GenStub_AddParam(SBN_NetId_AsInt, SBN_NetId_t, NetId);

    UT_GenStub_Execute(SBN_NetId_AsInt, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_NetId_AsInt, int);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_PeerId_AsInt()
 * ----------------------------------------------------
 */
int SBN_PeerId_AsInt(SBN_PeerId_t PeerId)
{
    UT_GenStub_SetupReturnBuffer(SBN_PeerId_AsInt, int);

    UT_GenStub_AddParam(SBN_PeerId_AsInt, SBN_PeerId_t, PeerId);

    UT_GenStub_Execute(SBN_PeerId_AsInt, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_PeerId_AsInt, int);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_RegisterNet()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_RegisterNet(SBN_NetId_t *NetIdOut)
{
    UT_GenStub_SetupReturnBuffer(SBN_RegisterNet, SBN_Status_t);

    UT_GenStub_AddParam(SBN_RegisterNet, SBN_NetId_t *, NetIdOut);

    UT_GenStub_Execute(SBN_RegisterNet, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_RegisterNet, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_RegisterPeer()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_RegisterPeer(SBN_PeerId_t *PeerIdOut)
{
    UT_GenStub_SetupReturnBuffer(SBN_RegisterPeer, SBN_Status_t);

    UT_GenStub_AddParam(SBN_RegisterPeer, SBN_PeerId_t *, PeerIdOut);

    UT_GenStub_Execute(SBN_RegisterPeer, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_RegisterPeer, SBN_Status_t);
}
