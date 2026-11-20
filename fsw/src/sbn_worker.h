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

#ifndef SBN_WORKER_H
#define SBN_WORKER_H

#include "common_types.h"
#include "cfe_es_api_typedefs.h"
#include "cfe_mission_cfg.h"
#include "sbn_types.h"
#include "sbn_interfaces.h"

/* A small amount of extra buffer space for the SBN protocol messagse */
#define SBN_WORKER_BUFFER_SZ (SBN_PACKED_HDR_SZ + CFE_MISSION_SB_MAX_SB_MSG_SIZE)

/**
 * Identifies the type of job being scheduled/performed
 */
typedef enum
{
    SBN_Worker_JobType_NONE,          /**< Reserved, do not use */
    SBN_Worker_JobType_PEER_OUTBOUND, /**< Outbound data traffic on a peer */
    SBN_Worker_JobType_PEER_INBOUND,  /**< Inbound data traffic on a peer */
    SBN_Worker_JobType_NET_INBOUND    /**< Inbound data traffic on a net */
} SBN_Worker_JobType_t;

/**
 * Context for communication-related work items
 *
 * This indicates the intended peer/net.  For Net
 * contexts, the PeerId will be invalid (0).
 */
typedef struct SBN_Worker_CommsInfo
{
    SBN_NetId_t  NetId;
    SBN_PeerId_t PeerId;
} SBN_Worker_CommsInfo_t;

/**
 * Context for work item
 *
 * at the moment only communication tasks are
 * scheduled via this method, but this is made as
 * a union so it can be extended
 */
typedef union SBN_Worker_Info
{
    SBN_Worker_CommsInfo_t Comms;
} SBN_Worker_Info_t;

/**
 * The entry in the work queue
 */
typedef struct SBN_Worker_WorkItem
{
    SBN_Worker_JobType_t JobType; /**< Type of job this is */
    SBN_Worker_Info_t    Info;    /**< Context for the job */
} SBN_Worker_WorkItem_t;

/**
 * A context object for a single worker task
 *
 * This contains some stats tracking info as well
 * as any substantially-sized buffers needed for processing
 * data traffic.  This is allocated statically to avoid
 * putting large objects on the stack.
 */
typedef struct SBN_Worker_TaskCtxt
{
    CFE_ES_TaskId_t TaskId;
    uint32          NumCompleted;

    SBN_BufferDesc_t BufferDesc;

    /* temporary storage for messages/buffering */
    union
    {
        uint8              MessageBuffer[SBN_WORKER_BUFFER_SZ];
        double             AlignDbl;
        unsigned long long AlignLong;
    } u;

} SBN_Worker_TaskCtxt_t;

/**
 * The main object representing the overall work queue for the system.
 *
 * Some items are qualified as "volatile" because they may be read by
 * tasks other than the ones updating it, and in these cases the
 * order of operations is very important.  Only tasks that hold
 * the sync mutex should ever update/write to these counters.
 */
typedef struct SBN_Worker_WorkQueue
{
    SBN_Worker_WorkItem_t WorkItems[SBN_PLATFORM_WORKER_QUEUE_DEPTH];

    SBN_Worker_TaskCtxt_t Tasks[SBN_PLATFORM_NUM_WORKER_TASKS];

    /* gate to hold all workers, if needed */
    volatile bool ShouldRun;

    /* instructs all workers to exit themselves */
    volatile bool ShouldExit;

    volatile uint32 NumPosted;
    volatile uint32 NumClaimed;
    volatile uint32 NumCompleted;

    uint32 NumWorkers;

    osal_id_t Sync;
} SBN_Worker_WorkQueue_t;

extern SBN_Worker_WorkQueue_t SBN_WorkQueue;

/**
 * The signature of a worker function that operates on a peer
 */
typedef void (*SBN_Worker_PeerJobFunc_t)(SBN_PeerRecord_t *PeerRec, SBN_BufferDesc_t *BufD, int32 MsgLim);

/**
 * The signature of a worker function that operates on a net
 */
typedef void (*SBN_Worker_NetJobFunc_t)(SBN_NetRecord_t *NetRec, SBN_BufferDesc_t *BufD, int32 MsgLim);

/**
 * Gets the next assignment
 *
 * This will block until a new assignment is available.  Once it is obtained,
 * the buffer will be filled with the assignment information.  This does not
 * return a value, all output is in the assignment.
 *
 * This is intended to be called from the context of a worker thread
 *
 * @note in some cases the assignment could be NONE.
 *
 * @param[out] Assignment  Buffer to populate with next assignment detail
 */
