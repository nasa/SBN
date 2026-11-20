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
 * Auto-Generated stub implementations for functions defined in sbn_app header
 */

#include "sbn_app.h"
#include "utgenstub.h"

void UT_DefaultHandler_SBN_GetPeer(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_SBN_UnpackMsg(void *, UT_EntryKey_t, const UT_StubContext_t *);

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_AppMain()
 * ----------------------------------------------------
 */
void SBN_AppMain(void)
{
    UT_GenStub_Execute(SBN_AppMain, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_Cleanup()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_Cleanup(void)
{
    UT_GenStub_SetupReturnBuffer(SBN_Cleanup, SBN_Status_t);

    UT_GenStub_Execute(SBN_Cleanup, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_Cleanup, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_FindActiveFilter()
 * ----------------------------------------------------
 */
const SBN_FilterModInfo_t *SBN_FindActiveFilter(const char *FilterName)
{
    UT_GenStub_SetupReturnBuffer(SBN_FindActiveFilter, const SBN_FilterModInfo_t *);

    UT_GenStub_AddParam(SBN_FindActiveFilter, const char *, FilterName);

    UT_GenStub_Execute(SBN_FindActiveFilter, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_FindActiveFilter, const SBN_FilterModInfo_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_FindActiveProtocol()
 * ----------------------------------------------------
 */
const SBN_ProtocolModInfo_t *SBN_FindActiveProtocol(const char *ProtocolName)
{
    UT_GenStub_SetupReturnBuffer(SBN_FindActiveProtocol, const SBN_ProtocolModInfo_t *);

    UT_GenStub_AddParam(SBN_FindActiveProtocol, const char *, ProtocolName);

    UT_GenStub_Execute(SBN_FindActiveProtocol, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_FindActiveProtocol, const SBN_ProtocolModInfo_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_GetPeer()
 * ----------------------------------------------------
 */
SBN_PeerInterface_t *
SBN_GetPeer(SBN_NetInterface_t *Net, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID)
{
    UT_GenStub_SetupReturnBuffer(SBN_GetPeer, SBN_PeerInterface_t *);

    UT_GenStub_AddParam(SBN_GetPeer, SBN_NetInterface_t *, Net);
    UT_GenStub_AddParam(SBN_GetPeer, SBN_ProcessorID_t, ProcessorID);
    UT_GenStub_AddParam(SBN_GetPeer, SBN_SpacecraftID_t, SpacecraftID);

    UT_GenStub_Execute(SBN_GetPeer, Basic, UT_DefaultHandler_SBN_GetPeer);

    return UT_GenStub_GetReturnValue(SBN_GetPeer, SBN_PeerInterface_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_GetPeerRecord()
 * ----------------------------------------------------
 */
SBN_PeerRecord_t *
SBN_GetPeerRecord(SBN_NetRecord_t *NetRec, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID)
{
    UT_GenStub_SetupReturnBuffer(SBN_GetPeerRecord, SBN_PeerRecord_t *);

    UT_GenStub_AddParam(SBN_GetPeerRecord, SBN_NetRecord_t *, NetRec);
    UT_GenStub_AddParam(SBN_GetPeerRecord, SBN_ProcessorID_t, ProcessorID);
    UT_GenStub_AddParam(SBN_GetPeerRecord, SBN_SpacecraftID_t, SpacecraftID);

    UT_GenStub_Execute(SBN_GetPeerRecord, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_GetPeerRecord, SBN_PeerRecord_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_InitInterfaces()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_InitInterfaces(void)
{
    UT_GenStub_SetupReturnBuffer(SBN_InitInterfaces, SBN_Status_t);

    UT_GenStub_Execute(SBN_InitInterfaces, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_InitInterfaces, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_InitializeCounters()
 * ----------------------------------------------------
 */
void SBN_InitializeCounters(void)
{
    UT_GenStub_Execute(SBN_InitializeCounters, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_InitializePeerCounters()
 * ----------------------------------------------------
 */
void SBN_InitializePeerCounters(SBN_PeerRecord_t *PeerRec)
{
    UT_GenStub_AddParam(SBN_InitializePeerCounters, SBN_PeerRecord_t *, PeerRec);

    UT_GenStub_Execute(SBN_InitializePeerCounters, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_PackMsg()
 * ----------------------------------------------------
 */
void SBN_PackMsg(void              *SBNBuf,
                 SBN_MsgSz_t        MsgSz,
                 SBN_MsgType_t      MsgType,
                 SBN_ProcessorID_t  ProcessorID,
                 SBN_SpacecraftID_t SpacecraftID,
                 void              *Msg)
{
    UT_GenStub_AddParam(SBN_PackMsg, void *, SBNBuf);
    UT_GenStub_AddParam(SBN_PackMsg, SBN_MsgSz_t, MsgSz);
    UT_GenStub_AddParam(SBN_PackMsg, SBN_MsgType_t, MsgType);
    UT_GenStub_AddParam(SBN_PackMsg, SBN_ProcessorID_t, ProcessorID);
    UT_GenStub_AddParam(SBN_PackMsg, SBN_SpacecraftID_t, SpacecraftID);
    UT_GenStub_AddParam(SBN_PackMsg, void *, Msg);

    UT_GenStub_Execute(SBN_PackMsg, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_ReloadConfTbl()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_ReloadConfTbl(void)
{
    UT_GenStub_SetupReturnBuffer(SBN_ReloadConfTbl, SBN_Status_t);

    UT_GenStub_Execute(SBN_ReloadConfTbl, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_ReloadConfTbl, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_SendNetMsg()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer)
{
    UT_GenStub_SetupReturnBuffer(SBN_SendNetMsg, SBN_Status_t);

    UT_GenStub_AddParam(SBN_SendNetMsg, SBN_MsgType_t, MsgType);
    UT_GenStub_AddParam(SBN_SendNetMsg, SBN_MsgSz_t, MsgSz);
    UT_GenStub_AddParam(SBN_SendNetMsg, void *, Msg);
    UT_GenStub_AddParam(SBN_SendNetMsg, SBN_PeerInterface_t *, Peer);

    UT_GenStub_Execute(SBN_SendNetMsg, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_SendNetMsg, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_UnpackMsg()
 * ----------------------------------------------------
 */
bool SBN_UnpackMsg(void               *SBNBuf,
                   SBN_MsgSz_t        *MsgSzPtr,
                   SBN_MsgType_t      *MsgTypePtr,
                   SBN_ProcessorID_t  *ProcessorIDPtr,
                   SBN_SpacecraftID_t *SpacecraftIDPtr,
                   void               *Msg)
{
    UT_GenStub_SetupReturnBuffer(SBN_UnpackMsg, bool);

    UT_GenStub_AddParam(SBN_UnpackMsg, void *, SBNBuf);
    UT_GenStub_AddParam(SBN_UnpackMsg, SBN_MsgSz_t *, MsgSzPtr);
    UT_GenStub_AddParam(SBN_UnpackMsg, SBN_MsgType_t *, MsgTypePtr);
    UT_GenStub_AddParam(SBN_UnpackMsg, SBN_ProcessorID_t *, ProcessorIDPtr);
    UT_GenStub_AddParam(SBN_UnpackMsg, SBN_SpacecraftID_t *, SpacecraftIDPtr);
    UT_GenStub_AddParam(SBN_UnpackMsg, void *, Msg);

    UT_GenStub_Execute(SBN_UnpackMsg, Basic, UT_DefaultHandler_SBN_UnpackMsg);

    return UT_GenStub_GetReturnValue(SBN_UnpackMsg, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_ValidateConfTbl()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_ValidateConfTbl(void *TableData)
{
    UT_GenStub_SetupReturnBuffer(SBN_ValidateConfTbl, SBN_Status_t);

    UT_GenStub_AddParam(SBN_ValidateConfTbl, void *, TableData);

    UT_GenStub_Execute(SBN_ValidateConfTbl, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_ValidateConfTbl, SBN_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_WaitForWakeup()
 * ----------------------------------------------------
 */
SBN_Status_t SBN_WaitForWakeup(int32 iTimeOut)
{
    UT_GenStub_SetupReturnBuffer(SBN_WaitForWakeup, SBN_Status_t);

    UT_GenStub_AddParam(SBN_WaitForWakeup, int32, iTimeOut);

    UT_GenStub_Execute(SBN_WaitForWakeup, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_WaitForWakeup, SBN_Status_t);
}
