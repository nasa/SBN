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

#include "sbn_coveragetest_common.h"
#include "sbn_resource.h"
#include "sbn_error.h"
#include "cfe_resourceid.h"

void Test_SBN_NetId_AsInt(void)
{
    /* Test Case For:
     * int SBN_NetId_AsInt(SBN_NetId_t NetId);
     */
    SBN_NetId_t NetId = SBN_NETID_C(CFE_ResourceId_FromInteger(2));

    UtAssert_INT32_EQ(SBN_NetId_AsInt(NetId), 2);
    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_ToIndex), 1, -1);
    UtAssert_INT32_EQ(SBN_NetId_AsInt(NetId), -1);
}

void Test_SBN_PeerId_AsInt(void)
{
    /* Test Case For:
     * int SBN_PeerId_AsInt(SBN_PeerId_t PeerId);
     */
    SBN_PeerId_t PeerId = SBN_PEERID_C(CFE_ResourceId_FromInteger(3));

    UtAssert_INT32_EQ(SBN_PeerId_AsInt(PeerId), 3);
    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_ToIndex), 1, -1);
    UtAssert_INT32_EQ(SBN_PeerId_AsInt(PeerId), -1);
}

void Test_SBN_CheckNetIdSlotUsed(void)
{
    START();

    /* Test Case For:
     * bool SBN_CheckNetIdSlotUsed(CFE_ResourceId_t CheckId);
     */
    CFE_ResourceId_t CheckId = CFE_ResourceId_FromInteger(1);
    SBN_NetRecord_t *NetRec  = &SBN_AppData.NetRecords[1];

    UtAssert_BOOL_FALSE(SBN_CheckNetIdSlotUsed(CheckId));

    NetRec->NetId = SBN_NETID_C(CheckId);
    UtAssert_BOOL_TRUE(SBN_CheckNetIdSlotUsed(CheckId));

    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_ToIndex), 1, -1);
    UtAssert_BOOL_TRUE(SBN_CheckNetIdSlotUsed(CheckId));
}

void Test_SBN_CheckPeerIdSlotUsed(void)
{
    START();

    /* Test Case For:
     * bool SBN_CheckPeerIdSlotUsed(CFE_ResourceId_t CheckId);
     */
    CFE_ResourceId_t  CheckId = CFE_ResourceId_FromInteger(2);
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[2];

    UtAssert_BOOL_FALSE(SBN_CheckPeerIdSlotUsed(CheckId));

    PeerRec->PeerId = SBN_PEERID_C(CheckId);
    UtAssert_BOOL_TRUE(SBN_CheckPeerIdSlotUsed(CheckId));

    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_ToIndex), 1, -1);
    UtAssert_BOOL_TRUE(SBN_CheckPeerIdSlotUsed(CheckId));
}

void Test_SBN_LocatePeerRecordByID(void)
{
    START();

    /* Test Case For:
     * SBN_PeerRecord_t *SBN_LocatePeerRecordByID(SBN_PeerId_t PeerId);
     */
    SBN_PeerId_t      PeerId  = SBN_PEERID_C(CFE_ResourceId_FromInteger(1));
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[1];
    UtAssert_ADDRESS_EQ(SBN_LocatePeerRecordByID(PeerId), PeerRec);

    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_ToIndex), 1, -1);
    UtAssert_NULL(SBN_LocatePeerRecordByID(PeerId));
}

void Test_SBN_LocateNetRecordByID(void)
{
    START();

    /* Test Case For:
     * SBN_NetRecord_t *SBN_LocateNetRecordByID(SBN_NetId_t NetId);
     */
    SBN_NetId_t      NetId  = SBN_NETID_C(CFE_ResourceId_FromInteger(1));
    SBN_NetRecord_t *NetRec = &SBN_AppData.NetRecords[1];
    UtAssert_ADDRESS_EQ(SBN_LocateNetRecordByID(NetId), NetRec);

    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_ToIndex), 1, -1);
    UtAssert_NULL(SBN_LocateNetRecordByID(NetId));
}

void Test_SBN_RegisterPeer(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_RegisterPeer(SBN_PeerId_t *PeerIdOut);
     */
    SBN_PeerId_t PeerId;

    UtAssert_INT32_EQ(SBN_RegisterPeer(&PeerId), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_RegisterPeer(&PeerId), SBN_SUCCESS);

    /* This is so the allocation will find "used" entries */
    UT_ResetState(UT_KEY(CFE_ResourceId_FindNext));
    UtAssert_INT32_EQ(SBN_RegisterPeer(&PeerId), SBN_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_FindNext), 1, -1);
    UtAssert_INT32_EQ(SBN_RegisterPeer(&PeerId), SBN_ERROR);

    UT_SetDefaultReturnValue(UT_KEY(CFE_ResourceId_ToIndex), -1);
    UtAssert_INT32_EQ(SBN_RegisterPeer(&PeerId), SBN_ERROR);
}

