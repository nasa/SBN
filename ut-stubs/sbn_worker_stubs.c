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
 * Auto-Generated stub implementations for functions defined in sbn_worker header
 */

#include "sbn_worker.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_DispatchAssignment()
 * ----------------------------------------------------
 */
void SBN_Worker_DispatchAssignment(SBN_Worker_TaskCtxt_t *Ctxt, const SBN_Worker_WorkItem_t *Assignment)
{
    UT_GenStub_AddParam(SBN_Worker_DispatchAssignment, SBN_Worker_TaskCtxt_t *, Ctxt);
    UT_GenStub_AddParam(SBN_Worker_DispatchAssignment, const SBN_Worker_WorkItem_t *, Assignment);

    UT_GenStub_Execute(SBN_Worker_DispatchAssignment, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_DispatchToNet()
 * ----------------------------------------------------
 */
void SBN_Worker_DispatchToNet(SBN_Worker_TaskCtxt_t *Ctxt, SBN_NetId_t NetId, SBN_Worker_NetJobFunc_t HandlerFunc)
{
    UT_GenStub_AddParam(SBN_Worker_DispatchToNet, SBN_Worker_TaskCtxt_t *, Ctxt);
    UT_GenStub_AddParam(SBN_Worker_DispatchToNet, SBN_NetId_t, NetId);
    UT_GenStub_AddParam(SBN_Worker_DispatchToNet, SBN_Worker_NetJobFunc_t, HandlerFunc);

    UT_GenStub_Execute(SBN_Worker_DispatchToNet, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_DispatchToPeer()
 * ----------------------------------------------------
 */
void SBN_Worker_DispatchToPeer(SBN_Worker_TaskCtxt_t *Ctxt, SBN_PeerId_t PeerId, SBN_Worker_PeerJobFunc_t HandlerFunc)
{
    UT_GenStub_AddParam(SBN_Worker_DispatchToPeer, SBN_Worker_TaskCtxt_t *, Ctxt);
    UT_GenStub_AddParam(SBN_Worker_DispatchToPeer, SBN_PeerId_t, PeerId);
    UT_GenStub_AddParam(SBN_Worker_DispatchToPeer, SBN_Worker_PeerJobFunc_t, HandlerFunc);

    UT_GenStub_Execute(SBN_Worker_DispatchToPeer, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_EntryPoint()
 * ----------------------------------------------------
 */
void SBN_Worker_EntryPoint(void)
{
    UT_GenStub_Execute(SBN_Worker_EntryPoint, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_GetAssignment()
 * ----------------------------------------------------
 */
void SBN_Worker_GetAssignment(SBN_Worker_WorkItem_t *Assignment)
{
    UT_GenStub_AddParam(SBN_Worker_GetAssignment, SBN_Worker_WorkItem_t *, Assignment);

    UT_GenStub_Execute(SBN_Worker_GetAssignment, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_IsJobPending()
 * ----------------------------------------------------
 */
bool SBN_Worker_IsJobPending(uint32 JobId)
{
    UT_GenStub_SetupReturnBuffer(SBN_Worker_IsJobPending, bool);

    UT_GenStub_AddParam(SBN_Worker_IsJobPending, uint32, JobId);

    UT_GenStub_Execute(SBN_Worker_IsJobPending, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Worker_IsJobPending, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_Pause()
 * ----------------------------------------------------
 */
void SBN_Worker_Pause(void)
{
    UT_GenStub_Execute(SBN_Worker_Pause, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_Resume()
 * ----------------------------------------------------
 */
void SBN_Worker_Resume(void)
{
    UT_GenStub_Execute(SBN_Worker_Resume, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_ScheduleJob()
 * ----------------------------------------------------
 */
bool SBN_Worker_ScheduleJob(uint32 *JobIdOut, const SBN_Worker_WorkItem_t *Job)
{
    UT_GenStub_SetupReturnBuffer(SBN_Worker_ScheduleJob, bool);

    UT_GenStub_AddParam(SBN_Worker_ScheduleJob, uint32 *, JobIdOut);
    UT_GenStub_AddParam(SBN_Worker_ScheduleJob, const SBN_Worker_WorkItem_t *, Job);

    UT_GenStub_Execute(SBN_Worker_ScheduleJob, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Worker_ScheduleJob, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_Setup()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Worker_Setup(void)
{
    UT_GenStub_SetupReturnBuffer(SBN_Worker_Setup, SBN_Status_t);

    UT_GenStub_Execute(SBN_Worker_Setup, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Worker_Setup, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Worker_Shutdown()
 * ----------------------------------------------------
 */
void SBN_Worker_Shutdown(void)
{
    UT_GenStub_Execute(SBN_Worker_Shutdown, Basic, NULL);
}
