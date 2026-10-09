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
 *   Specification for the SBN command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef SBN_FCNCODES_H
#define SBN_FCNCODES_H

#include "sbn_fcncode_values.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Sample App command codes
*/
#define SBN_NOOP_CC          SBN_CCVAL(NOOP)
#define SBN_HK_CC            SBN_CCVAL(SEND_HK)
#define SBN_HK_NET_CC        SBN_CCVAL(SEND_HK_NET)
#define SBN_HK_PEER_CC       SBN_CCVAL(SEND_HK_PEER)
#define SBN_HK_PEERSUBS_CC   SBN_CCVAL(SEND_HK_PEER_SUBS)
#define SBN_HK_MYSUBS_CC     SBN_CCVAL(SEND_HK_MY_SUBS)
#define SBN_HK_RESET_CC      SBN_CCVAL(RESET)
#define SBN_HK_RESET_PEER_CC SBN_CCVAL(RESET_PEER)
#define SBN_SCH_WAKEUP_CC    SBN_CCVAL(WAKEUP)
#define SBN_TBL_CC           SBN_CCVAL(RELOAD_TBL)

#endif
