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
#include "sbn_connection.h"

void UT_DefaultHandler_SBN_Connected(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_PeerInterface_t *Peer = UT_Hook_GetArgValueByName(Context, "Peer", SBN_PeerInterface_t *);
    SBN_Status_t         status;

    UT_Stub_GetInt32StatusCode(Context, &status);

    if (status >= 0 && Peer != NULL)
    {
        Peer->Connected = true;
    }
}

void UT_DefaultHandler_SBN_Disconnected(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_PeerInterface_t *Peer = UT_Hook_GetArgValueByName(Context, "Peer", SBN_PeerInterface_t *);
    SBN_Status_t         status;

    UT_Stub_GetInt32StatusCode(Context, &status);

    if (status >= 0 && Peer != NULL)
    {
        Peer->Connected = false;
    }
}