void SBN_Worker_GetAssignment(SBN_Worker_WorkItem_t *Assignment);

/**
 * Dispatches an assignment at the net level
 *
 * Calls a net-oriented worker function using the given NetId
 *
 * This is intended to be called from the context of a worker thread
 *
 * @param[inout] Ctxt        The context block for this worker task
 * @param[in]    NetId       The network ID to operate on
 * @param[in]    HandlerFunc The function to invoke
 */
void SBN_Worker_DispatchToNet(SBN_Worker_TaskCtxt_t *Ctxt, SBN_NetId_t NetId, SBN_Worker_NetJobFunc_t HandlerFunc);

/**
 * Dispatches an assignment at the peer level
 *
 * Calls a peer-oriented worker function using the given PeerId
 *
 * This is intended to be called from the context of a worker thread
 *
 * @param[inout] Ctxt        The context block for this worker task
 * @param[in]    PeerId      The peer ID to operate on
 * @param[in]    HandlerFunc The function to invoke
 */
void SBN_Worker_DispatchToPeer(SBN_Worker_TaskCtxt_t *Ctxt, SBN_PeerId_t PeerId, SBN_Worker_PeerJobFunc_t HandlerFunc);

/**
 * Dispatches an assignment
 *
 * Identifies whether this assignment correlates to a net or a peer,
 * and invokes the handler function to perform that work item.
 *
 * This is intended to be called from the context of a worker thread
 *
 * @param[inout] Ctxt        The context block for this worker task
 * @param[in]    Assignment  Buffer containing next assignment detail
 */
void SBN_Worker_DispatchAssignment(SBN_Worker_TaskCtxt_t *Ctxt, const SBN_Worker_WorkItem_t *Assignment);

/**
 * Main task entry point for SBN worker threads
 */
void SBN_Worker_EntryPoint(void);

/**
 * Creates/Spawns all worker threads
 *
 * All worker threads are created upfront at boot time and persist until the SBN
 * app is shut down.  This keeps the memory and thread count more bounded.  This
 * is invoked during initial SBN start up to create the threads.
 *
 * @retval #SBN_SUCCESS if threads created successfully
 */
SBN_Status_t SBN_Worker_Setup(void);

/**
 * Destroy all worker threads
 *
 * All worker threads are created upfront at boot time and persist until the SBN
 * app is shut down.  This is invoked during SBN shut down to destroy the threads.
 */
void SBN_Worker_Shutdown(void);

/**
 * Pause all threads for reconfiguration
 *
 * This will prevent new jobs from being assigned, and wait/block until all worker
 * threads have completed any job that was already in progress.
 *
 * Upon return, SBN is effectively running in a single-threaded mode.  The threads
 * still exist but they are not allowed to perform any new work. This permits any
 * internal configuration structures to be torn down or rebuilt based on
 * configuration changes, without needing to protect between threads.
 */
void SBN_Worker_Pause(void);

/**
 * Resume all threads after reconfiguration
 *
 * Allows the paused worker threads to resume normal operation after reconfig.
 * Upon return, all worker threads will be unblocked, and new jobs can be scheduled.
 */
void SBN_Worker_Resume(void);

/**
 * Schedule a single job for future execution by a worker thread
 *
 * Adds the work item to the job queue, and outputs the location in the queue
 * as an ID.  This ID can then be used to check for completion later.
 *
 * Important: if this does not add the job, then the JobID output is not changed,
 * and will retain its previous value.  It is not set to zero or any other value.
 * The intent is that the status of the Job ID may be checked again on the next cycle,
 * and it should have the same status (i.e. incomplete).
 *
 * @sa SBN_Worker_IsJobPending()
 *
 * @param[out] JobIdOut  Output buffer to hold the job ID
 * @param[in]  Job       Pointer to buffer holding job information
 * @retval true  If job was successfully added.  JobIdOut is updated.
 * @retval false If job was not successfully added.  JobIdOut is NOT updated.
 */
bool SBN_Worker_ScheduleJob(uint32 *JobIdOut, const SBN_Worker_WorkItem_t *Job);

/**
 * Checks if a specific work item has been completed by a worker
 *
 * Using the JobID that was output from a previous call to SBN_Worker_ScheduleJob()
 * this will check if that job has been completed.  This only checks if its been
 * processed by a worker, not whether it was successful or not.
 *
 * @param[in] JobId   The job ID to check
 *
 * @retval true  If job is still pending or not yet completed (may be in active progress)
 * @retval false If job is completed (not in progress)
 */
bool SBN_Worker_IsJobPending(uint32 JobId);

#endif /* SBN_WORKER_H */
