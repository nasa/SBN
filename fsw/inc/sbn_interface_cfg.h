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
 * SBN Application Mission Configuration Header File
 *
 * This is a compatibility header for the "mission_cfg.h" file that has
 * traditionally provided public config definitions for each CFS app.
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef SBN_INTERFACE_CFG_H
#define SBN_INTERFACE_CFG_H

#include "sbn_interface_cfg_values.h"

/**
 * @brief SBN modules can provide status messages for housekeeping requests,
 * this is the maximum length those messages can be.
 */
#define SBN_MISSION_MOD_STATUS_MSG_SZ         SBN_MISSION_CFGVAL(MOD_STATUS_MSG_SZ)
#define DEFAULT_SBN_MISSION_MOD_STATUS_MSG_SZ 128

/** @brief Maximum number of subscriptions allowed per peer allowed. */
#define SBN_MISSION_MAX_SUBS_PER_PEER         SBN_MISSION_CFGVAL(MAX_SUBS_PER_PEER)
#define DEFAULT_SBN_MISSION_MAX_SUBS_PER_PEER 256

/**
 * @brief The maximum length of a module's name
 * file.
 */
#define SBN_MISSION_MAX_MOD_NAME_LEN         SBN_MISSION_CFGVAL(MAX_MOD_NAME_LEN)
#define DEFAULT_SBN_MISSION_MAX_MOD_NAME_LEN 16

/** @brief Maximum number of protocol modules. */
#define SBN_MISSION_MAX_MOD_COUNT         SBN_MISSION_CFGVAL(MAX_MOD_COUNT)
#define DEFAULT_SBN_MISSION_MAX_MOD_COUNT 8

/** @brief Maximum number of peers. */
#define SBN_MISSION_MAX_PEER_COUNT         SBN_MISSION_CFGVAL(MAX_PEER_COUNT)
#define DEFAULT_SBN_MISSION_MAX_PEER_COUNT 16

/** @brief Maximum number of outgoing and incoming message filters for each peer. */
#define SBN_MISSION_MAX_FILTERS_PER_PEER         SBN_MISSION_CFGVAL(MAX_FILTERS_PER_PEER)
#define DEFAULT_SBN_MISSION_MAX_FILTERS_PER_PEER 8

/**
 * @brief The number of characters for a "peer address", this can be
 * an IP address, a device inode path, a DTN EIN, etc. The meaning
 * of the address field is network module-dependent.
 */
#define SBN_MISSION_ADDR_SZ         SBN_MISSION_CFGVAL(ADDR_SZ)
#define DEFAULT_SBN_MISSION_ADDR_SZ 48

#endif
