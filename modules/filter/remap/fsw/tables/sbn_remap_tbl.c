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

#include "sbn_remap_tbl.h"
#include "cfe_tbl_filedef.h"

/* clang-format off */
SBN_RemapTbl_t SBN_RemapTbl =
{
    .RemapDefaultFlag = SBN_REMAP_DEFAULT_SEND,

    .Entries =
    {
        { .ProcessorID = 3, .SpacecraftID = 0x42, .FromMID = CFE_SB_MSGID_WRAP_VALUE(0x0882), .ToMID = CFE_SB_MSGID_WRAP_VALUE(0x0883) }
    }
}; /* end SBN_RemapTbl */

CFE_TBL_FILEDEF(SBN_RemapTbl, SBN.RemapTbl, SBN Remap Table, sbn_remap_tbl.tbl)
