#include "sbn_coveragetest_common.h"
#include "sbn_app.h"

/********************************** event hook ************************************/
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

UT_CheckEvent_t EventTest;

/*
 * Helper function to set up for event checking
 * This attaches the hook function to CFE_EVS_SendEvent
 */
static void UT_CheckEvent_Setup(uint16 ExpectedEvent, const char *ExpectedText)
{
    memset(&EventTest, 0, sizeof(EventTest));
    EventTest.ExpectedEvent = ExpectedEvent;
    EventTest.ExpectedText = ExpectedText;
    UT_SetVaHookFunction(UT_KEY(CFE_EVS_SendEvent), UT_CheckEvent_Hook, &EventTest);
}

/********************************** interface operations ************************************/

static CFE_Status_t ProtoInitModule_Nominal(int ProtoVersion, CFE_EVS_EventID_t BaseEID)
{
    return CFE_SUCCESS;
}/* end ProtoInitModule_Nominal() */

static SBN_Status_t InitNet_Nominal(SBN_NetInterface_t *Net)
{
    Net->Configured = true;
    return SBN_SUCCESS;
}/* end InitNet_Nominal() */

static SBN_Status_t RecvFromNet_Nominal(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr, CFE_ProcessorID_t *ProcessorIDPtr,
    void *PayloadBuffer)
{
    return SBN_SUCCESS;
}/* end RecvFromNet_Nominal() */

static SBN_Status_t LoadPeer_Nominal(SBN_PeerInterface_t *Peer, const char *Address)
{
    return SBN_SUCCESS;
}/* end LoadPeer_Nominal() */

SBN_IfOps_t IfOps =
{
    .InitModule = ProtoInitModule_Nominal,
    .InitNet = InitNet_Nominal,
    .InitPeer = NULL,
    .LoadNet = NULL,
    .LoadPeer = LoadPeer_Nominal,
    .PollPeer = NULL,
    .Send = NULL,
    .RecvFromPeer = NULL,
    .RecvFromNet = RecvFromNet_Nominal,
    .UnloadNet = NULL,
    .UnloadPeer = NULL
}; /* end IfOps */

SBN_IfOps_t *IfOpsPtr = &IfOps;

static CFE_Status_t FilterInitModule_Nominal(int FilterVersion, CFE_EVS_EventID_t BaseEID)
{
    return CFE_SUCCESS;
}/* end InitFilterModule() */

SBN_FilterInterface_t FilterInterface =
{
    .InitModule = FilterInitModule_Nominal,
    .FilterRecv = NULL,
    .FilterSend = NULL,
    .RemapMID = NULL
}; /* end FilterInterface */

SBN_FilterInterface_t *FilterInterfacePtr = &FilterInterface;

/********************************** table ************************************/

SBN_ConfTbl_t NominalTbl =
{
    .ProtocolModules =
    {
        {
            .Name = "UDP",
            .LibFileName = "/cf/sbn_udp.so",
            .LibSymbol = "SBN_UDP_Ops",
            .BaseEID = 0x0100
        }
    },
    .ProtocolCnt = 1,
    .FilterModules =
    {
        {
            .Name = "CCSDS Endian",
            .LibFileName = "/cf/sbn_f_ccsds_end.so",
            .LibSymbol = "SBN_F_CCSDS_End",
            .BaseEID = 0x1000
        }
    },
    .FilterCnt = 1,

    .Peers =
    {
       { /* [0] */
            .ProcessorID = 1,
            .SpacecraftID = 42,
            .NetNum = 0,
            .ProtocolName = "UDP",
            .Filters = {
                "CCSDS Endian"
            },
            .Address = "127.0.0.1:2234",
            .TaskFlags = SBN_TASK_POLL
        },
    },
    .PeerCnt = 1
};

SBN_ConfTbl_t *NominalTblPtr = &NominalTbl;

/********************************** tests ************************************/

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
    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to get AppID");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetAppID), 1, 1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(OS_TaskGetId, 0);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_AppIdErr() */

static void AppMain_TaskInfoErr(void)
{
    UT_CheckEvent_Setup(SBN_INIT_EID, "SBN failed to get task info (1)");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetTaskInfo), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_TaskInfoErr() */

/********************************** load conf tbl tests  ************************************/

static void LoadConfTbl_RegisterErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to register conf tbl handle");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_RegisterErr() */

static void LoadConfTbl_LoadErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to load conf tbl /cf/sbn_conf_tbl.tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_LoadErr() */

static void LoadConfTbl_ManageErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to manage conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_ManageErr() */

static void LoadConfTbl_NotifyErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_NotifyErr() */

static void Test_AppMain_LoadConfTbl(void)
{
    LoadConfTbl_RegisterErr();
    LoadConfTbl_LoadErr();
    LoadConfTbl_ManageErr();
    LoadConfTbl_NotifyErr();
}/* end Test_AppMain_LoadConfTbl() */

char SymLookHook_LastSeen[32] = "";

int32 SymLookHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    char *SymbolName = (char *)Context->ArgPtr[1];

    /* this forces the LoadConf_Module() function to call ModuleLoad */

    /* we've seen this symbol already, time to load */
    if (!strcmp(SymbolName, SymLookHook_LastSeen))
    {
        if(!strcmp(SymbolName, "SBN_UDP_Ops"))
        {
            /* the stub will read this into the addr */
            UT_SetDataBuffer(UT_KEY(OS_SymbolLookup), &IfOpsPtr, sizeof(IfOpsPtr), false);
        }
        else if (!strcmp(SymbolName, "SBN_F_CCSDS_End"))
        {
            /* the stub will read this into the addr */
            UT_SetDataBuffer(UT_KEY(OS_SymbolLookup), &FilterInterfacePtr, sizeof(FilterInterfacePtr), false);
        }/* end if */

        return OS_SUCCESS;
    }
    else
    {
        strcpy(SymLookHook_LastSeen, SymbolName); /* so that next call succeeds */

        return 1;
    }/* end if */
}/* end SymLookHook() */

