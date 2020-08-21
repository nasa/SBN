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

SBN_App_t SBN;

SBN_NetInterface_t *NetPtr;
SBN_PeerInterface_t *PeerPtr;
typedef struct
{
    uint16 ExpectedEvent;
    int MatchCount;
    const char *ExpectedText;
} UT_CheckEvent_t;
UT_CheckEvent_t EventTest;

#define EVENT_CNT(C) UtAssert_True(EventTest.MatchCount == (C), "SBN_UDP_SOCK_EID generated (%d)", EventTest.MatchCount)

#define START() START_fn(__func__,__LINE__)

static void START_fn(const char *fn, int ln)
{
    UT_ResetState(0);
    printf("Start item %s (%d)\n", fn, ln);
    memset(&SBN, 0, sizeof(SBN));
    SBN.NetCnt = 1;
    NetPtr = &SBN.Nets[0]; PeerPtr = &NetPtr->Peers[0];
    NetPtr->PeerCnt = 1;
    PeerPtr->Net = NetPtr;
    PeerPtr->ProcessorID = 1;
    PeerPtr->SpacecraftID = 42;
}/* end START_fn() */

extern SBN_IfOps_t SBN_UDP_Ops;

/*
 * An example hook function to check for a specific event.
 */
static int32 UT_CheckEvent_Hook(void *UserObj, int32 StubRetcode,
        uint32 CallCount, const UT_StubContext_t *Context, va_list va)
{
    UT_CheckEvent_t *State = UserObj;
    char TestText[CFE_MISSION_EVS_MAX_MESSAGE_LENGTH];
    uint16 EventId;
    const char *Spec;

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
                    if (strncmp(TestText,State->ExpectedText,strlen(State->ExpectedText)) == 0)
                    {
                        ++State->MatchCount;
                    }
                }
            }
            else
            {
                ++State->MatchCount;
            }/* end if */
        }/* end if */
    }/* end if */

    return 0;
}/* end UT_CheckEvent_Hook() */

/*
 * Helper function to set up for event checking
 * This attaches the hook function to CFE_EVS_SendEvent
 */
static void UT_CheckEvent_Setup(UT_CheckEvent_t *Evt, uint16 ExpectedEvent, const char *ExpectedText)
{
    memset(Evt, 0, sizeof(*Evt));
    Evt->ExpectedEvent = ExpectedEvent;
    Evt->ExpectedText = ExpectedText;
    UT_SetVaHookFunction(UT_KEY(CFE_EVS_SendEvent), UT_CheckEvent_Hook, Evt);
}/* end UT_CheckEvent_Setup() */

static void Init_VerErr(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(-1, 0), CFE_ES_ERR_APP_CREATE);
}/* end Init_VerErr() */

static void Init_Nominal(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitModule(4, 0), CFE_SUCCESS);
}/* end Init_Nominal() */

void Test_SBN_UDP_Init(void)
{
    Init_VerErr();
    Init_Nominal();
}/* end Test_SBN_UDP_Init() */

static void InitNet_OpenErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketOpen), 1, OS_ERROR);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketBind), 1, OS_SUCCESS);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "socket call failed");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitNet(NetPtr), SBN_ERROR);

    EVENT_CNT(1);
}/* end InitNet_OpenErr() */

static void InitNet_BindErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketOpen), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketBind), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "bind call failed (NetData=0x");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitNet(NetPtr), SBN_ERROR);

    EVENT_CNT(1);
}/* end InitNet_OpenErr() */

static void InitNet_Nominal(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketOpen), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketBind), 1, OS_SUCCESS);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "creating socket (NetData=0x");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitNet(NetPtr), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end InitNet_Nominal() */

void Test_SBN_UDP_InitNet(void)
{
    InitNet_OpenErr();
    InitNet_BindErr();
    InitNet_Nominal();
}/* end Test_SBN_UDP_InitNet() */

static void Test_SBN_UDP_InitPeer(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.InitPeer(PeerPtr), SBN_SUCCESS);
}/* end Test_SBN_UDP_InitPeer() */

static void LoadNet_AddrErr(void)
{
    START();

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "invalid address (Address=no colon)");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "no colon"), SBN_ERROR);

    EVENT_CNT(1);
}/* end LoadNet_AddrErr() */

static void LoadNet_InitErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "addr init failed (Status=-1)");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_ERROR);

    EVENT_CNT(1);
}/* end LoadNet_InitErr() */

