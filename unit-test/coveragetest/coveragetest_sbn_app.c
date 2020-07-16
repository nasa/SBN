#include "sbn_coveragetest_common.h"
#include "sbn_app.h"

typedef struct
{
    uint16 ExpectedEvent;
    int MatchCount;
    const char *ExpectedText;
} UT_CheckEvent_t;

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
            }
        }
    }

    return 0;
}

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
}
static void AppMain_ESRegisterErr(void)
{
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RegisterApp), 1, 1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_EVS_Register, 0);
}/* end AppMain_ESRegisterErr() */

static void AppMain_EVSRegisterErr(void)
{
    UT_SetDeferredRetcode(UT_KEY(CFE_EVS_Register), 1, 1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_ES_GetAppID, 0);
}/* end AppMain_EVSRegisterErr() */

static void AppMain_AppIdErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_INIT_EID, "unable to get AppID");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetAppID), 1, 1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(OS_TaskGetId, 0);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_AppIdErr() */

static void AppMain_TaskInfoErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_INIT_EID, "SBN failed to get task info (1)");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetTaskInfo), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_TaskInfoErr() */

static void LoadConfTbl_RegisterErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_TBL_EID, "unable to register conf tbl handle");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_RegisterErr() */

static void LoadConfTbl_LoadErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_TBL_EID, "unable to load conf tbl /cf/sbn_conf_tbl.tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_LoadErr() */

static void LoadConfTbl_ManageErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_TBL_EID, "unable to manage conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_ManageErr() */

static void LoadConfTbl_NotifyErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_NotifyErr() */

static void AppMain_MutSemCrErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_INIT_EID, "error creating mutex for send tasks");
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_MutSemCrErr() */

static void InitInt_NoNets(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_PEER_EID, "no networks configured");
    SBN.NetCnt = 0;

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NoNets() */

static void InitInt_NetConfErr(void)
{
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_PEER_EID, "network #0 not configured");

    SBN.NetCnt = 1;
    SBN.Nets[0].Configured = false;

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetConfErr() */

static SBN_Status_t NetSuccess(SBN_NetInterface_t *Host)
{
    return SBN_SUCCESS;
}/* end NetSuccess() */

SBN_Status_t RecvFromPeer(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
    CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    return SBN_SUCCESS;
}/* end RecvFromPeer() */

SBN_Status_t RecvFromNet(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr, CFE_ProcessorID_t *ProcessorIDPtr,
    void *PayloadBuffer)
{
    return SBN_SUCCESS;
}/* end RecvFromNet() */

static void InitInt_NetChildErr(void)
{
    SBN_IfOps_t IfOps;
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_PEER_EID, "error creating task for net 0");

    SBN.NetCnt = 1;
    SBN_NetInterface_t *Net = &SBN.Nets[0];
    Net->Configured = true;
    IfOps.InitNet = NetSuccess;
    IfOps.RecvFromNet = RecvFromNet;
    Net->IfOps = &IfOps;
    Net->TaskFlags = SBN_TASK_RECV;

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetChildErr() */

static void InitInt_NetChildSuccess(void)
{
    SBN_IfOps_t IfOps;
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_INIT_EID, "configured, 1 nets");

    SBN.NetCnt = 1;
    SBN_NetInterface_t *Net = &SBN.Nets[0];
    Net->Configured = true;
    IfOps.InitNet = NetSuccess;
    IfOps.RecvFromNet = RecvFromNet;
    Net->IfOps = &IfOps;
    Net->TaskFlags = SBN_TASK_RECV;
    Net->PeerCnt = 0;

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, 1); /* fail out after InitInterfaces() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetChildSuccess() */

static SBN_Status_t PeerSuccess(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}/* end PeerSuccess() */

static void InitInt_PeerChildRecvErr(void)
{
    SBN_IfOps_t IfOps;
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_PEER_EID, "error creating task for 1234");

    SBN.NetCnt = 1;
    SBN_NetInterface_t *Net = &SBN.Nets[0];
    SBN_PeerInterface_t *Peer = &SBN.Nets[0].Peers[0];
    Net->Configured = true;
    IfOps.InitNet = NetSuccess;
    IfOps.InitPeer = PeerSuccess;
    IfOps.RecvFromPeer = RecvFromPeer;
    Net->IfOps = &IfOps;
    Net->PeerCnt = 1;
    Peer->TaskFlags = SBN_TASK_RECV;
    Peer->ProcessorID = 1234;

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_PeerChildRecvErr() */

static void InitInt_PeerChildSendErr(void)
{
    SBN_IfOps_t IfOps;
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_PEER_EID, "error creating send task for 1234");

    SBN.NetCnt = 1;
    SBN_NetInterface_t *Net = &SBN.Nets[0];
    SBN_PeerInterface_t *Peer = &SBN.Nets[0].Peers[0];
    Net->Configured = true;
    IfOps.InitNet = NetSuccess;
    IfOps.InitPeer = PeerSuccess;
    IfOps.RecvFromPeer = RecvFromPeer;
    Net->IfOps = &IfOps;
    Net->PeerCnt = 1;
    Peer->TaskFlags = SBN_TASK_SEND;
    Peer->ProcessorID = 1234;

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_PeerChildSendErr() */

static void InitInt_Success(void)
{
    SBN_IfOps_t IfOps;
    UT_CheckEvent_t EventTest;
    UT_CheckEvent_Setup(&EventTest, SBN_INIT_EID, "configured, 1 nets");

    SBN.NetCnt = 1;
    SBN_NetInterface_t *Net = &SBN.Nets[0];
    SBN_PeerInterface_t *Peer = &SBN.Nets[0].Peers[0];
    Net->Configured = true;
    IfOps.InitNet = NetSuccess;
    IfOps.InitPeer = PeerSuccess;
    IfOps.RecvFromPeer = RecvFromPeer;
    Net->IfOps = &IfOps;
    Net->PeerCnt = 1;
    Peer->TaskFlags = SBN_TASK_SEND | SBN_TASK_RECV;
    Peer->ProcessorID = 1234;

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, 1); /* fail out after InitInterfaces() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_Success() */

void Test_SBN_AppMain(void)
{
    AppMain_ESRegisterErr();
    AppMain_EVSRegisterErr();
    AppMain_AppIdErr();
    AppMain_TaskInfoErr();

    LoadConfTbl_RegisterErr();
    LoadConfTbl_LoadErr();
    LoadConfTbl_ManageErr();
    LoadConfTbl_NotifyErr();

    AppMain_MutSemCrErr();

    InitInt_NoNets();
    InitInt_NetConfErr();
    InitInt_NetChildErr();
    InitInt_NetChildSuccess();
    InitInt_PeerChildRecvErr();
    InitInt_PeerChildSendErr();
    InitInt_Success();
    
    #if 0

    AppMain_SubPipeCrErr();
    AppMain_SubPipeAllSubErr();
    AppMain_SubPipeOneSubErr();

    AppMain_CmdPipeCrErr();
    AppMain_CmdPipeSubErr();

    UnloadModsProtoErr();
    UnloadModsFilterErr();
    #endif
}/* end Test_SBN_AppMain() */

void UT_Setup(void)
{
}/* end UT_Setup() */

void UT_TearDown(void)
{
}/* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_AppMain);
}
