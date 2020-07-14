/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2020 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/*
** File: sbn_stubs.c
**
** Purpose:
** Stubs for SBN functions so that modules can be tested.
*/

#include <string.h> /* for memcpy */

#include "sbn_stubs.h"
#include "utstubs.h"

void SBN_PackMsg(void *SBNMsgBuf, SBN_MsgSz_t MsgSz, SBN_MsgType_t MsgType, CFE_ProcessorID_t ProcessorID, void *Msg)
{
    UT_DEFAULT_IMPL(SBN_PackMsg);
}/* end SBN_PackMsg() */

bool SBN_UnpackMsg(void *SBNBuf, SBN_MsgSz_t *MsgSzPtr, SBN_MsgType_t *MsgTypePtr,
    CFE_ProcessorID_t *ProcessorIDPtr, void *Msg)
{
    uint32 status = 0;
    SBN_Unpack_Buf_t p;

    status = UT_DEFAULT_IMPL(SBN_UnpackMsg);

    if (status >= 0)
    {
        if (UT_Stub_CopyToLocal(UT_KEY(SBN_UnpackMsg), &p, sizeof(p)) < sizeof(p))
        {
            return NULL;
        }
    }

    if (MsgSzPtr != NULL) *MsgSzPtr = p.MsgSz;
    if (MsgTypePtr != NULL) *MsgTypePtr = p.MsgType;
    if (ProcessorIDPtr != NULL) *ProcessorIDPtr = p.ProcessorID;
    if (Msg != NULL) memcpy(Msg, p.MsgBuf, p.MsgSz);

    return true;
}/* end SBN_UnpackMsg() */

SBN_Status_t SBN_Connected(SBN_PeerInterface_t *Peer)
{
    SBN_Status_t status;
    
    status = UT_DEFAULT_IMPL(SBN_Connected);

    if (status >= 0)
    {
        Peer->Connected = true;
    }

    return status;
}/* end SBN_Connected() */

SBN_Status_t SBN_Disconnected(SBN_PeerInterface_t *Peer)
{
    SBN_Status_t status;
    
    status = UT_DEFAULT_IMPL(SBN_Disconnected);

    if (status >= 0)
    {
        Peer->Connected = false;
    }

    return status;
}/* end SBN_Disconnected() */

SBN_MsgSz_t SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer)
{
    return UT_DEFAULT_IMPL_RC(SBN_SendNetMsg, MsgSz);
}/* end SBN_SendNetMsg() */

SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID)
{
    uint32 status = 0;
    SBN_PeerInterface_t *p = NULL;

    status = UT_DEFAULT_IMPL(SBN_GetPeer);

    if (status >= 0)
    {
        if (UT_Stub_CopyToLocal(UT_KEY(SBN_GetPeer), &p, sizeof(p)) < sizeof(p))
        {
            return NULL;
        }
    }

    return p;
}/* end SBN_GetPeer() */
