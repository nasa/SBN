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
#include "sbn_worker.h"
#include "sbn_error.h"

void SBN_UT_Worker_IncrementCountHandler(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    uint32 *Count = UserObj;

    ++(*Count);
    if (UT_GetStubCount(FuncKey) >= 10)
    {
        SBN_WorkQueue.ShouldExit = true;
    }
}

void Test_SBN_Worker_GetAssignment(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_GetAssignment(SBN_Worker_WorkItem_t *Assignment);
     */
    SBN_Worker_WorkItem_t Assignment;

    memset(&Assignment, 0xFF, sizeof(Assignment));

    ++SBN_WorkQueue.NumPosted;
    UtAssert_VOIDCALL(SBN_Worker_GetAssignment(&Assignment));

    ++SBN_WorkQueue.NumPosted;
    SBN_WorkQueue.ShouldRun = true;
    UtAssert_VOIDCALL(SBN_Worker_GetAssignment(&Assignment));

    UT_SetHandlerFunction(UT_KEY(OS_CondVarTimedWait),
                          SBN_UT_Worker_IncrementCountHandler,
                          (void *)&SBN_WorkQueue.NumPosted);
    UtAssert_VOIDCALL(SBN_Worker_GetAssignment(&Assignment));
    UtAssert_VOIDCALL(SBN_Worker_GetAssignment(&Assignment));
    UtAssert_VOIDCALL(SBN_Worker_GetAssignment(&Assignment));
}

static void UT_SBN_Worker_NetJobFunc(SBN_NetRecord_t *NetRec, SBN_BufferDesc_t *BufD, int32 MsgLim)
{
    UT_DEFAULT_IMPL(UT_SBN_Worker_NetJobFunc);
}

void Test_SBN_Worker_DispatchToNet(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_DispatchToNet(SBN_Worker_TaskCtxt_t *Ctxt, SBN_NetId_t NetId, SBN_Worker_NetJobFunc_t
     * HandlerFunc);
     */
    SBN_Worker_TaskCtxt_t Ctxt;
    SBN_NetId_t           NetId = SBN_AppData.NetRecords[0].NetId;

    UT_SetDefaultReturnValue(UT_KEY(SBN_LocateNetRecordByID), (UT_IntReturn_t)&SBN_AppData.NetRecords[0]);

    memset(&Ctxt, 0, sizeof(Ctxt));
    UtAssert_VOIDCALL(SBN_Worker_DispatchToNet(&Ctxt, NetId, UT_SBN_Worker_NetJobFunc));
    UtAssert_STUB_COUNT(UT_SBN_Worker_NetJobFunc, 1);

    UT_SetDeferredRetcode(UT_KEY(SBN_LocateNetRecordByID), 1, 0);
    UtAssert_VOIDCALL(SBN_Worker_DispatchToNet(&Ctxt, NetId, UT_SBN_Worker_NetJobFunc));
    UtAssert_STUB_COUNT(UT_SBN_Worker_NetJobFunc, 1);
}

static void UT_SBN_Worker_PeerJobFunc(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD, int32 MsgLim)
{
    UT_DEFAULT_IMPL(UT_SBN_Worker_PeerJobFunc);
}

void Test_SBN_Worker_DispatchToPeer(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_DispatchToPeer(SBN_Worker_TaskCtxt_t *Ctxt, SBN_PeerId_t PeerId, SBN_Worker_PeerJobFunc_t
     * HandlerFunc);
     */
    SBN_Worker_TaskCtxt_t Ctxt;
    SBN_PeerId_t          PeerId = SBN_AppData.PeerRecords[0].PeerId;

    memset(&Ctxt, 0, sizeof(Ctxt));

    UT_SetDefaultReturnValue(UT_KEY(SBN_LocatePeerRecordByID), (UT_IntReturn_t)&SBN_AppData.PeerRecords[0]);

    UtAssert_VOIDCALL(SBN_Worker_DispatchToPeer(&Ctxt, PeerId, UT_SBN_Worker_PeerJobFunc));
    UtAssert_STUB_COUNT(UT_SBN_Worker_PeerJobFunc, 1);

    UT_SetDeferredRetcode(UT_KEY(SBN_LocatePeerRecordByID), 1, 0);
    UtAssert_VOIDCALL(SBN_Worker_DispatchToPeer(&Ctxt, PeerId, UT_SBN_Worker_PeerJobFunc));
    UtAssert_STUB_COUNT(UT_SBN_Worker_PeerJobFunc, 1);
}

void Test_SBN_Worker_DispatchAssignment(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_DispatchAssignment(SBN_Worker_TaskCtxt_t *Ctxt, const SBN_Worker_WorkItem_t *Assignment);
     */
    SBN_Worker_TaskCtxt_t Ctxt;
    SBN_Worker_WorkItem_t Assignment;

    memset(&Ctxt, 0, sizeof(Ctxt));
    memset(&Assignment, 0, sizeof(Assignment));

    UtAssert_VOIDCALL(SBN_Worker_DispatchAssignment(&Ctxt, &Assignment));

    Assignment.JobType = SBN_Worker_JobType_PEER_OUTBOUND;
    UtAssert_VOIDCALL(SBN_Worker_DispatchAssignment(&Ctxt, &Assignment));

    Assignment.JobType = SBN_Worker_JobType_PEER_INBOUND;
    UtAssert_VOIDCALL(SBN_Worker_DispatchAssignment(&Ctxt, &Assignment));

    Assignment.JobType = SBN_Worker_JobType_NET_INBOUND;
    UtAssert_VOIDCALL(SBN_Worker_DispatchAssignment(&Ctxt, &Assignment));
}

