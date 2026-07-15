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
** File: coveragetest_sbn_udp.c
**
** Purpose:
** Coverage Unit Test cases for the SBN UDP protocol module
**
** Notes:
** This implements various test cases to exercise all code
** paths through all functions defined in the SBN UDP module.
**
** It is primarily focused at providing examples of the various
** stub configurations, hook functions, and wrapper calls that
** are often needed when coercing certain code paths through
** complex functions.
*/

#include "sbn_stubs.h"
#include "sbn_udp_if_coveragetest_common.h"
#include "sbn_udp_if.h"
#include "sbn_app.h"
#include "sbn_error.h"

#define SBN_PROTOCOL_VERSION 6

SBN_AppData_t SBN_AppData;

SBN_NetInterface_t  *NetPtr;
SBN_PeerInterface_t *PeerPtr;
typedef struct
{
    uint16      ExpectedEvent;
    int         MatchCount;
    const char *ExpectedText;
} UT_CheckEvent_t;
UT_CheckEvent_t EventTest;

static bool               g_MockUnpackResult = true;
static SBN_MsgSz_t        g_MockMsgSz        = 0;
static SBN_MsgType_t      g_MockMsgType      = 0;
static CFE_ProcessorID_t  g_MockProcessorID  = 0;
static CFE_SpacecraftID_t g_MockSpacecraftID = 0;

#define EVENT_CNT(C) UtAssert_True(EventTest.MatchCount == (C), "SBN_UDP_SOCK_EID generated (%d)", EventTest.MatchCount)

#define START() START_fn(__func__, __LINE__)

static SBN_Status_t DisconnectCallback(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}

static SBN_Status_t SendNetMsgCallback(SBN_MsgType_t type, SBN_MsgSz_t size, void *raw, SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}

static void PackMsgCallback(void              *buf,
                            SBN_MsgSz_t        size,
                            SBN_MsgType_t      type,
                            CFE_ProcessorID_t  procID,
                            CFE_SpacecraftID_t scID,
                            void              *payload)
{
    return;
}

static void START_fn(const char *fn, int ln)
{
    UT_ResetState(0);
    printf("Start item %s (%d)\n", fn, ln);
    memset(&SBN_AppData, 0, sizeof(SBN_AppData));
    SBN_AppData.NetCnt    = 1;
    NetPtr                = &SBN_AppData.Nets[0];
    PeerPtr               = &NetPtr->Peers[0];
    NetPtr->PeerCnt       = 1;
    PeerPtr->Net          = NetPtr;
    PeerPtr->ProcessorID  = 1;
    PeerPtr->SpacecraftID = 42;
} /* end START_fn() */

extern SBN_IfOps_t SBN_UDP_Ops;

/*
 * An example hook function to check for a specific event.
 */
static int32
UT_CheckEvent_Hook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context, va_list va)
{
    UT_CheckEvent_t *State = UserObj;
    char             TestText[CFE_MISSION_EVS_MAX_MESSAGE_LENGTH];
    uint16           EventId;
    const char      *Spec;

    /*
     * The CFE_EVS_SendEvent stub passes the EventID as the
     * first context argument.
     */
    if (Context->ArgCount > 0)
    {
        EventId = UT_Hook_GetArgValueByName(Context, "EventID", uint16);
        if (EventId == State->ExpectedEvent)
        {
            /*
             * Example of how to validate the full argument set.
             * If reference text was supplied, also check against this.
             *
             * NOTE: While this can be done, use with discretion - This isn't really
             * verifying that the FSW code unit generated the correct event text,
             * rather it is validating what the system snprintf() library function
             * produces when passed the format string and args.
             *
             * __This derived string is not an actual output of the unit under test__
             */
            if (State->ExpectedText != NULL)
            {
                Spec = UT_Hook_GetArgValueByName(Context, "Spec", const char *);
                if (Spec != NULL)
                {
                    vsnprintf(TestText, sizeof(TestText), Spec, va);
                    if (strncmp(TestText, State->ExpectedText, strlen(State->ExpectedText)) == 0)
                    {
                        ++State->MatchCount;
                    }
                }
            }
            else
            {
                ++State->MatchCount;
            } /* end if */
        } /* end if */
    } /* end if */

    return 0;
} /* end UT_CheckEvent_Hook() */

/*
 * Helper function to set up for event checking
 * This attaches the hook function to CFE_EVS_SendEvent
 */