static void LoadNet_HostErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrFromString), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "addr host set failed (AddrHost=");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_ERROR);

    EVENT_CNT(1);
}/* end LoadNet_HostErr() */

static void LoadNet_PortErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrSetPort), 1, OS_ERROR);
    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "addr port set failed (Port=");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_ERROR);

    EVENT_CNT(1);
}/* end LoadNet_PortErr() */

static void LoadNet_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "configured (NetData=");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadNet(NetPtr, "localhost:1234"), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end LoadNet_Nominal() */

void Test_SBN_UDP_LoadNet(void)
{
    LoadNet_AddrErr();
    LoadNet_InitErr();
    LoadNet_HostErr();
    LoadNet_PortErr();
    LoadNet_Nominal();
}/* end Test_SBN_UDP_LoadNet() */

static void LoadPeer_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_CONFIG_EID, "configured (PeerData=");

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.LoadPeer(PeerPtr, "localhost:1234"), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end LoadPeer_Nominal() */

void Test_SBN_UDP_LoadPeer(void)
{
    /* LoadPeer() uses the same internal ConfAddr() fn as LoadNet(), so we need not cover errs there */
    LoadPeer_Nominal();
}/* end Test_SBN_UDP_LoadNet() */

static void PollPeer_ConnTimeout(void)
{
    START();

    OS_time_t tm;
    memset(&tm, 0, sizeof(tm));

    PeerPtr->Connected = true;

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_DEBUG_EID, "disconnected CPU ");

    tm.seconds = SBN_UDP_PEER_TIMEOUT + 1;
    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end PollPeer_ConnTimeout() */

static void PollPeer_HeartbeatTimeout(void)
{
    START();

    OS_time_t tm;

    memset(&tm, 0, sizeof(tm));

    PeerPtr->Connected = true;

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_DEBUG_EID, "heartbeat CPU ");

    tm.seconds = SBN_UDP_PEER_HEARTBEAT + 1;
    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(SBN_SendNetMsg), 1, SBN_SUCCESS);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end PollPeer_HeartbeatTimeout() */

static void PollPeer_AnnTimeout(void)
{
    START();

    OS_time_t tm;

    memset(&tm, 0, sizeof(tm));

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_DEBUG_EID, "announce CPU ");

    tm.seconds = SBN_UDP_ANNOUNCE_MSG + 1;
    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(SBN_SendNetMsg), 1, SBN_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 2);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);

    EVENT_CNT(1);
}/* end PollPeer_AnnTimeout() */

static void PollPeer_Nominal(void)
{
    START();

    OS_time_t tm;

    memset(&tm, 0, sizeof(tm));

    PeerPtr->Connected = true;

    UT_SetDataBuffer(UT_KEY(OS_GetLocalTime), &tm, sizeof(tm), false);
    UT_SetDeferredRetcode(UT_KEY(OS_GetLocalTime), 1, OS_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(SBN_SendNetMsg), 1, SBN_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.PollPeer(PeerPtr), SBN_SUCCESS);
}/* end PollPeer_Nominal() */

void Test_SBN_UDP_PollPeer(void)
{
    PollPeer_ConnTimeout();
    PollPeer_HeartbeatTimeout();
    PollPeer_AnnTimeout();
    PollPeer_Nominal();
}/* end Test_SBN_UDP_LoadNet() */

static void Send_AddrInitErr(void)
{
    START();

    CFE_SB_MsgPtr_t SBMsgPtr;
    CCSDS_TelemetryPacket_t TlmPkt;

    SBMsgPtr = (CFE_SB_MsgPtr_t)&TlmPkt;
    CFE_SB_InitMsg(SBMsgPtr, 0x1234, CFE_SB_TLM_HDR_SIZE, true);

    UT_CheckEvent_Setup(&EventTest, SBN_UDP_SOCK_EID, "socket addr init failed");

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_ERROR);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.Send(PeerPtr, SBN_APP_MSG, CFE_SB_TLM_HDR_SIZE, SBMsgPtr), SBN_ERROR);

    EVENT_CNT(1);
}/* end Send_AddrInitErr() */

static void Send_SendErr(void)
{
    START();
    CFE_SB_MsgPtr_t SBMsgPtr;
    CCSDS_TelemetryPacket_t TlmPkt;

    SBMsgPtr = (CFE_SB_MsgPtr_t)&TlmPkt;
    CFE_SB_InitMsg(SBMsgPtr, 0x1234, CFE_SB_TLM_HDR_SIZE, true);

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketSendTo), 1, -2);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.Send(PeerPtr, SBN_APP_MSG, CFE_SB_TLM_HDR_SIZE, SBMsgPtr), SBN_ERROR);
}/* end Send_SendErr() */