void Test_SBN_Worker_EntryPoint(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_EntryPoint(void);
     */

    UT_SetHandlerFunction(UT_KEY(OS_CondVarTimedWait),
                          SBN_UT_Worker_IncrementCountHandler,
                          (void *)&SBN_WorkQueue.NumPosted);

    SBN_WorkQueue.ShouldRun = true;
    UtAssert_VOIDCALL(SBN_Worker_EntryPoint());

    UT_ResetState(UT_KEY(OS_CondVarTimedWait));
    UT_SetHandlerFunction(UT_KEY(OS_CondVarTimedWait),
                          SBN_UT_Worker_IncrementCountHandler,
                          (void *)&SBN_WorkQueue.NumPosted);
    SBN_WorkQueue.ShouldExit = false;
    SBN_WorkQueue.ShouldRun  = false;
    UtAssert_VOIDCALL(SBN_Worker_EntryPoint());
}

void Test_SBN_Worker_Setup(void)
{
    START();

    /* Test Case For:
     *  SBN_Status_t SBN_Worker_Setup(void);
     */
    UtAssert_VOIDCALL(SBN_Worker_Setup());

    SBN_WorkQueue.NumWorkers = 0;
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, -1);
    UtAssert_VOIDCALL(SBN_Worker_Setup());
}

void Test_SBN_Worker_Shutdown(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_Shutdown(void);
     */
    SBN_WorkQueue.NumWorkers = SBN_PLATFORM_NUM_WORKER_TASKS;
    UtAssert_VOIDCALL(SBN_Worker_Shutdown());

    SBN_WorkQueue.NumWorkers = SBN_PLATFORM_NUM_WORKER_TASKS;
    ++SBN_WorkQueue.NumPosted;
    UT_SetHandlerFunction(UT_KEY(OS_CondVarSignal),
                          SBN_UT_Worker_IncrementCountHandler,
                          (void *)&SBN_WorkQueue.NumClaimed);
    UT_SetHandlerFunction(UT_KEY(OS_CondVarWait),
                          SBN_UT_Worker_IncrementCountHandler,
                          (void *)&SBN_WorkQueue.NumCompleted);
}

void Test_SBN_Worker_Pause(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_Pause(void);
     */
    UtAssert_VOIDCALL(SBN_Worker_Pause());

    ++SBN_WorkQueue.NumPosted;
    ++SBN_WorkQueue.NumClaimed;
    UT_SetHandlerFunction(UT_KEY(OS_CondVarWait),
                          SBN_UT_Worker_IncrementCountHandler,
                          (void *)&SBN_WorkQueue.NumCompleted);
    UtAssert_VOIDCALL(SBN_Worker_Pause());
}

void Test_SBN_Worker_Resume(void)
{
    START();

    /* Test Case For:
     *  void SBN_Worker_Resume(void);
     */
    UtAssert_VOIDCALL(SBN_Worker_Resume());
}

void Test_SBN_Worker_ScheduleJob(void)
{
    START();

    /* Test Case For:
     *  bool SBN_Worker_ScheduleJob(uint32 *JobIdOut, const SBN_Worker_WorkItem_t *Job);
     */
    uint32                JobId;
    SBN_Worker_WorkItem_t Assignment;

    SBN_WorkQueue.ShouldRun = true;
    memset(&Assignment, 0, sizeof(Assignment));

    UtAssert_BOOL_TRUE(SBN_Worker_ScheduleJob(&JobId, &Assignment));
    UtAssert_UINT32_EQ(SBN_WorkQueue.NumPosted, SBN_WorkQueue.NumCompleted + 1);

    UtAssert_BOOL_TRUE(SBN_Worker_ScheduleJob(NULL, &Assignment));
    UtAssert_UINT32_EQ(SBN_WorkQueue.NumPosted, SBN_WorkQueue.NumCompleted + 2);

    SBN_WorkQueue.NumPosted += SBN_PLATFORM_WORKER_QUEUE_DEPTH - 2;
    UtAssert_BOOL_FALSE(SBN_Worker_ScheduleJob(&JobId, &Assignment));

    SBN_WorkQueue.NumClaimed   = SBN_WorkQueue.NumPosted;
    SBN_WorkQueue.NumCompleted = SBN_WorkQueue.NumPosted;
    SBN_WorkQueue.ShouldRun    = false;
    UtAssert_BOOL_FALSE(SBN_Worker_ScheduleJob(&JobId, &Assignment));
}

void Test_SBN_Worker_IsJobPending(void)
{
    START();

    /* Test Case For:
     *  bool SBN_Worker_IsJobPending(uint32 JobId);
     */
    uint32 JobId;

    JobId = -1;

    UtAssert_BOOL_FALSE(SBN_Worker_IsJobPending(JobId));
}

void UT_Setup(void)
{
    memset(&SBN_WorkQueue, 0, sizeof(SBN_WorkQueue));
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Worker_GetAssignment);
    ADD_TEST(SBN_Worker_DispatchToNet);
    ADD_TEST(SBN_Worker_DispatchToPeer);
    ADD_TEST(SBN_Worker_DispatchAssignment);
    ADD_TEST(SBN_Worker_EntryPoint);
    ADD_TEST(SBN_Worker_Setup);
    ADD_TEST(SBN_Worker_Shutdown);
    ADD_TEST(SBN_Worker_Pause);
    ADD_TEST(SBN_Worker_Resume);
    ADD_TEST(SBN_Worker_ScheduleJob);
    ADD_TEST(SBN_Worker_IsJobPending);
}