static void UT_CheckEvent_Setup(UT_CheckEvent_t *Evt, uint16 ExpectedEvent, const char *ExpectedText)
{
    memset(Evt, 0, sizeof(*Evt));
    Evt->ExpectedEvent = ExpectedEvent;
    Evt->ExpectedText  = ExpectedText;
    UT_SetVaHookFunction(UT_KEY(CFE_EVS_SendEvent), UT_CheckEvent_Hook, Evt);
} /* end UT_CheckEvent_Setup() */

static void Init_VerErr(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(-1, 0, NULL), SBN_ERROR);
} /* end Init_VerErr() */

static void Init_NullOutlet(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(SBN_PROTOCOL_VERSION, 0, NULL), SBN_ERROR);
} /* end Init_Nominal() */

static void Init_Nominal(void)
{
    SBN_ProtocolOutlet_t Outlet;
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(SBN_PROTOCOL_VERSION, 0, &Outlet), SBN_SUCCESS);
} /* end Init_Nominal() */

void Test_SBN_UDP_Init(void)
{
    Init_VerErr();
    Init_NullOutlet();
    Init_Nominal();
} /* end Test_SBN_UDP_Init() */

static void InitNet_OpenErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketOpen), 1, OS_ERROR);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketBind), 1, OS_SUCCESS);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "socket open call failed");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitNet(NetPtr), SBN_ERROR);

    EVENT_CNT(1);
} /* end InitNet_OpenErr() */

static void InitNet_BindErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketOpen), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketBind), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "bind call failed (NetData=0x");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitNet(NetPtr), SBN_ERROR);

    EVENT_CNT(1);
} /* end InitNet_OpenErr() */

static void InitNet_Nominal(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketOpen), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketBind), 1, OS_SUCCESS);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "creating socket (NetData=0x");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitNet(NetPtr), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end InitNet_Nominal() */

void Test_SBN_UDP_InitNet(void)
{
    InitNet_OpenErr();
    InitNet_BindErr();
    InitNet_Nominal();
} /* end Test_SBN_UDP_InitNet() */

static void Test_SBN_UDP_InitPeer(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitPeer(PeerPtr), SBN_SUCCESS);
} /* end Test_SBN_UDP_InitPeer() */

static void LoadNet_AddrErr(void)
{
    START();

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "invalid address (Address=no colon)");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "no colon"), SBN_ERROR);

    EVENT_CNT(1);
} /* end LoadNet_AddrErr() */

static void LoadNet_AddrPortErr(void)
{
    START();

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "invalid port (Address=foo:bar)");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "foo:bar"), SBN_ERROR);

    EVENT_CNT(1);
} /* end LoadNet_AddrErr() */

static void LoadNet_InitErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "addr init failed (Status=-1)");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_ERROR);

    EVENT_CNT(1);
} /* end LoadNet_InitErr() */

static void LoadNet_HostErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrFromString), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "addr host set failed (AddrHost=");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_ERROR);

    EVENT_CNT(1);
} /* end LoadNet_HostErr() */

static void LoadNet_PortErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrSetPort), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "addr port set failed (Port=");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_ERROR);

    EVENT_CNT(1);
} /* end LoadNet_PortErr() */

static void LoadNet_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "configured (NetData=");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end LoadNet_Nominal() */

void Test_SBN_UDP_LoadNet(void)
{
    LoadNet_AddrErr();
    LoadNet_AddrPortErr();
    LoadNet_InitErr();
    LoadNet_HostErr();
    LoadNet_PortErr();
    LoadNet_Nominal();
} /* end Test_SBN_UDP_LoadNet() */

static void LoadPeer_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "configured peer (");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadPeer(PeerPtr, "localhost:1234"), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end LoadPeer_Nominal() */

void Test_SBN_UDP_LoadPeer(void)
{
    /* LoadPeer() uses the same internal ConfAddr() fn as LoadNet(), so we need not cover errs there */
    LoadPeer_Nominal();
} /* end Test_SBN_UDP_LoadNet() */