static void Send_Nominal(void)
{
    START();
    CFE_SB_MsgPtr_t SBMsgPtr;
    CCSDS_TelemetryPacket_t TlmPkt;

    SBMsgPtr = (CFE_SB_MsgPtr_t)&TlmPkt;
    CFE_SB_InitMsg(SBMsgPtr, 0x1234, CFE_SB_TLM_HDR_SIZE, true);

    UT_SetDeferredRetcode(UT_KEY(OS_SocketAddrInit), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketSendTo), 1, CFE_SB_TLM_HDR_SIZE + SBN_PACKED_HDR_SZ);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.Send(PeerPtr, SBN_APP_MSG, CFE_SB_TLM_HDR_SIZE, SBMsgPtr), SBN_SUCCESS);
}/* end Send_Nominal() */

void Test_SBN_UDP_Send(void)
{
    Send_AddrInitErr();
    Send_SendErr();
    Send_Nominal();
}/* end Test_SBN_UDP_LoadNet() */

static int32 NoDataHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    *((uint32 *)Context->ArgPtr[1]) = 0;
    return OS_SUCCESS;
}/* end NoDataHook() */

static void Recv_NoData(void)
{
    START();

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), NoDataHook, NULL);
/*    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS); */

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, NULL, NULL, NULL, NULL), SBN_IF_EMPTY);
}/* end Recv_NoData() */

static int32 DataHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    *((uint32 *)Context->ArgPtr[0]) = OS_STREAM_STATE_READABLE;
    return OS_SUCCESS;
}/* end DataHook() */

static void Recv_SockRecvErr(void)
{
    START();

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, -1);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, NULL, NULL, NULL, NULL), SBN_ERROR);
}/* end Recv_SockRecvErr() */

static void Recv_UnpackErr(void)
{
    START();

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(SBN_UnpackMsg), 1, false);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, NULL, NULL, NULL, NULL), SBN_ERROR);
}/* end Recv_UnpackErr() */

static void Recv_GetPeerErr(void)
{
    START();

    SBN_MsgType_t MsgType;
    SBN_MsgSz_t MsgSz;
    CFE_ProcessorID_t ProcessorID;
    uint8 PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_Unpack_Buf_t UnpackBuf;

    UnpackBuf.MsgSz = 16;
    UnpackBuf.MsgType = SBN_APP_MSG;
    UnpackBuf.ProcessorID = PeerPtr->ProcessorID;
    strncpy((char *)UnpackBuf.MsgBuf, "deadbeef", 9);

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 1);
    UT_SetDataBuffer(UT_KEY(SBN_UnpackMsg), &UnpackBuf, sizeof(UnpackBuf), false);
    PeerPtr = NULL;
    UT_SetDataBuffer(UT_KEY(SBN_GetPeer), &PeerPtr, sizeof(PeerPtr), false);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, PayloadBuffer), SBN_ERROR);
}/* end Recv_GetPeerErr() */

static void Recv_NewConn(void)
{
    START();

    SBN_MsgType_t MsgType;
    SBN_MsgSz_t MsgSz;
    CFE_ProcessorID_t ProcessorID;
    uint8 PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_Unpack_Buf_t UnpackBuf;

    UnpackBuf.MsgSz = 16;
    UnpackBuf.MsgType = SBN_APP_MSG;
    UnpackBuf.ProcessorID = PeerPtr->ProcessorID;
    strncpy((char *)UnpackBuf.MsgBuf, "deadbeef", 9);

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 1);
    UT_SetDataBuffer(UT_KEY(SBN_UnpackMsg), &UnpackBuf, sizeof(UnpackBuf), false);
    UT_SetDataBuffer(UT_KEY(SBN_GetPeer), &PeerPtr, sizeof(PeerPtr), false);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, PayloadBuffer), CFE_SUCCESS);

    UtAssert_True(PeerPtr->Connected == true, "Peer not connected (%s)", __func__);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(MsgSz, 16);
    UtAssert_INT32_EQ(ProcessorID, PeerPtr->ProcessorID);
}/* end Recv_NewConn() */

