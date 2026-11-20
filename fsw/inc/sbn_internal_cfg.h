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
 * SBN Application Platform Configuration Header File
 *
 * This is a compatibility header for the "platform_cfg.h" file that has
 * traditionally provided both public and private config definitions
 * for each CFS app.
 *
 * These definitions are now provided in two separate files, one for
 * the public/mission scope and one for internal scope.
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef SBN_INTERNAL_CFG_H
#define SBN_INTERNAL_CFG_H

#include "sbn_internal_cfg_values.h"

/** @brief Maximum number of networks allowed. */
#define SBN_PLATFORM_MAX_NETS         SBN_PLATFORM_CFGVAL(MAX_NETS)
#define DEFAULT_SBN_PLATFORM_MAX_NETS 16

/** @brief Maximum number of incoming and outgoing message filters. */
#define SBN_PLATFORM_MAX_FILTERS         SBN_PLATFORM_CFGVAL(MAX_FILTERS)
#define DEFAULT_SBN_PLATFORM_MAX_FILTERS 16

/**
 * @brief At most process this many SB messages per peer per wakeup.
 *
 * Ideally, a worker task servicing a data pipe (inbound or outbound) will service that
 * pipe until it is empty.  However, if there is a substantial backlog of data, this can
 * cause the worker to get stuck servicing the backlogged items, causing other peers/nets
 * to start backing up as well.
 *
 * This limits the number of messages processed on a single wakeup, such that other
 * peers/nets can still be serviced when one or more of them are saturated and never become
 * idle.
 */
#define SBN_PLATFORM_MAX_MSG_PER_WAKEUP         SBN_PLATFORM_CFGVAL(MAX_MSG_PER_WAKEUP)
#define DEFAULT_SBN_PLATFORM_MAX_MSG_PER_WAKEUP 32

/**
 * @brief In the polling configuration, how long (in milliseconds) to wait for
 * a SCH wakeup message before SBN times out and processes. (Note, should
 * really be significantly longer than the expected time between SCH wakeup
 * messages.)
 */
#define SBN_PLATFORM_MAIN_LOOP_DELAY         SBN_PLATFORM_CFGVAL(MAIN_LOOP_DELAY)
#define DEFAULT_SBN_PLATFORM_MAIN_LOOP_DELAY 200

/**
 * @brief For each peer, a pipe is created to receive messages that the peer has
 * subscribed to. The pipe should be deep enough to handle all messages that
 * will queue between wakeups.
 */
#define SBN_PLATFORM_PEER_PIPE_DEPTH         SBN_PLATFORM_CFGVAL(PEER_PIPE_DEPTH)
#define DEFAULT_SBN_PLATFORM_PEER_PIPE_DEPTH 32

/**
 * @brief The maximum number of messages that will be queued for a particular
 * message ID for a particular peer.
 */
#define SBN_PLATFORM_PEER_MSG_LIM         SBN_PLATFORM_CFGVAL(PEER_MSG_LIM)
#define DEFAULT_SBN_PLATFORM_PEER_MSG_LIM 8

/**
 * @brief The maximum number of subscription messages that will be queued
 * between wakeups.
 */
#define SBN_PLATFORM_SUB_PIPE_DEPTH         SBN_PLATFORM_CFGVAL(SUB_PIPE_DEPTH)
#define DEFAULT_SBN_PLATFORM_SUB_PIPE_DEPTH 32

/**
 * @brief The maximum number of subscription messages for a single message ID
 * that will be queued between wakeups. (These are received when updates occur
 * after SBN starts up.)
 */
#define SBN_PLATFORM_MAX_ONESUB_PKTS_ON_PIPE         SBN_PLATFORM_CFGVAL(MAX_ONESUB_PKTS_ON_PIPE)
#define DEFAULT_SBN_PLATFORM_MAX_ONESUB_PKTS_ON_PIPE 16

/**
 * @brief The maximum number of subscription messages for all message IDs that
 * will be queued between wakeups. (These are received on SBN startup.)
 */
#define SBN_PLATFORM_MAX_ALLSUBS_PKTS_ON_PIPE         SBN_PLATFORM_CFGVAL(MAX_ALLSUBS_PKTS_ON_PIPE)
#define DEFAULT_SBN_PLATFORM_MAX_ALLSUBS_PKTS_ON_PIPE 64