static void PollPeer_ConnTimeout(void)
{
    START();
    memset(&SBN_AppData, 0, sizeof(SBN_AppData));
    SBN_AppData.NetCnt = 1;
    SBN_ProtocolOutlet_t Outlet;
    Outlet.Disconnected = DisconnectCallback;
    Outlet.SendNetMsg   = SendNetMsgCallback;

    OS_time_t tm;
    memset(&tm, 0, sizeof(tm));

    PeerPtr->Connected = true;

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_DEBUG_EID, "disconnected peer ");

    tm = OS_TimeFromTotalSeconds(SBN_UDP_PEER_TIMEOUT + 1);
    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    SBN_UDP_Ops.InitModule(SBN_PROTOCOL_VERSION, 0, &Outlet);
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end PollPeer_ConnTimeout() */

static void PollPeer_HeartbeatTimeout(void)
{
    START();

    OS_time_t tm;

    memset(&tm, 0, sizeof(tm));

    PeerPtr->Connected = true;

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_DEBUG_EID, "sending heartbeat to peer ");

    tm = OS_TimeFromTotalSeconds(SBN_UDP_PEER_HEARTBEAT + 1);
    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(SBN_SendNetMsg), 1, SBN_SUCCESS);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end PollPeer_HeartbeatTimeout() */

static void PollPeer_AnnTimeout(void)
{
    START();

    OS_time_t tm;

    memset(&tm, 0, sizeof(tm));

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_DEBUG_EID, "announce to peer ");

    tm = OS_TimeFromTotalSeconds(SBN_UDP_ANNOUNCE_MSG + 1);
    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(SBN_SendNetMsg), 1, SBN_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 2);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end PollPeer_AnnTimeout() */

static void PollPeer_Nominal(void)
{
    START();

    OS_time_t tm;

    memset(&tm, 0, sizeof(tm));

    PeerPtr->Connected = true;

    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(SBN_SendNetMsg), 1, SBN_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, OS_ERROR);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);
} /* end PollPeer_Nominal() */

void Test_SBN_UDP_PollPeer(void)
{
    PollPeer_ConnTimeout();
    PollPeer_HeartbeatTimeout();
    PollPeer_AnnTimeout();
    PollPeer_Nominal();
} /* end Test_SBN_UDP_LoadNet() */

static void Send_AddrInitErr(void)
{
    SBN_ProtocolOutlet_t Outlet;
    Outlet.PackMsg = PackMsgCallback;

    START();

    CFE_MSG_Message_t        *SBMsgPtr;
    CFE_MSG_TelemetryHeader_t TlmPkt;

    SBMsgPtr = (CFE_MSG_Message_t *)&TlmPkt;
    CFE_MSG_Init(SBMsgPtr, CFE_SB_ValueToMsgId(0x1234), sizeof(TlmPkt));

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "socket addr init failed");

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_ERROR);

    SBN_UDP_Ops.InitModule(SBN_PROTOCOL_VERSION, 0, &Outlet);
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.Send(PeerPtr, SBN_APP_MSG, sizeof(TlmPkt), SBMsgPtr), SBN_ERROR);

    EVENT_CNT(1);
} /* end Send_AddrInitErr() */

static void Send_SendErr(void)
{
    START();
    CFE_MSG_Message_t        *SBMsgPtr;
    CFE_MSG_TelemetryHeader_t TlmPkt;

    SBMsgPtr = (CFE_MSG_Message_t *)&TlmPkt;
    CFE_MSG_Init(SBMsgPtr, CFE_SB_ValueToMsgId(0x1234), sizeof(TlmPkt));

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketSendTo), 1, -2);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.Send(PeerPtr, SBN_APP_MSG, sizeof(TlmPkt), SBMsgPtr), SBN_ERROR);
} /* end Send_SendErr() */

static void Send_Nominal(void)
{
    START();
    CFE_MSG_Message_t        *SBMsgPtr;
    CFE_MSG_TelemetryHeader_t TlmPkt;

    SBMsgPtr = (CFE_MSG_Message_t *)&TlmPkt;
    CFE_MSG_Init(SBMsgPtr, CFE_SB_ValueToMsgId(0x1234), sizeof(TlmPkt));

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketSendTo), 1, sizeof(TlmPkt) + SBN_PACKED_HDR_SZ);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.Send(PeerPtr, SBN_APP_MSG, sizeof(TlmPkt), SBMsgPtr), SBN_SUCCESS);
} /* end Send_Nominal() */

void Test_SBN_UDP_Send(void)
{
    Send_AddrInitErr();
    Send_SendErr();
    Send_Nominal();
} /* end Test_SBN_UDP_LoadNet() */

