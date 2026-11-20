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
#include "sbn_worker.h"
#include "sbn_resource.h"
#include "sbn_inbound.h"
#include "sbn_outbound.h"
#include "sbn_connection.h"

/*
 * The mask applied when converting a job ID into an actual entry index.
 * This relies on the queue depth being a power of 2.
 */
#define SBN_WORKER_QUEUE_MASK (SBN_PLATFORM_WORKER_QUEUE_DEPTH - 1)

/* Local data structure (may be rolled into the main SBN_AppData) */
SBN_Worker_WorkQueue_t SBN_WorkQueue;

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Gets a pointer to the work item based on the Job ID
 * This must only be used when the state is locked.
 *
 *-----------------------------------------------------------------*/
static inline SBN_Worker_WorkItem_t *SBN_Worker_GetJob(uint32 JobId)
{
    uint32 Idx = (JobId & SBN_WORKER_QUEUE_MASK);
    return &SBN_WorkQueue.WorkItems[Idx];
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Checks if any work items are pending (not picked up yet)
 *
 *-----------------------------------------------------------------*/
static inline bool SBN_Worker_IsPending(void)
{
    return (SBN_WorkQueue.NumPosted != SBN_WorkQueue.NumClaimed);
}

/*----------------------------------------------------------------
 *
 * Local Helper function, not used outside of this unit
 * Checks if all work items are complete (nothing pending nor in progress)
 *
 *-----------------------------------------------------------------*/
static inline bool SBN_Worker_IsIdle(void)
{
    return (SBN_WorkQueue.NumPosted == SBN_WorkQueue.NumCompleted);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_GetAssignment(SBN_Worker_WorkItem_t *Assignment)
{
    OS_time_t WakeupTime;

    OS_GetLocalTime(&WakeupTime);

    OS_CondVarLock(SBN_WorkQueue.Sync);

    ++SBN_WorkQueue.NumCompleted;
    Assignment->JobType = SBN_Worker_JobType_NONE;

    if (!SBN_WorkQueue.ShouldRun)
    {
        /* because the main thread might be waiting for idle-ness */
        OS_CondVarBroadcast(SBN_WorkQueue.Sync);
    }

    while (!SBN_WorkQueue.ShouldExit)
    {
        if (SBN_Worker_IsPending())
        {
            /* snapshot the job description.  This is small. */
            *Assignment = *(SBN_Worker_GetJob(SBN_WorkQueue.NumClaimed));
            ++SBN_WorkQueue.NumClaimed;

            /* NOTE: even if the job type is NONE, we claimed it and thus incremented the count.
             * we must exit this loop, do the job, then come back so it will be properly "Completed" */
            break;
        }

        /* this is done with a timeout so we periodically check ShouldExit */
        WakeupTime = OS_TimeAdd(WakeupTime, OS_TimeFromTotalMilliseconds(SBN_PLATFORM_WORKER_WAIT_TIME_MSEC));
        OS_CondVarTimedWait(SBN_WorkQueue.Sync, &WakeupTime);
    }
    OS_CondVarUnlock(SBN_WorkQueue.Sync);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_DispatchToNet(SBN_Worker_TaskCtxt_t *Ctxt, SBN_NetId_t NetId, SBN_Worker_NetJobFunc_t HandlerFunc)
{
    SBN_NetRecord_t *NetRec;

    NetRec = SBN_LocateNetRecordByID(NetId);
    if (SBN_NetRecordIsMatch(NetRec, NetId))
    {
        HandlerFunc(NetRec, &Ctxt->BufferDesc, SBN_PLATFORM_MAX_MSG_PER_WAKEUP);
    }
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_DispatchToPeer(SBN_Worker_TaskCtxt_t *Ctxt, SBN_PeerId_t PeerId, SBN_Worker_PeerJobFunc_t HandlerFunc)
{
    SBN_PeerRecord_t *PeerRec;

    PeerRec = SBN_LocatePeerRecordByID(PeerId);
    if (SBN_PeerRecordIsMatch(PeerRec, PeerId))
    {
        HandlerFunc(PeerRec, &Ctxt->BufferDesc, SBN_PLATFORM_MAX_MSG_PER_WAKEUP);
    }
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_DispatchAssignment(SBN_Worker_TaskCtxt_t *Ctxt, const SBN_Worker_WorkItem_t *Assignment)
{
    memset(&Ctxt->BufferDesc, 0, sizeof(Ctxt->BufferDesc));

    Ctxt->BufferDesc.DataPtr   = &Ctxt->u;
    Ctxt->BufferDesc.MaxDataSz = sizeof(Ctxt->u);

    switch (Assignment->JobType)
    {
        case SBN_Worker_JobType_PEER_OUTBOUND:
            SBN_Worker_DispatchToPeer(Ctxt, Assignment->Info.Comms.PeerId, SBN_Outbound_ServicePeer);
            break;
        case SBN_Worker_JobType_PEER_INBOUND:
            SBN_Worker_DispatchToPeer(Ctxt, Assignment->Info.Comms.PeerId, SBN_Inbound_ServicePeer);
            break;
        case SBN_Worker_JobType_NET_INBOUND:
            SBN_Worker_DispatchToNet(Ctxt, Assignment->Info.Comms.NetId, SBN_Inbound_ServiceNet);
            break;
        default:
            break;
    }

    ++Ctxt->NumCompleted;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_EntryPoint(void)
{
    SBN_Worker_WorkItem_t  Assignment;
    SBN_Worker_TaskCtxt_t *Ctxt;

    OS_CondVarLock(SBN_WorkQueue.Sync);
    Ctxt = &SBN_WorkQueue.Tasks[SBN_WorkQueue.NumWorkers];
    ++SBN_WorkQueue.NumWorkers;

    /* When this worker gets its first assignment, it will increment the completed count.
     * This first time through it had no real job yet.  So pretend this startup was a job. */
    ++SBN_WorkQueue.NumPosted;
    SBN_WorkQueue.NumClaimed = SBN_WorkQueue.NumPosted;

    OS_CondVarBroadcast(SBN_WorkQueue.Sync);
    OS_CondVarUnlock(SBN_WorkQueue.Sync);

    while (true)
    {
        /* this will pend/block until something interesting is there */
        SBN_Worker_GetAssignment(&Assignment);

        if (SBN_WorkQueue.ShouldExit)
        {
            break;
        }

        if (SBN_WorkQueue.ShouldRun)
        {
            SBN_Worker_DispatchAssignment(Ctxt, &Assignment);
        }
    }

    OS_CondVarLock(SBN_WorkQueue.Sync);
    --SBN_WorkQueue.NumWorkers;
    OS_CondVarBroadcast(SBN_WorkQueue.Sync);
    OS_CondVarUnlock(SBN_WorkQueue.Sync);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
SBN_Status_t SBN_Worker_Setup(void)
{
    uint32       i;
    char         TaskName[CFE_MISSION_MAX_API_LEN];
    CFE_Status_t Status;

    memset(&SBN_WorkQueue, 0, sizeof(SBN_WorkQueue));

    /* We can use a condition variable to sync the threads */
    OS_CondVarCreate(&SBN_WorkQueue.Sync, "SBN_Worker", 0);

    i = 0;
    while (i < SBN_PLATFORM_NUM_WORKER_TASKS)
    {
        ++i;

        snprintf(TaskName, sizeof(TaskName), "SBN_WORK_%02u", (unsigned int)i);
        Status = CFE_ES_CreateChildTask(&SBN_WorkQueue.Tasks[i].TaskId,
                                        TaskName,
                                        SBN_Worker_EntryPoint,
                                        NULL,
                                        SBN_PLATFORM_WORKER_TASK_STACK_SIZE,
                                        SBN_PLATFORM_WORKER_TASK_PRIORITY,
                                        0);
        if (Status != CFE_SUCCESS)
        {
            break;
        }

        /* Wait for the child to grab its slot */
        OS_CondVarLock(SBN_WorkQueue.Sync);
        while (SBN_WorkQueue.NumWorkers != i)
        {
            OS_CondVarWait(SBN_WorkQueue.Sync);
        }
        OS_CondVarUnlock(SBN_WorkQueue.Sync);
    }

    return SBN_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_Shutdown(void)
{
    OS_CondVarLock(SBN_WorkQueue.Sync);
    SBN_WorkQueue.ShouldExit = true;
    while (SBN_WorkQueue.NumWorkers != 0)
    {
        OS_CondVarSignal(SBN_WorkQueue.Sync);
        OS_CondVarWait(SBN_WorkQueue.Sync);
    }
    OS_CondVarUnlock(SBN_WorkQueue.Sync);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_Pause(void)
{
    OS_CondVarLock(SBN_WorkQueue.Sync);

    /* Instructs all workers to pause.  When they get to their stopping point,
     * they will also broadcast the signal so we can check it again */
    SBN_WorkQueue.ShouldRun = false;

    while (!SBN_Worker_IsIdle())
    {
        OS_CondVarWait(SBN_WorkQueue.Sync);
    }

    OS_CondVarUnlock(SBN_WorkQueue.Sync);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void SBN_Worker_Resume(void)
{
    OS_CondVarLock(SBN_WorkQueue.Sync);

    /* Instructs all workers to resume.
     * This broadcasts the signal so workers will check */
    SBN_WorkQueue.ShouldRun = true;

    /* This broadcast might not be needed.  For the whole time it was paused,
     * new job scheduling was also blocked too.  This means the queue is guaranteed empty. */
    OS_CondVarBroadcast(SBN_WorkQueue.Sync);
    OS_CondVarUnlock(SBN_WorkQueue.Sync);
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool SBN_Worker_ScheduleJob(uint32 *JobIdOut, const SBN_Worker_WorkItem_t *Job)
{
    uint32 Depth;
    uint32 Position;
    bool   GotSpace;

    OS_CondVarLock(SBN_WorkQueue.Sync);

    Position = SBN_WorkQueue.NumPosted;
    Depth    = Position - SBN_WorkQueue.NumCompleted;
    if (SBN_WorkQueue.ShouldRun)
    {
        GotSpace = (Depth < SBN_PLATFORM_WORKER_QUEUE_DEPTH);
    }
    else
    {
        GotSpace = false;
    }

    if (GotSpace)
    {
        /* put job into the queue */
        *(SBN_Worker_GetJob(Position)) = *Job;
        ++SBN_WorkQueue.NumPosted;

        if (JobIdOut != NULL)
        {
            *JobIdOut = Position;
        }

        /* we only need to signal a single task to come get this. */
        OS_CondVarSignal(SBN_WorkQueue.Sync);
    }

    OS_CondVarUnlock(SBN_WorkQueue.Sync);

    return GotSpace;
}

/*----------------------------------------------------------------
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool SBN_Worker_IsJobPending(uint32 JobId)
{
    uint32 Completed;
    uint32 Posted;

    /* Be sure to read the globals only once, as they can be updated at any time */
    Completed = SBN_WorkQueue.NumCompleted;
    Posted    = SBN_WorkQueue.NumPosted;

    return ((JobId - Completed) < (Posted - Completed));
}
