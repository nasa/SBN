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
** File: sbn_stubs.c
**
** Purpose:
** Stubs for SBN functions so that modules can be tested.
*/

#include <string.h> /* for memcpy */

#include "sbn_stubs.h"
#include "utstubs.h"
#include "sbn_app.h"

void UT_DefaultHandler_SBN_UnpackMsg(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_MsgSz_t       *MsgSzPtr       = UT_Hook_GetArgValueByName(Context, "MsgSz", SBN_MsgSz_t *);
    SBN_MsgType_t     *MsgTypePtr     = UT_Hook_GetArgValueByName(Context, "MsgType", SBN_MsgType_t *);
    SBN_ProcessorID_t *ProcessorIDPtr = UT_Hook_GetArgValueByName(Context, "ProcessorID", SBN_ProcessorID_t *);
    void              *Msg            = UT_Hook_GetArgValueByName(Context, "Msg", void *);
    int32              status;
    SBN_Unpack_Buf_t   p;

    memset(&p, 0, sizeof(p));

    UT_Stub_GetInt32StatusCode(Context, &status);

    if (status >= 0 && UT_Stub_CopyToLocal(UT_KEY(SBN_UnpackMsg), &p, sizeof(p)) == sizeof(p))
    {
        if (MsgSzPtr != NULL)
            *MsgSzPtr = p.MsgSz;
        if (MsgTypePtr != NULL)
            *MsgTypePtr = p.MsgType;
        if (ProcessorIDPtr != NULL)
            *ProcessorIDPtr = p.ProcessorID;
        if (Msg != NULL)
            memcpy(Msg, p.MsgBuf, p.MsgSz);
    }
}

void UT_DefaultHandler_SBN_GetPeer(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    int32                status = 0;
    SBN_PeerInterface_t *p      = NULL;

    UT_Stub_GetInt32StatusCode(Context, &status);

    if (status >= 0 && UT_Stub_CopyToLocal(UT_KEY(SBN_GetPeer), &p, sizeof(p)) == sizeof(p))
    {
        UT_Stub_SetReturnValue(FuncKey, p);
    }
}