/**
 * @brief The number of subscription changes that can be tracked as incremental updates
 *
 * Changes within this limit can be sent incrementally to the peer,
 * or applied incrementally to the local pipe for this peer.
 *
 * Beyond this, a full subscription re-sync will need to be done.
 *
 * This needs to be big enough to absorb the largest number of subs that
 * can come in a single subscription message from the peer.  In the current
 * implementation this is #SBN_MISSION_MAX_SUBS_PER_PEER.
 *
 * It must be a power of two such that it can be accessed as a ring buffer.
 */
#define SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH         SBN_PLATFORM_CFGVAL(SUBSCRIPTION_UPDATE_QUEUE_DEPTH)
#define DEFAULT_SBN_PLATFORM_SUBSCRIPTION_UPDATE_QUEUE_DEPTH 512

/**
 * @brief The number of worker tasks to create
 *
 * These tasks will service moving data between the local machine and the peers
 *
 * This should be set to the number of configured peers which are configured
 * to have a dedicated task, but must be at least 1 even in polling mode.
 *
 * This represents the upper bound on the number of parallel operations that
 * can take place.  If more peers are configured with a "task" flag, then the
 * worker tasks will round-robin between them.
 */
#define SBN_PLATFORM_NUM_WORKER_TASKS         SBN_PLATFORM_CFGVAL(NUM_WORKER_TASKS)
#define DEFAULT_SBN_PLATFORM_NUM_WORKER_TASKS 2

/**
 * @brief The depth of the assignment queue
 *
 * This is a the size of a ring buffer used to schedule the worker tasks
 * It must be a power of 2 that is greater than double the number of configured peers
 * plus the number of nets.
 *
 * Each peer may have inbound and outbound traffic is scheduled independently, as well
 * as inbound traffic at the net level.
 */
#define SBN_PLATFORM_WORKER_QUEUE_DEPTH         SBN_PLATFORM_CFGVAL(WORKER_QUEUE_DEPTH)
#define DEFAULT_SBN_PLATFORM_WORKER_QUEUE_DEPTH 64

/**
 * @brief Stack Size for SBN worker tasks
 *
 * This is the stack size for the threads that process and move network data in SBN.
 *
 * Note that some SBN modules create ad-hoc network data buffers on the stack, which can
 * be at least #SBN_MAX_PACKED_MSG_SZ.  The default value is therefore set large
 * enough to accommodate use of these modules.
 *
 * If the SBN modules in use do _not_ use the stack for buffering, this can be set
 * to a smaller value to save memory.
 */
#define SBN_PLATFORM_WORKER_TASK_STACK_SIZE         SBN_PLATFORM_CFGVAL(WORKER_TASK_STACK_SIZE)
#define DEFAULT_SBN_PLATFORM_WORKER_TASK_STACK_SIZE 131072

/**
 * @brief Priority for SBN worker tasks
 *
 * The OSAL task priority for SBN worker tasks
 */
#define SBN_PLATFORM_WORKER_TASK_PRIORITY         SBN_PLATFORM_CFGVAL(WORKER_TASK_PRIORITY)
#define DEFAULT_SBN_PLATFORM_WORKER_TASK_PRIORITY 85

/**
 * @brief Wait time for SBN worker tasks
 *
 * When idle/waiting on an assignment, the worker tasks will periodically wake up
 * and check other system flags, which in turn will allow them to pause or shut down
 * as needed.  This controls the period of that wake up.  Larger values will be
 * more CPU efficient at the cost of higher latency for some configuration and
 * control operations.
 */
#define SBN_PLATFORM_WORKER_WAIT_TIME_MSEC         SBN_PLATFORM_CFGVAL(WORKER_WAIT_TIME_MSEC)
#define DEFAULT_SBN_PLATFORM_WORKER_WAIT_TIME_MSEC 100

/**
 * @brief If defined, remapping is enabled at boot time.
 */
#define SBN_PLATFORM_REMAP_ENABLED         SBN_PLATFORM_CFGVAL(REMAP_ENABLED)
#define DEFAULT_SBN_PLATFORM_REMAP_ENABLED true

#define SBN_PLATFORM_REMAP_TBL_FILENAME         SBN_PLATFORM_CFGVAL(REMAP_TBL_FILENAME)
#define DEFAULT_SBN_PLATFORM_REMAP_TBL_FILENAME "/cf/sbn_remap_tbl.tbl"

#define SBN_PLATFORM_CONF_TBL_FILENAME         SBN_PLATFORM_CFGVAL(CONF_TBL_FILENAME)
#define DEFAULT_SBN_PLATFORM_CONF_TBL_FILENAME "/cf/sbn_conf_tbl.tbl"

#endif /* SBN_INTERNAL_CFG_H */