static int32 NoDataHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    *((uint32 *)Context->ArgPtr[1]) = 0;
    return OS_ERROR;
} /* end NoDataHook() */

static void Recv_NoData(void)
{
    START();

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), NoDataHook, NULL);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, NULL, NULL, NULL, NULL, NULL), SBN_IF_EMPTY);
} /* end Recv_NoData() */

static int32 DataHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    *((uint32 *)Context->ArgPtr[0]) = OS_STREAM_STATE_READABLE;
    return OS_SUCCESS;
} /* end DataHook() */

static void Recv_SockRecvErr(void)
{
    START();

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, -1);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, NULL, NULL, NULL, NULL, NULL), SBN_ERROR);
} /* end Recv_SockRecvErr() */

static bool SBN_UnpackMsg_Fail(void               *RecvBuf,
                               SBN_MsgSz_t        *MsgSzPtr,
                               SBN_MsgType_t      *MsgTypePtr,
                               CFE_ProcessorID_t  *ProcessorIDPtr,
                               CFE_SpacecraftID_t *SpacecraftIDPtr,
                               void               *Payload)
{
    return false;
}

static void Recv_UnpackErr(void)
{
    SBN_ProtocolOutlet_t Outlet;
    START();

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 1);
    Outlet.UnpackMsg = SBN_UnpackMsg_Fail;

    SBN_UDP_Ops.InitModule(SBN_PROTOCOL_VERSION, 0, &Outlet);
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, NULL, NULL, NULL, NULL, NULL), SBN_ERROR);
} /* end Recv_UnpackErr() */

static void Recv_GetPeerErr(void)
{
    START();

    SBN_MsgType_t      MsgType;
    SBN_MsgSz_t        MsgSz;
    CFE_ProcessorID_t  ProcessorID;
    CFE_SpacecraftID_t SpacecraftID;
    uint8              PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_Unpack_Buf_t   UnpackBuf;

    UnpackBuf.MsgSz        = 16;
    UnpackBuf.MsgType      = SBN_APP_MSG;
    UnpackBuf.ProcessorID  = PeerPtr->ProcessorID;
    UnpackBuf.SpacecraftID = PeerPtr->SpacecraftID;
    strncpy((char *)UnpackBuf.MsgBuf, "deadbeef", 9);

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 1);
    UT_SetDataBuffer(UT_KEY(SBN_UnpackMsg), &UnpackBuf, sizeof(UnpackBuf), false);
    PeerPtr = NULL;
    UT_SetDataBuffer(UT_KEY(SBN_GetPeer), &PeerPtr, sizeof(PeerPtr), false);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, &SpacecraftID, PayloadBuffer),
                        SBN_ERROR);
} /* end Recv_GetPeerErr() */

static SBN_Status_t MockConnected_TrackCall(SBN_PeerInterface_t *Peer)
{
    Peer->Connected = true; /* Set the peer as connected */
    return SBN_SUCCESS;
}

static SBN_Status_t MockDisconnected_TrackCall(SBN_PeerInterface_t *Peer)
{
    Peer->Connected = false; /* Set the peer as disconnected */
    return SBN_SUCCESS;
}

static SBN_Status_t MockConnected_NoAction(SBN_PeerInterface_t *Peer)
{
    /* For nominal test, connection state shouldn't change */
    return SBN_SUCCESS;
}

static SBN_Status_t MockDisconnected_NoAction(SBN_PeerInterface_t *Peer)
{
    /* For nominal test, connection state shouldn't change */
    return SBN_SUCCESS;
}

static SBN_Status_t MockConnected_Test(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}
static SBN_Status_t MockDisconnected_Test(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}
static SBN_Status_t MockSendNetMsg_Test(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}
static void MockPackMsg_Test(void             *SBNBuf,
                             SBN_MsgSz_t       MsgSz,
                             SBN_MsgType_t     MsgType,
                             CFE_ProcessorID_t ProcessorID,
                             CFE_ProcessorID_t SpacecraftID,
                             void             *Msg)
{
}

/* Mock GetPeer that returns our test peer */
static SBN_PeerInterface_t *
MockGetPeer_NewConn(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID)
{
    if (ProcessorID == PeerPtr->ProcessorID && SpacecraftID == PeerPtr->SpacecraftID)
    {
        return PeerPtr;
    }
    return NULL;
}