static void Recv_Disconn(void)
{
    START();

    SBN_MsgType_t MsgType;
    SBN_MsgSz_t MsgSz;
    CFE_ProcessorID_t ProcessorID;
    uint8 PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_Unpack_Buf_t UnpackBuf;

    PeerPtr->Connected = true;

    UnpackBuf.MsgSz = 16;
    UnpackBuf.MsgType = SBN_UDP_DISCONN_MSG;
    UnpackBuf.ProcessorID = PeerPtr->ProcessorID;
    strncpy((char *)UnpackBuf.MsgBuf, "deadbeef", 9);

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 1);
    UT_SetDataBuffer(UT_KEY(SBN_UnpackMsg), &UnpackBuf, sizeof(UnpackBuf), false);
    UT_SetDataBuffer(UT_KEY(SBN_GetPeer), &PeerPtr, sizeof(PeerPtr), false);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, PayloadBuffer), CFE_SUCCESS);

    UtAssert_True(PeerPtr->Connected == false, "Peer connected (%s)", __func__);
    UtAssert_INT32_EQ(MsgType, SBN_UDP_DISCONN_MSG);
    UtAssert_INT32_EQ(MsgSz, 16);
    UtAssert_INT32_EQ(ProcessorID, PeerPtr->ProcessorID);
}/* end Recv_NewConn() */

static void Recv_Nominal(void)
{
    START();

    SBN_MsgType_t MsgType;
    SBN_MsgSz_t MsgSz;
    CFE_ProcessorID_t ProcessorID;
    uint8 PayloadBuffer[CFE_MISSION_SB_MAX_SB_MSG_SIZE];
    SBN_Unpack_Buf_t UnpackBuf;

    PeerPtr->Connected = true;

    UnpackBuf.MsgSz = 16;
    UnpackBuf.MsgType = SBN_APP_MSG;
    UnpackBuf.ProcessorID = PeerPtr->ProcessorID;
    strncpy((char *)UnpackBuf.MsgBuf, "deadbeef", 9);

    UT_SetHookFunction(UT_KEY(OS_SelectSingle), DataHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SelectSingle), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_SocketRecvFrom), 1, 1);
    UT_SetDataBuffer(UT_KEY(SBN_UnpackMsg), &UnpackBuf, sizeof(UnpackBuf), false);
    UT_SetDataBuffer(UT_KEY(SBN_GetPeer), &PeerPtr, sizeof(PeerPtr), false);

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.RecvFromNet(NetPtr, &MsgType, &MsgSz, &ProcessorID, PayloadBuffer), CFE_SUCCESS);

    UtAssert_True(PeerPtr->Connected == true, "Peer not connected (%s)", __func__);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(MsgSz, 16);
    UtAssert_INT32_EQ(ProcessorID, PeerPtr->ProcessorID);
}/* end Recv_Nominal() */

void Test_SBN_UDP_Recv(void)
{
    Recv_NoData();
    Recv_SockRecvErr();
    Recv_UnpackErr();
    Recv_GetPeerErr();
    Recv_NewConn();
    Recv_Disconn();
    Recv_Nominal();
}/* end Test_SBN_UDP_Recv() */

static void UnloadPeer_Disconn(void)
{
    START();

    PeerPtr->Connected = true;

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.UnloadPeer(PeerPtr), CFE_SUCCESS);

    UtAssert_True(PeerPtr->Connected == false, "Peer still connected (%s)", __func__);
}/* end UnloadPeer_Disconn() */

static void UnloadPeer_Nominal(void)
{
    START();

    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.UnloadPeer(PeerPtr), CFE_SUCCESS);

    UtAssert_True(PeerPtr->Connected == false, "Peer connected (%s)", __func__);
}/* end UnloadPeer_Nominal() */

void Test_SBN_UDP_UnloadPeer(void)
{
    UnloadPeer_Disconn();
    UnloadPeer_Nominal();
}/* end Test_SBN_UDP_UnloadPeer() */

static void UnloadNet_Nominal(void)
{
    START();

    SBN_UDP_Net_t *NetData = (SBN_UDP_Net_t *)&(NetPtr->ModulePvt);

    PeerPtr->Connected = true;

    NetData->Socket = OS_open(NULL, 0, 0);
    UT_TEST_FUNCTION_RC(SBN_UDP_Ops.UnloadNet(NetPtr), CFE_SUCCESS);

    /* TODO: check what was called? */
    UtAssert_True(PeerPtr->Connected == false, "Peer still connected (%s)", __func__);
}/* end UnloadNet_Nominal() */

void Test_SBN_UDP_UnloadNet(void)
{
    UnloadNet_Nominal();
}/* end Test_SBN_UDP_UnloadPeer() */

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