void Test_SBN_RegisterNet(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_RegisterNet(SBN_NetId_t *NetIdOut);
     */
    SBN_NetId_t NetId;

    UtAssert_INT32_EQ(SBN_RegisterNet(&NetId), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_RegisterNet(&NetId), SBN_SUCCESS);

    /* This is so the allocation will find "used" entries */
    UT_ResetState(UT_KEY(CFE_ResourceId_FindNext));
    UtAssert_INT32_EQ(SBN_RegisterNet(&NetId), SBN_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(CFE_ResourceId_FindNext), 1, -1);
    UtAssert_INT32_EQ(SBN_RegisterNet(&NetId), SBN_ERROR);

    UT_SetDefaultReturnValue(UT_KEY(CFE_ResourceId_ToIndex), -1);
    UtAssert_INT32_EQ(SBN_RegisterNet(&NetId), SBN_ERROR);
}

void Test_SBN_LocateNetRecordByExtIdx(void)
{
    START();

    /* Test Case For:
     * SBN_NetRecord_t *SBN_LocateNetRecordByExtIdx(SBN_NetIdx_t Idx);
     */
    SBN_NetRecord_t *NetRec = &SBN_AppData.NetRecords[0];

    UtAssert_ADDRESS_EQ(SBN_LocateNetRecordByExtIdx(0), NetRec);
    UtAssert_NULL(SBN_LocateNetRecordByExtIdx(SBN_PLATFORM_MAX_NETS));

    /* Make the NetId different */
    NetRec->NetId = SBN_NETID_C(CFE_ResourceId_FromInteger(33));
    UtAssert_NULL(SBN_LocateNetRecordByExtIdx(0));
}

void Test_SBN_LocatePeerRecordInNet(void)
{
    /* Test Case For:
     * SBN_PeerRecord_t *SBN_LocatePeerRecordInNet(SBN_NetRecord_t *NetRec, SBN_PeerIdx_t Idx);
     */
    SBN_NetRecord_t  *NetRec  = &SBN_AppData.NetRecords[0];
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];

    UtAssert_ADDRESS_EQ(SBN_LocatePeerRecordInNet(NetRec, 0), PeerRec);
    UtAssert_NULL(SBN_LocatePeerRecordInNet(NetRec, SBN_MISSION_MAX_PEER_CNT));
    UtAssert_NULL(SBN_LocatePeerRecordInNet(NULL, 0));

    /* Make the PeerId different */
    PeerRec->PeerId = SBN_PEERID_C(CFE_ResourceId_FromInteger(34));
    UtAssert_NULL(SBN_LocatePeerRecordInNet(NetRec, 0));
}

static SBN_Status_t UT_SBN_PeerFunc(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    ++*((uint32 *)Arg);
    return UT_DEFAULT_IMPL(UT_SBN_PeerFunc);
}

void Test_SBN_ForEachConfiguredPeer(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_ForEachConfiguredPeer(SBN_PeerFunc_t Func, void *Arg);
     */
    uint32 *Count;

    Count = 0;

    UtAssert_INT32_EQ(SBN_ForEachConfiguredPeer(UT_SBN_PeerFunc, &Count), SBN_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(UT_SBN_PeerFunc), 1, SBN_FINISHED);
    UtAssert_INT32_EQ(SBN_ForEachConfiguredPeer(UT_SBN_PeerFunc, &Count), SBN_FINISHED);
}

/**
 * @brief Function to use with SBN_ForEachConfiguredNet()
 *
 * A function to be called for every net
 */
static SBN_Status_t UT_SBN_NetFunc(SBN_NetRecord_t *NetRec, void *Arg)
{
    ++*((uint32 *)Arg);
    return UT_DEFAULT_IMPL(UT_SBN_NetFunc);
}

void Test_SBN_ForEachConfiguredNet(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_ForEachConfiguredNet(SBN_NetFunc_t Func, void *Arg);
     */
    uint32 *Count;

    Count = 0;

    UtAssert_INT32_EQ(SBN_ForEachConfiguredNet(UT_SBN_NetFunc, &Count), SBN_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(UT_SBN_NetFunc), 1, SBN_FINISHED);
    UtAssert_INT32_EQ(SBN_ForEachConfiguredNet(UT_SBN_NetFunc, &Count), SBN_FINISHED);
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_NetId_AsInt);
    ADD_TEST(SBN_PeerId_AsInt);
    ADD_TEST(SBN_CheckNetIdSlotUsed);
    ADD_TEST(SBN_CheckPeerIdSlotUsed);
    ADD_TEST(SBN_LocatePeerRecordByID);
    ADD_TEST(SBN_LocateNetRecordByID);
    ADD_TEST(SBN_RegisterPeer);
    ADD_TEST(SBN_RegisterNet);
    ADD_TEST(SBN_LocateNetRecordByExtIdx);
    ADD_TEST(SBN_LocatePeerRecordInNet);
    ADD_TEST(SBN_ForEachConfiguredPeer);
    ADD_TEST(SBN_ForEachConfiguredNet);
}
