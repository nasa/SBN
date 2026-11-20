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
#include "sbn_filter.h"
#include "sbn_error.h"

SBN_Status_t SBN_UT_Gatekeeper_Stub(const CFE_MSG_Message_t *Msg, const SBN_Filter_Ctx_t *Context)
{
    return UT_DEFAULT_IMPL(SBN_UT_Gatekeeper_Stub);
}

void Test_SBN_Filter_Gatekeeper(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_Filter_Gatekeeper(const CFE_MSG_Message_t *MsgPtr, SBN_PeerInterface_t *Peer,
     * SBN_Filter_Direction_Enum_t Dir);
     */

    CFE_MSG_Message_t    Msg;
    SBN_PeerInterface_t *Peer = &SBN_AppData.PeerRecords[0].Intf;

    memset(&Msg, 0, sizeof(Msg));

    Peer->FilterCnt  = 2;
    Peer->Filters[0] = NULL;
    Peer->Filters[1] = FilterInterfacePtr;

    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_INVALID), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_OUTBOUND), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_INBOUND), SBN_SUCCESS);

    FilterInterfacePtr->CheckOutbound = SBN_UT_Gatekeeper_Stub;
    FilterInterfacePtr->CheckInbound  = SBN_UT_Gatekeeper_Stub;

    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_INVALID), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_UT_Gatekeeper_Stub, 0);

    UT_SetDeferredRetcode(UT_KEY(SBN_UT_Gatekeeper_Stub), 1, SBN_IF_EMPTY);
    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_OUTBOUND), SBN_IF_EMPTY);
    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_OUTBOUND), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_UT_Gatekeeper_Stub, 2);

    UT_SetDeferredRetcode(UT_KEY(SBN_UT_Gatekeeper_Stub), 2, SBN_ERROR);
    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_INBOUND), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_Filter_Gatekeeper(&Msg, Peer, SBN_Filter_Direction_INBOUND), SBN_ERROR);
    UtAssert_STUB_COUNT(SBN_UT_Gatekeeper_Stub, 4);
}

SBN_Status_t SBN_UT_Mutation_Stub(void *MsgBuf, SBN_Filter_Ctx_t *Context)
{
    return UT_DEFAULT_IMPL(SBN_UT_Mutation_Stub);
}

void Test_SBN_Filter_Mutation(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_Filter_Mutation(SBN_BufferDesc_t *BufD, SBN_PeerInterface_t *Peer, SBN_Filter_Direction_Enum_t
     * Dir);
     */
    SBN_BufferDesc_t     BufD;
    CFE_MSG_Message_t    Msg;
    SBN_PeerInterface_t *Peer = &SBN_AppData.PeerRecords[0].Intf;

    memset(&BufD, 0, sizeof(BufD));
    memset(&Msg, 0, sizeof(Msg));

    Peer->FilterCnt  = 2;
    Peer->Filters[0] = NULL;
    Peer->Filters[1] = FilterInterfacePtr;

    BufD.ActualDataSz = sizeof(Msg);
    BufD.MaxDataSz    = sizeof(Msg);
    BufD.DataPtr      = &Msg;
    BufD.MsgType      = SBN_APP_MSG;

    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_INVALID), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_INBOUND), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_OUTBOUND), SBN_SUCCESS);

    FilterInterfacePtr->FilterRecv = SBN_UT_Mutation_Stub;
    FilterInterfacePtr->FilterSend = SBN_UT_Mutation_Stub;

    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_INVALID), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_UT_Mutation_Stub, 0);

    UT_SetDeferredRetcode(UT_KEY(SBN_UT_Mutation_Stub), 1, SBN_IF_EMPTY);
    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_OUTBOUND), SBN_IF_EMPTY);
    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_OUTBOUND), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_UT_Mutation_Stub, 2);

    UT_SetDeferredRetcode(UT_KEY(SBN_UT_Mutation_Stub), 2, SBN_ERROR);
    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_INBOUND), SBN_SUCCESS);
    UtAssert_INT32_EQ(SBN_Filter_Mutation(&BufD, Peer, SBN_Filter_Direction_INBOUND), SBN_ERROR);
    UtAssert_STUB_COUNT(SBN_UT_Mutation_Stub, 4);
}

SBN_Status_t SBN_UT_RemapMID_Stub(CFE_SB_MsgId_t *FromToMidPtr, SBN_Filter_Ctx_t *Context)
{
    if (FromToMidPtr)
    {
        UT_Stub_CopyToLocal(UT_KEY(SBN_UT_RemapMID_Stub), FromToMidPtr, sizeof(*FromToMidPtr));
    }
    return UT_DEFAULT_IMPL(SBN_UT_RemapMID_Stub);
}

void Test_SBN_Filter_Remap(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_Filter_Remap(CFE_SB_MsgId_t *MsgIdPtr, SBN_PeerInterface_t *Peer);
     */
    CFE_SB_MsgId_t       MsgId;
    CFE_SB_MsgId_t       MsgIdMod;
    SBN_PeerInterface_t *Peer = &SBN_AppData.PeerRecords[0].Intf;

    MsgId    = CFE_SB_ValueToMsgId(0xAAAA);
    MsgIdMod = CFE_SB_ValueToMsgId(0xBBBB);

    Peer->FilterCnt  = 2;
    Peer->Filters[0] = FilterInterfacePtr;
    Peer->Filters[1] = NULL;

    UtAssert_INT32_EQ(SBN_Filter_Remap(&MsgId, Peer), SBN_SUCCESS);

    FilterInterfacePtr->RemapMID = SBN_UT_RemapMID_Stub;

    UT_SetDataBuffer(UT_KEY(SBN_UT_RemapMID_Stub), &MsgIdMod, sizeof(MsgIdMod), false);
    UtAssert_INT32_EQ(SBN_Filter_Remap(&MsgId, Peer), SBN_SUCCESS);
    UtAssert_UINT32_EQ(CFE_SB_MsgIdToValue(MsgId), CFE_SB_MsgIdToValue(MsgIdMod));
    UtAssert_STUB_COUNT(SBN_UT_RemapMID_Stub, 1);

    UT_SetDeferredRetcode(UT_KEY(SBN_UT_RemapMID_Stub), 1, SBN_ERROR);
    UtAssert_INT32_EQ(SBN_Filter_Remap(&MsgId, Peer), SBN_ERROR);
    UtAssert_STUB_COUNT(SBN_UT_RemapMID_Stub, 2);
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Filter_Gatekeeper);
    ADD_TEST(SBN_Filter_Mutation);
    ADD_TEST(SBN_Filter_Remap);
}