/* Mock GetPeer that returns our test peer */
static SBN_PeerInterface_t *
MockGetPeer_Disconnect(SBN_NetInterface_t *Net, CFE_ProcessorID_t ProcessorID, CFE_SpacecraftID_t SpacecraftID)
{
    if (ProcessorID == PeerPtr->ProcessorID && SpacecraftID == PeerPtr->SpacecraftID)
    {
        return PeerPtr;
    }
    return NULL;
}

/* Configurable mock UnpackMsg */
static bool MockUnpackMsg_Configurable(void               *SBNBuf,
                                       SBN_MsgSz_t        *MsgSzPtr,
                                       SBN_MsgType_t      *MsgTypePtr,
                                       CFE_ProcessorID_t  *ProcessorIDPtr,
                                       CFE_SpacecraftID_t *SpacecraftIDPtr,
                                       void               *Msg)
{
    if (g_MockUnpackResult && MsgSzPtr && MsgTypePtr && ProcessorIDPtr && SpacecraftIDPtr)
    {
        *MsgSzPtr        = g_MockMsgSz;
        *MsgTypePtr      = g_MockMsgType;
        *ProcessorIDPtr  = g_MockProcessorID;
        *SpacecraftIDPtr = g_MockSpacecraftID;
        if (Msg && g_MockMsgSz > 0)
        {
            memset(Msg, 0xAB, g_MockMsgSz); /* Fill with test pattern */
        }
    }
    return g_MockUnpackResult;
}

/* Mock UnpackMsg that returns disconnect message data */
static bool MockUnpackMsg_Disconnect(void               *SBNBuf,
                                     SBN_MsgSz_t        *MsgSzPtr,
                                     SBN_MsgType_t      *MsgTypePtr,
                                     CFE_ProcessorID_t  *ProcessorIDPtr,
                                     CFE_SpacecraftID_t *SpacecraftIDPtr,
                                     void               *Msg)
{
    *MsgSzPtr        = 16;
    *MsgTypePtr      = SBN_UDP_DISCONN_MSG; /* Return disconnect message type */
    *ProcessorIDPtr  = PeerPtr->ProcessorID;
    *SpacecraftIDPtr = PeerPtr->SpacecraftID;
    memcpy(Msg, "deadbeef", 8);
    return true;
}

static bool MockUnpackMsg_Nominal(void               *SBNBuf,
                                  SBN_MsgSz_t        *MsgSzPtr,
                                  SBN_MsgType_t      *MsgTypePtr,
                                  CFE_ProcessorID_t  *ProcessorIDPtr,
                                  CFE_SpacecraftID_t *SpacecraftIDPtr,
                                  void               *Msg)
{
    *MsgSzPtr        = 16;
    *MsgTypePtr      = SBN_APP_MSG; /* Regular app message */
    *ProcessorIDPtr  = PeerPtr->ProcessorID;
    *SpacecraftIDPtr = PeerPtr->SpacecraftID;
    memcpy(Msg, "deadbeef", 8);
    return true;
}

static void Recv_NewConn(void)
{
    START();

    SBN_ProtocolOutlet_t Outlet;
    SBN_MsgType_t        MsgType;
    SBN_MsgSz_t          MsgSz;
    CFE_ProcessorID_t    ProcessorID;
    CFE_SpacecraftID_t   SpacecraftID;
    uint8                PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];

    /* Configure the mock behavior */
    g_MockUnpackResult = true;
    g_MockMsgSz        = 16;
    g_MockMsgType      = SBN_APP_MSG;
    g_MockProcessorID  = PeerPtr->ProcessorID;
    g_MockSpacecraftID = PeerPtr->SpacecraftID;

    /* Set up the outlet */
    memset(&Outlet, 0, sizeof(Outlet));
    Outlet.Connected    = MockConnected_TrackCall;
    Outlet.Disconnected = MockDisconnected_Test;
    Outlet.SendNetMsg   = MockSendNetMsg_Test;
    Outlet.PackMsg      = MockPackMsg_Test;
    Outlet.UnpackMsg    = MockUnpackMsg_Configurable;
    Outlet.GetPeer      = MockGetPeer_NewConn;

    /* Initialize the module */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(6, 0, &Outlet), SBN_SUCCESS);

    /* Set up initial state */
    PeerPtr->Connected = false;
    NetPtr->TaskFlags  = 0;

    /* Set up stubs */
    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 20);

    /* Test the function */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, &SpacecraftID, PayloadBuffer),
                        SBN_SUCCESS);

    /* Verify results */
    UtAssert_True(PeerPtr->Connected == true, "Peer should be connected");
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(MsgSz, 16);
    UtAssert_INT32_EQ(ProcessorID, PeerPtr->ProcessorID);
    UtAssert_INT32_EQ(SpacecraftID, PeerPtr->SpacecraftID);
} /* end Recv_NewConn() */

