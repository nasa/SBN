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

#ifndef _sbn_stubs_h_
#define _sbn_stubs_h_

#include "sbn_interfaces.h"

typedef struct SBN_Unpack_Buf
{
    SBN_MsgSz_t        MsgSz;
    SBN_MsgType_t      MsgType;
    SBN_ProcessorID_t  ProcessorID;
    SBN_SpacecraftID_t SpacecraftID;
    uint8              MsgBuf[256]; /* TODO: use a defined buffer size? */
} SBN_Unpack_Buf_t;

#endif /* _sbn_stubs_h_ */
