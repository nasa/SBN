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
 *   SBN Application Topic IDs
 */
#ifndef SBN_TOPICIDS_H
#define SBN_TOPICIDS_H

#include "sbn_topicid_values.h"

#define SBN_MISSION_CMD_TOPICID                     SBN_MISSION_TIDVAL(CMD)
#define DEFAULT_SBN_MISSION_CMD_TOPICID             189
#define SBN_MISSION_HK_TLM_TOPICID                  SBN_MISSION_TIDVAL(HK_TLM)
#define DEFAULT_SBN_MISSION_HK_TLM_TOPICID          190
#define SBN_MISSION_HK_NET_TLM_TOPICID              SBN_MISSION_TIDVAL(HK_NET_TLM)
#define DEFAULT_SBN_MISSION_HK_NET_TLM_TOPICID      191
#define SBN_MISSION_HK_PEER_TLM_TOPICID             SBN_MISSION_TIDVAL(HK_PEER_TLM)
#define DEFAULT_SBN_MISSION_HK_PEER_TLM_TOPICID     192
#define SBN_MISSION_HK_MYSUBS_TLM_TOPICID           SBN_MISSION_TIDVAL(HK_MYSUBS_TLM)
#define DEFAULT_SBN_MISSION_HK_MYSUBS_TLM_TOPICID   193
#define SBN_MISSION_HK_PEERSUBS_TLM_TOPICID         SBN_MISSION_TIDVAL(HK_PEERSUBS_TLM)
#define DEFAULT_SBN_MISSION_HK_PEERSUBS_TLM_TOPICID 194

#endif
