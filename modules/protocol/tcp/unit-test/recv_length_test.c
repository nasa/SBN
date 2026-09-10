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

#include "utassert.h"
#include "uttest.h"
#include "utstubs.h"
#include <stdint.h>

/* Include the real receive state machine and its private connection state. */
#include "../fsw/src/sbn_tcp_if.c"

static SBN_NetInterface_t Net;
static SBN_TCP_Conn_t    *Conn;
static uint8              Wire[SBN_MAX_PACKED_MSG_SZ];
static uint8              Output[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
static size_t             WireSize, WireOffset, ReadChunk;
static SBN_MsgSz_t        DeclaredSize;
static unsigned           BodyReads, UnpackCalls, DisconnectCalls;
static bool               UnpackResult;

static SBN_Status_t PeerDisconnected(SBN_PeerInterface_t *Peer)
{
    ++DisconnectCalls;
    return SBN_SUCCESS;
}

static bool Unpack(void               *Buffer,
                   SBN_MsgSz_t        *Size,
                   SBN_MsgType_t      *Type,
                   CFE_ProcessorID_t  *Processor,
                   CFE_SpacecraftID_t *Spacecraft,
                   void               *Payload)
{
    ++UnpackCalls;
    UtAssert_True(memcmp(Buffer, Wire, WireSize) == 0, "Complete frame passed to unpack");
    *Size = DeclaredSize;
    return UnpackResult;
}

static void ReadHandler(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    void  *Buffer    = UT_Hook_GetArgValueByName(Context, "buffer", void *);
    size_t Requested = UT_Hook_GetArgValueByName(Context, "nbytes", size_t);
    size_t Offset    = (uint8 *)Buffer - RecvBufs[Conn->BufNum];
    size_t Count;
    int32  Result;

    if (WireOffset >= SBN_PACKED_HDR_SZ)
    {
        ++BodyReads;
    }
    /* Catch an unsafe request without actually writing outside the buffer. */
    UtAssert_True(Offset <= SBN_MAX_PACKED_MSG_SZ && Requested <= SBN_MAX_PACKED_MSG_SZ - Offset,
                  "Read stays inside this connection's receive buffer");
    if (Offset > SBN_MAX_PACKED_MSG_SZ || Requested > SBN_MAX_PACKED_MSG_SZ - Offset)
    {
        Result = OS_ERROR;
        UT_Stub_SetReturnValue(FuncKey, Result);
        return;
    }
    Count = WireSize - WireOffset;
    if (Count > Requested)
    {
        Count = Requested;
    }
    if (Count > ReadChunk)
    {
        Count = ReadChunk;
    }
    memcpy(Buffer, Wire + WireOffset, Count);
    WireOffset += Count;
    Result      = (int32)Count;
    UT_Stub_SetReturnValue(FuncKey, Result);
}

static void Prepare(SBN_MsgSz_t Size, bool KnownPeer)
{
    SBN_TCP_Net_t *NetData;
    SBN_MsgSz_t    EncodedSize = CFE_MAKE_BIG32(Size);

    UT_ResetState(0);
    memset(&Net, 0, sizeof(Net));
    memset(&SBN, 0, sizeof(SBN));
    memset(RecvBufs, 0, sizeof(RecvBufs));
    memset(Wire, 0x5a, sizeof(Wire));
    memcpy(Wire, &EncodedSize, sizeof(EncodedSize));
    DeclaredSize = Size;
    WireSize     = SBN_PACKED_HDR_SZ;
    if (Size <= CFE_MISSION_SB_MAX_SB_MSG_SIZE)
    {
        WireSize += Size;
    }
    WireOffset = 0;
    ReadChunk  = sizeof(Wire);
    BodyReads = UnpackCalls = DisconnectCalls = 0;
    UnpackResult                              = true;
    SendBufCnt                                = 0;
    RecvBufCnt                                = 0;
    UtAssert_INT32_EQ(LoadNet(&Net, "127.0.0.1:1234"), SBN_SUCCESS);
    NetData     = GetNetData(&Net);
    Conn        = &NetData->Conns[0];
    Conn->InUse = true;
    OS_SocketOpen(&Conn->Socket, OS_SocketDomain_INET, OS_SocketType_STREAM);
    SBN.UnpackMsg    = Unpack;
    SBN.Disconnected = PeerDisconnected;
    if (KnownPeer)
    {
        SBN_PeerInterface_t *Peer = &Net.Peers[0];
        SBN_TCP_Peer_t      *PeerData;
        UtAssert_INT32_EQ(LoadPeer(Peer, "127.0.0.1:1235"), SBN_SUCCESS);
        PeerData            = GetPeerData(Peer);
        PeerData->Conn      = Conn;
        Conn->PeerInterface = Peer;
    }
    UT_SetDefaultReturnValue(UT_KEY(OS_SelectMultiple), OS_SUCCESS);
    UT_SetDefaultReturnValue(UT_KEY(OS_SelectFdIsSet), true);
    UT_SetHandlerFunction(UT_KEY(OS_read), ReadHandler, NULL);
}

static SBN_Status_t Receive(void)
{
    SBN_MsgType_t      Type;
    SBN_MsgSz_t        Size;
    CFE_ProcessorID_t  Processor;
    CFE_SpacecraftID_t Spacecraft;
    return Recv(&Net, &Type, &Size, &Processor, &Spacecraft, Output);
}

static void CheckClosed(bool KnownPeer)
{
    UtAssert_True(!Conn->InUse, "Connection slot released");
    UtAssert_True(!Conn->ReceivingBody && Conn->RecvSz == 0, "Receive state reset");
    UtAssert_True(Conn->PeerInterface == NULL, "Peer association cleared");
    UtAssert_UINT32_EQ(UT_GetStubCount(UT_KEY(OS_close)), 1);
    UtAssert_UINT32_EQ(DisconnectCalls, KnownPeer ? 1 : 0);
    if (KnownPeer)
    {
        SBN_TCP_Peer_t *PeerData = GetPeerData(&Net.Peers[0]);
        UtAssert_True(PeerData->Conn == NULL, "Peer no longer holds the closed connection");
    }
}

static void RejectOversized(void)
{
    const SBN_MsgSz_t Sizes[] = { CFE_MISSION_SB_MAX_SB_MSG_SIZE + 1, UINT32_MAX };
    unsigned          i, Known;
    for (i = 0; i < sizeof(Sizes) / sizeof(Sizes[0]); ++i)
    {
        for (Known = 0; Known < 2; ++Known)
        {
            Prepare(Sizes[i], Known != 0);
            UtAssert_INT32_EQ(Receive(), SBN_ERROR);
            UtAssert_UINT32_EQ(BodyReads, 0);
            UtAssert_UINT32_EQ(UnpackCalls, 0);
            CheckClosed(Known != 0);
        }
    }
}

static void RejectAfterPartialHeader(void)
{
    Prepare(CFE_MISSION_SB_MAX_SB_MSG_SIZE + 1, false);
    ReadChunk = 3;
    while (WireOffset + ReadChunk < SBN_PACKED_HDR_SZ)
    {
        UtAssert_INT32_EQ(Receive(), SBN_IF_EMPTY);
        UtAssert_UINT32_EQ(BodyReads, 0);
        UtAssert_True(Conn->InUse, "Partial header retains connection");
    }
    UtAssert_INT32_EQ(Receive(), SBN_ERROR);
    UtAssert_UINT32_EQ(BodyReads, 0);
    UtAssert_UINT32_EQ(UnpackCalls, 0);
    CheckClosed(false);
}

static void RejectInconsistentBodyProgress(void)
{
    const int Progress[] = { SBN_PACKED_HDR_SZ - 1, SBN_PACKED_HDR_SZ + 2 };
    unsigned  i;
    for (i = 0; i < sizeof(Progress) / sizeof(Progress[0]); ++i)
    {
        Prepare(1, false);
        memcpy(RecvBufs[Conn->BufNum], Wire, SBN_PACKED_HDR_SZ);
        Conn->ReceivingBody = true;
        Conn->RecvSz        = Progress[i];
        UtAssert_INT32_EQ(Receive(), SBN_ERROR);
        UtAssert_UINT32_EQ(UT_GetStubCount(UT_KEY(OS_read)), 0);
        UtAssert_UINT32_EQ(UnpackCalls, 0);
        CheckClosed(false);
    }
}

static void AcceptBoundarySizes(void)
{
    const SBN_MsgSz_t Sizes[] = { 0, 1, CFE_MISSION_SB_MAX_SB_MSG_SIZE };
    unsigned          i;
    for (i = 0; i < sizeof(Sizes) / sizeof(Sizes[0]); ++i)
    {
        Prepare(Sizes[i], false);
        /* The next row need not align its size field to a uint32 boundary. */
        Conn->BufNum = 1;
        UtAssert_INT32_EQ(Receive(), SBN_SUCCESS);
        UtAssert_UINT32_EQ(UnpackCalls, 1);
        UtAssert_UINT32_EQ(BodyReads, Sizes[i] ? 1 : 0);
        UtAssert_True(Conn->InUse, "Valid message keeps connection open");
        UtAssert_True(!Conn->ReceivingBody && Conn->RecvSz == 0, "Ready for next frame");
        UtAssert_UINT32_EQ(UT_GetStubCount(UT_KEY(OS_close)), 0);
    }
}

static void AcceptPartialBody(void)
{
    Prepare(CFE_MISSION_SB_MAX_SB_MSG_SIZE, false);
    ReadChunk = CFE_MISSION_SB_MAX_SB_MSG_SIZE / 2;
    UtAssert_INT32_EQ(Receive(), SBN_IF_EMPTY);
    UtAssert_UINT32_EQ(UnpackCalls, 0);
    UtAssert_True(Conn->ReceivingBody, "Incomplete body retained");
    UtAssert_INT32_EQ(Receive(), SBN_SUCCESS);
    UtAssert_UINT32_EQ(UnpackCalls, 1);
    UtAssert_UINT32_EQ(BodyReads, 2);
    /* A following heartbeat must not reuse the previous message size. */
    memset(Wire, 0, SBN_PACKED_HDR_SZ);
    DeclaredSize = 0;
    WireOffset   = 0;
    WireSize     = SBN_PACKED_HDR_SZ;
    UtAssert_INT32_EQ(Receive(), SBN_SUCCESS);
    UtAssert_UINT32_EQ(UnpackCalls, 2);
}

static void CloseOnEOF(void)
{
    unsigned Known;
    for (Known = 0; Known < 2; ++Known)
    {
        Prepare(1, Known != 0);
        WireSize = 0;
        UtAssert_INT32_EQ(Receive(), SBN_IF_EMPTY);
        CheckClosed(Known != 0);
    }
}

static void CloseOnBodyEOF(void)
{
    unsigned Known;
    for (Known = 0; Known < 2; ++Known)
    {
        Prepare(1, Known != 0);
        WireSize = SBN_PACKED_HDR_SZ;
        UtAssert_INT32_EQ(Receive(), SBN_ERROR);
        UtAssert_UINT32_EQ(UnpackCalls, 0);
        CheckClosed(Known != 0);
    }
}

static void CloseOnUnpackFailure(void)
{
    unsigned Known;
    for (Known = 0; Known < 2; ++Known)
    {
        Prepare(1, Known != 0);
        UnpackResult = false;
        UtAssert_INT32_EQ(Receive(), SBN_ERROR);
        UtAssert_UINT32_EQ(UnpackCalls, 1);
        CheckClosed(Known != 0);
    }
}

static void PrivateStorage(void)
{
    static SBN_NetInterface_t OtherNet;
    SBN_TCP_Net_t            *First;
    SBN_TCP_Net_t            *Second;
    SBN_TCP_Peer_t           *PeerData;
    const uint8               Empty[sizeof(Net.ModulePvt)] = { 0 };

    Prepare(0, true);
    First    = GetNetData(&Net);
    PeerData = GetPeerData(&Net.Peers[0]);
    UtAssert_True(First == &NetStates[0], "Network state is stored outside the opaque bytes");
    UtAssert_True(PeerData == &PeerStates[0], "Peer state is stored outside the opaque bytes");
    First->Conns[SBN_MAX_PEER_CNT - 1].RecvSz = 123;
    UtAssert_True(memcmp((uint8 *)Net.ModulePvt + sizeof(First), Empty, sizeof(Net.ModulePvt) - sizeof(First)) == 0,
                  "Connection table writes leave opaque interface bytes untouched");
    memset(&OtherNet, 0, sizeof(OtherNet));
    UtAssert_INT32_EQ(LoadNet(&OtherNet, "127.0.0.1:1236"), SBN_SUCCESS);
    Second = GetNetData(&OtherNet);
    UtAssert_True(Second != First, "Networks have independent connection tables");
    UtAssert_INT32_EQ(First->Conns[SBN_MAX_PEER_CNT - 1].RecvSz, 123);
    UtAssert_INT32_EQ(Second->Conns[SBN_MAX_PEER_CNT - 1].RecvSz, 0);
}

static void RejectConfigurationOverflow(void)
{
    static SBN_NetInterface_t Unconfigured;
    unsigned                  i;

    UT_ResetState(0);
    SendBufCnt = 0;
    RecvBufCnt = 0;
    memset(&Net, 0, sizeof(Net));
    memset(&Unconfigured, 0, sizeof(Unconfigured));
    for (i = 0; i < SBN_MAX_NETS; ++i)
    {
        UtAssert_INT32_EQ(LoadNet(&Net, "127.0.0.1:1234"), SBN_SUCCESS);
    }
    UtAssert_INT32_EQ(LoadNet(&Unconfigured, "127.0.0.1:1234"), SBN_ERROR);
    UtAssert_True(GetNetData(&Unconfigured) == NULL, "Rejected network remains unconfigured");
    UtAssert_INT32_EQ(InitNet(&Unconfigured), SBN_ERROR);
    for (i = 0; i < SBN_MAX_PEER_CNT; ++i)
    {
        UtAssert_INT32_EQ(LoadPeer(&Net.Peers[i], "127.0.0.1:1235"), SBN_SUCCESS);
    }
    UtAssert_INT32_EQ(LoadPeer(&Unconfigured.Peers[0], "127.0.0.1:1235"), SBN_ERROR);
    UtAssert_True(GetPeerData(&Unconfigured.Peers[0]) == NULL, "Rejected peer remains unconfigured");
    UtAssert_INT32_EQ(InitPeer(&Unconfigured.Peers[0]), SBN_ERROR);
}

void UtTest_Setup(void)
{
    UtTest_Add(PrivateStorage, NULL, NULL, "Keep typed protocol state in bounded aligned storage");
    UtTest_Add(RejectConfigurationOverflow, NULL, NULL, "Reject exhausted protocol storage");
    UtTest_Add(RejectOversized, NULL, NULL, "Reject oversized lengths before body reads");
    UtTest_Add(RejectAfterPartialHeader, NULL, NULL, "Validate fragmented headers");
    UtTest_Add(RejectInconsistentBodyProgress, NULL, NULL, "Reject inconsistent partial-body offsets");
    UtTest_Add(AcceptBoundarySizes, NULL, NULL, "Accept zero, small, and maximum payloads");
    UtTest_Add(AcceptPartialBody, NULL, NULL, "Retain partial bodies and receive the next frame");
    UtTest_Add(CloseOnEOF, NULL, NULL, "Close known and unidentified peers on EOF");
    UtTest_Add(CloseOnBodyEOF, NULL, NULL, "Close truncated bodies");
    UtTest_Add(CloseOnUnpackFailure, NULL, NULL, "Close frames rejected by unpack");
}
