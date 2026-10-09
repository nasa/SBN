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
 * (To prevent starvation if a peer is babbling.)
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
 * @brief If defined, remapping is enabled at boot time.
 */
#define SBN_PLATFORM_REMAP_ENABLED         SBN_PLATFORM_CFGVAL(REMAP_ENABLED)
#define DEFAULT_SBN_PLATFORM_REMAP_ENABLED true

#define SBN_PLATFORM_REMAP_TBL_FILENAME         SBN_PLATFORM_CFGVAL(REMAP_TBL_FILENAME)
#define DEFAULT_SBN_PLATFORM_REMAP_TBL_FILENAME "/cf/sbn_remap_tbl.tbl"

#define SBN_PLATFORM_CONF_TBL_FILENAME         SBN_PLATFORM_CFGVAL(CONF_TBL_FILENAME)
#define DEFAULT_SBN_PLATFORM_CONF_TBL_FILENAME "/cf/sbn_conf_tbl.tbl"

#endif