/* Hook for OS_SelectSingle to make socket readable */
static int32 DataHook_Disconnect(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    uint32 *StateFlags = UT_Hook_GetArgValueByName(Context, "StateFlags", uint32 *);
    if (StateFlags != NULL)
    {
        *StateFlags = OS_STREAM_STATE_READABLE;
    }
    return OS_SUCCESS;
}

static SBN_Status_t DisconnectCallback_TrackCalled(SBN_PeerInterface_t *Peer)
{
    /* Mark peer as disconnected when called */
    Peer->Connected = false;
    return SBN_SUCCESS;
}

static SBN_Status_t
SendNetMsgCallback_CheckDisconnMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer)
{
    /* Verify a disconnect message was sent */
    UtAssert_INT32_EQ(MsgType, SBN_UDP_DISCONN_MSG);
    return SBN_SUCCESS;
}

static void Recv_Disconn(void)
{
    START();

    SBN_ProtocolOutlet_t Outlet;
    SBN_MsgType_t        MsgType;
    SBN_MsgSz_t          MsgSz;
    CFE_ProcessorID_t    ProcessorID;
    CFE_SpacecraftID_t   SpacecraftID;
    uint8                PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];

    /* Set up the outlet with all required function pointers */
    memset(&Outlet, 0, sizeof(Outlet));
    Outlet.Connected    = MockConnected_Test;
    Outlet.Disconnected = MockDisconnected_TrackCall; /* This will set Connected = false */
    Outlet.SendNetMsg   = MockSendNetMsg_Test;
    Outlet.PackMsg      = MockPackMsg_Test;
    Outlet.UnpackMsg    = MockUnpackMsg_Disconnect; /* Returns disconnect message */
    Outlet.GetPeer      = MockGetPeer_Disconnect;

    /* Initialize the UDP module */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(6, 0, &Outlet), SBN_SUCCESS);

    /* Set up the peer as initially connected */
    PeerPtr->Connected = true;

    /* Set up the network to not use task-based receiving */
    NetPtr->TaskFlags = 0;

    /* Set up OS function stubs */
    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook_Disconnect, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 20); /* Received some bytes */

    /* Call the function under test */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, &SpacecraftID, PayloadBuffer),
                        SBN_SUCCESS);

    /* Verify the results */
    UtAssert_True(PeerPtr->Connected == false, "Peer should be disconnected after receiving disconnect message");
    UtAssert_INT32_EQ(MsgType, SBN_UDP_DISCONN_MSG);
    UtAssert_INT32_EQ(MsgSz, 16);
    UtAssert_INT32_EQ(ProcessorID, PeerPtr->ProcessorID);
    UtAssert_INT32_EQ(SpacecraftID, PeerPtr->SpacecraftID);
} /* end Recv_NewConn() */

static void Recv_Nominal(void)
{
    START();

    SBN_ProtocolOutlet_t Outlet;
    SBN_MsgType_t        MsgType;
    SBN_MsgSz_t          MsgSz;
    CFE_ProcessorID_t    ProcessorID;
    CFE_SpacecraftID_t   SpacecraftID;
    uint8                PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];

    /* Set up the outlet with all required function pointers */
    memset(&Outlet, 0, sizeof(Outlet));
    Outlet.Connected    = MockConnected_NoAction;    /* No change to connection state */
    Outlet.Disconnected = MockDisconnected_NoAction; /* No change to connection state */
    Outlet.SendNetMsg   = MockSendNetMsg_Test;       /* Reuse from previous tests */
    Outlet.PackMsg      = MockPackMsg_Test;          /* Reuse from previous tests */
    Outlet.UnpackMsg    = MockUnpackMsg_Nominal;     /* Returns regular app message */
    Outlet.GetPeer      = MockGetPeer_NewConn;       /* Reuse from previous tests */

    /* Initialize the UDP module */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(6, 0, &Outlet), SBN_SUCCESS);

    /* Set up the peer as already connected */
    PeerPtr->Connected = true;

    /* Set up the network to not use task-based receiving */
    NetPtr->TaskFlags = 0;

    /* Set up OS function stubs */
    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 20); /* Received some bytes */

    /* Call the function under test */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, &SpacecraftID, PayloadBuffer),
                        SBN_SUCCESS);

    /* Verify the results - peer should still be connected */
    UtAssert_True(PeerPtr->Connected == true, "Peer should remain connected");
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(MsgSz, 16);
    UtAssert_INT32_EQ(ProcessorID, PeerPtr->ProcessorID);
    UtAssert_INT32_EQ(SpacecraftID, PeerPtr->SpacecraftID);
} /* end Recv_Nominal() */

