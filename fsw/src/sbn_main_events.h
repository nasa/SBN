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

/************************************************************************
**   sbn_events.h
**
**   2014/11/21 ejtimmon
**
**   Specification for the Software Bus Network event identifiers.
**
*************************************************************************/

#ifndef _sbn_main_events_h_
#define _sbn_main_events_h_

#define SBN_FIRST_EID 0x0001

#define SBN_SB_EID       SBN_FIRST_EID + 0
#define SBN_INIT_EID     SBN_FIRST_EID + 1
#define SBN_MSG_EID      SBN_FIRST_EID + 2
#define SBN_TBL_EID      SBN_FIRST_EID + 3
#define SBN_PEER_EID     SBN_FIRST_EID + 4
#define SBN_PROTO_EID    SBN_FIRST_EID + 5
#define SBN_CMD_EID      SBN_FIRST_EID + 6
#define SBN_SUB_EID      SBN_FIRST_EID + 7
/* SBN_FIRST_EID + 8 is available for future use. */
#define SBN_PEERTASK_EID SBN_FIRST_EID + 9
#define SBN_DEBUG_EID    SBN_FIRST_EID + 10

#endif /* _sbn_main_events_h_ */