static void LoadConf_Module_ProtoLibFNNull(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=UDP)");

    char tmpFNFirst = NominalTbl.ProtocolModules[0].LibFileName[0];
    NominalTbl.ProtocolModules[0].LibFileName[0] = '\0';

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    SBN_AppMain();

    NominalTbl.ProtocolModules[0].LibFileName[0] = tmpFNFirst;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Module_ProtoLibFNNull() */

static void LoadConf_Module_FiltLibFNNull(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=CCSDS Endian)");

    char tmpFNFirst = NominalTbl.FilterModules[0].LibFileName[0];
    NominalTbl.FilterModules[0].LibFileName[0] = '\0';

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    SBN_AppMain();

    NominalTbl.FilterModules[0].LibFileName[0] = tmpFNFirst;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Module_FiltLibFNNull() */

static void LoadConf_Module_ModLdErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module file (Name=UDP LibFileName=/cf/sbn_udp.so)");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Module_ModLdErr() */

int32 AlwaysErrHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    return 1;
}

static void LoadConf_Module_SymLookErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid symbol (Name=UDP LibSymbol=SBN_UDP_Ops)");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), AlwaysErrHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    SBN_AppMain();

    UT_DEFAULT_IMPL(OS_SymbolLookup);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Module_SymLookErr() */

static void Test_LoadConf_Module(void)
{
    LoadConf_Module_ProtoLibFNNull();
    LoadConf_Module_FiltLibFNNull();
    LoadConf_Module_ModLdErr();
    LoadConf_Module_SymLookErr();
}/* end Test_LoadConf_Module() */

static void LoadConf_GetAddrErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to get conf table address");

    /* make sure it does not return INFO_UPDATED */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED - 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_GetAddrErr() */

static CFE_Status_t ProtoInitErr_InitModule(int ProtocolVersion, CFE_EVS_EventID_t BaseEID)
{
    return 1;
}/* end ProtoInitErr_InitModule */

static void LoadConf_ProtoInitErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "error in protocol init");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    IfOps.InitModule = ProtoInitErr_InitModule;

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    SBN_AppMain();

    IfOps.InitModule = ProtoInitModule_Nominal;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_ProtoInitErr() */

static CFE_Status_t FilterInitErr_InitModule(int FilterVersion, CFE_EVS_EventID_t BaseEID)
{
    return 1;
}/* end FilterInitErr_InitModule */

static void LoadConf_FilterInitErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "error in filter init");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    FilterInterface.InitModule = FilterInitErr_InitModule;

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    SBN_AppMain();

    FilterInterface.InitModule = FilterInitModule_Nominal;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_FilterInitErr() */

static void LoadConf_ProtoNameErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module name XDP");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    char o = NominalTbl.Peers[0].ProtocolName[0];
    NominalTbl.Peers[0].ProtocolName[0] = 'X'; /* temporary make it "XDP" */

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, 1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    NominalTbl.Peers[0].ProtocolName[0] = o;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_ProtoNameErr() */

static void LoadConf_TooManyNets(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "too many networks");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, 1); /* fail just after LoadConfTbl() */

    int i = 0;
    for (; i < SBN_MAX_NETS + 1; i++)
    {
        strcpy(NominalTbl.Peers[i].ProtocolName, "UDP");
        NominalTbl.Peers[i].NetNum = i;
        NominalTbl.Peers[i].ProcessorID = 1;
        NominalTbl.Peers[i].SpacecraftID = 42;
    }/* end for */
    NominalTbl.PeerCnt = i;

    SBN_AppMain();

    NominalTbl.PeerCnt = 1;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_TooManyNets() */

static void LoadConf_ReleaseAddrErr(void)
{
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to release address of conf tbl");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, 1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_ReleaseAddrErr() */

static void LoadConf_Nominal(void)
{
    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, 1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Nominal() */

static void Test_LoadConf(void)
{
    LoadConf_GetAddrErr();
    LoadConf_ProtoInitErr();
    LoadConf_FilterInitErr();
    LoadConf_ProtoNameErr();
    LoadConf_TooManyNets();
    LoadConf_ReleaseAddrErr();
    LoadConf_Nominal();
}/* end Test_LoadConf() */

static void AppMain_MutSemCrErr(void)
{
    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, 1);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_MutSemCrErr() */

void Test_SBN_AppMain(void)
{
    AppMain_ESRegisterErr();
    AppMain_EVSRegisterErr();
    AppMain_AppIdErr();
    AppMain_TaskInfoErr();

    Test_AppMain_LoadConfTbl();

    Test_LoadConf_Module();

    Test_LoadConf();

    AppMain_MutSemCrErr();

    #if 0

    InitInt_NoNets();
    InitInt_NetConfErr();
    InitInt_NetChildErr();
    InitInt_NetChildSuccess();
    InitInt_PeerChildRecvErr();
    InitInt_PeerChildSendErr();
    InitInt_Success();
    
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
    #if 0
    ADD_TEST(SBN_ProcessNetMsg);
    ADD_TEST(SBN_GetPeer);
    ADD_TEST(SBN_Connected);
    ADD_TEST(SBN_Disconnected);
    ADD_TEST(SBN_ReloadConfTbl);
    #endif
}