void Test_SBN_UDP_Recv(void)
{
    Recv_NoData();
    Recv_SockRecvErr();
    Recv_UnpackErr();
    Recv_GetPeerErr();
    Recv_NewConn();
    Recv_Disconn();
    Recv_Nominal();
} /* end Test_SBN_UDP_Recv() */

static void UnloadPeer_Disconn(void)
{
    START();

    /* Set up outlet with our test callbacks */
    SBN_ProtocolOutlet_t Outlet;
    memset(&Outlet, 0, sizeof(Outlet));
    Outlet.Disconnected = DisconnectCallback_TrackCalled;     /* Will set Connected = false */
    Outlet.SendNetMsg   = SendNetMsgCallback_CheckDisconnMsg; /* Verifies disconnect message sent */

    /* Add required functions to avoid crashes */
    Outlet.Connected = MockConnected_NoAction;
    Outlet.PackMsg   = MockPackMsg_Test;
    Outlet.UnpackMsg = MockUnpackMsg_Nominal;
    Outlet.GetPeer   = MockGetPeer_NewConn;

    /* Set up initial state - peer starts connected */
    PeerPtr->Connected = true;

    /* Initialize the module with correct version */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(6, 0, &Outlet), SBN_SUCCESS);

    /* Call the function under test */
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.UnloadPeer(PeerPtr), SBN_SUCCESS);

    /* Verify the peer is now disconnected */
    UtAssert_True(PeerPtr->Connected == false, "Peer should be disconnected after UnloadPeer");
} /* end UnloadPeer_Disconn() */

static void UnloadPeer_Nominal(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.UnloadPeer(PeerPtr), SBN_SUCCESS);

    UtAssert_True(PeerPtr->Connected == false, "Peer connected (%s)", __func__);
} /* end UnloadPeer_Nominal() */

void Test_SBN_UDP_UnloadPeer(void)
{
    UnloadPeer_Disconn();
    UnloadPeer_Nominal();
} /* end Test_SBN_UDP_UnloadPeer() */

static void UnloadNet_Nominal(void)
{
    START();

    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)&(NetPtr->ModulePvt);

    PeerPtr->Connected = true;

    OS_OpenCreate(&(NetData->Socket), NULL, 0, 0);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.UnloadNet(NetPtr), SBN_SUCCESS);

    /* TODO: check what was called? */
    UtAssert_True(PeerPtr->Connected == false, "Peer still connected (%s)", __func__);
} /* end UnloadNet_Nominal() */

void Test_SBN_UDP_UnloadNet(void)
{
    UnloadNet_Nominal();
} /* end Test_SBN_UDP_UnloadPeer() */

/*
 * Setup function prior to every test
 */
void UT_Setup(void)
{
    UT_ResetState(0);
}

/*
 * Teardown function after every test
 */
void UT_TearDown(void)
{
}

void UtTest_Setup(void)
{
    ADD_TEST(SBN_UDP_Init);
    ADD_TEST(SBN_UDP_InitNet);
    ADD_TEST(SBN_UDP_InitPeer);
    ADD_TEST(SBN_UDP_LoadNet);
    ADD_TEST(SBN_UDP_LoadPeer);
    ADD_TEST(SBN_UDP_PollPeer);
    ADD_TEST(SBN_UDP_Send);
    ADD_TEST(SBN_UDP_Recv);
    ADD_TEST(SBN_UDP_UnloadPeer);
    ADD_TEST(SBN_UDP_UnloadNet);
}
