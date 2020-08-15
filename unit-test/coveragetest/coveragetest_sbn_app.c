#include "sbn_coveragetest_common.h"
#include "sbn_app.h"
#include "cfe_msgids.h"
#include "cfe_sb_events.h"
#include "sbn_pack.h"

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

static SBN_Status_t LoadNet_Nominal(SBN_NetInterface_t *Net, const char *Address)
{
    return SBN_SUCCESS;
}/* end InitNet_Nominal() */

static SBN_Status_t InitPeer_Nominal(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}/* end InitPeer_Nominal() */

static SBN_Status_t RecvFromNet_Nominal(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
    CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    *ProcessorIDPtr = 1235;

    return SBN_SUCCESS;
}/* end RecvFromNet_Nominal() */

static SBN_Status_t LoadPeer_Nominal(SBN_PeerInterface_t *Peer, const char *Address)
{
    return SBN_SUCCESS;
}/* end LoadPeer_Nominal() */

static SBN_Status_t UnloadNet_Nominal(SBN_NetInterface_t *Net)
{
    return SBN_SUCCESS;
}/* end UnloadNet_Nominal() */

static SBN_Status_t UnloadPeer_Nominal(SBN_PeerInterface_t *Net)
{
    return SBN_SUCCESS;
}/* end UnloadPeer_Nominal() */

static SBN_Status_t PollPeer_Nominal(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}/* end PollPeer_Nominal() */

static SBN_MsgSz_t Send_Nominal(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload)
{
    return MsgSz;
}/* end Send_Nominal() */

static SBN_MsgSz_t Send_Err(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload)
{
    return -1;
}/* end Send_Err() */

SBN_IfOps_t IfOps =
{
    .InitModule = ProtoInitModule_Nominal,
    .InitNet = InitNet_Nominal,
    .InitPeer = InitPeer_Nominal,
    .LoadNet = LoadNet_Nominal,
    .LoadPeer = LoadPeer_Nominal,
    .PollPeer = PollPeer_Nominal,
    .Send = Send_Nominal,
    .RecvFromPeer = NULL,
    .RecvFromNet = RecvFromNet_Nominal,
    .UnloadNet = UnloadNet_Nominal,
    .UnloadPeer = UnloadPeer_Nominal
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
            .ProcessorID = 1234,
            .SpacecraftID = 5678,
            .NetNum = 0,
            .ProtocolName = "UDP",
            .Filters = {
                "CCSDS Endian"
            },
            .Address = "127.0.0.1:2234",
            .TaskFlags = SBN_TASK_POLL
        },
        { /* [1] */
            .ProcessorID = 1235,
            .SpacecraftID = 5678,
            .NetNum = 0,
            .ProtocolName = "UDP",
            .Filters = {
                "CCSDS Endian"
            },
            .Address = "127.0.0.1:2235",
            .TaskFlags = SBN_TASK_POLL
        },
    },
    .PeerCnt = 2
}; /* end NominalTbl */

SBN_ConfTbl_t *NominalTblPtr = &NominalTbl;

/********************************** globals ************************************/
CFE_ProcessorID_t ProcessorID = 1234;
CFE_SpacecraftID_t SpacecraftID = 5678;
CFE_SB_MsgId_t MsgID = 0xDEAD;
SBN_NetInterface_t *NetPtr = NULL;
SBN_PeerInterface_t *PeerPtr = NULL;
/********************************** tests ************************************/
/* SBN tries to look up the symbol of the protocol or filter module using OS_SymbolLookup, if it
 * finds it (because ES loaded it) it does nothing; if it does not, it tries to load the symbol.
 * This hook forces the OS_SymbolLookup to fail the first time but succeed the second time and
 * load the symbol into the data buffer so that the "load" has been performed successfully.
 */
static int32 SymLookHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    static char LastSeen[32] = {0};
    char *SymbolName = (char *)Context->ArgPtr[1];

    /* this forces the LoadConf_Module() function to call ModuleLoad */

    /* we've seen this symbol already, time to load */
    if (!strcmp(SymbolName, LastSeen))
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
        strcpy(LastSeen, SymbolName); /* so that next call succeeds */

        return 1;
    }/* end if */
}/* end SymLookHook() */

#define START() START_fn(__func__, __LINE__)
void START_fn(const char *func, int line)
{
    UT_ResetState(0);
    printf("Start item %s (%d)\n", func, line);
    memset(&SBN, 0, sizeof(SBN));

    NetPtr = &SBN.Nets[0];
    SBN.NetCnt = 1;
    NetPtr->PeerCnt = 1;
    NetPtr->Configured = 1;
    PeerPtr = &NetPtr->Peers[0];
    PeerPtr->ProcessorID = ProcessorID;
    PeerPtr->SpacecraftID = SpacecraftID;
    PeerPtr->Net = NetPtr;
    NetPtr->IfOps = &IfOps;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
}/* end START() */

static void AppMain_ESRegisterErr(void)
{
    START();
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RegisterApp), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_EVS_Register, 0);
}/* end AppMain_ESRegisterErr() */

static void AppMain_EVSRegisterErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(CFE_EVS_Register), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_ES_GetAppID, 0);
}/* end AppMain_EVSRegisterErr() */

static void AppMain_AppIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to get AppID");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetAppID), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(OS_TaskGetId, 0);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_AppIdErr() */

static void AppMain_TaskInfoErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "SBN failed to get task info (");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetTaskInfo), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_TaskInfoErr() */

/********************************** load conf tbl tests  ************************************/

static void LoadConfTbl_RegisterErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to register conf tbl handle");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_RegisterErr() */

static void LoadConfTbl_LoadErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to load conf tbl /cf/sbn_conf_tbl.tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_LoadErr() */

static void LoadConfTbl_ManageErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to manage conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_ManageErr() */

static void LoadConfTbl_NotifyErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, -1);

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

static void LoadConf_Module_ProtoLibFNNull(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=UDP)");

    char tmpFNFirst = NominalTbl.ProtocolModules[0].LibFileName[0];
    NominalTbl.ProtocolModules[0].LibFileName[0] = '\0';

    SBN_AppMain();

    NominalTbl.ProtocolModules[0].LibFileName[0] = tmpFNFirst;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Module_ProtoLibFNNull() */

static void LoadConf_Module_FiltLibFNNull(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=CCSDS Endian)");

    char tmpFNFirst = NominalTbl.FilterModules[0].LibFileName[0];
    NominalTbl.FilterModules[0].LibFileName[0] = '\0';

    SBN_AppMain();

    NominalTbl.FilterModules[0].LibFileName[0] = tmpFNFirst;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Module_FiltLibFNNull() */

static void LoadConf_Module_ModLdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module file (Name=UDP LibFileName=/cf/sbn_udp.so)");

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Module_ModLdErr() */

int32 AlwaysErrHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    return 1;
}

static void LoadConf_Module_SymLookErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid symbol (Name=UDP LibSymbol=SBN_UDP_Ops)");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), AlwaysErrHook, NULL);

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
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to get conf table address");

    /* make sure it does not return INFO_UPDATED */
    UT_ResetState(UT_KEY(CFE_TBL_GetAddress));
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
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "error in protocol init");

    IfOps.InitModule = ProtoInitErr_InitModule;

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
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "error in filter init");

    FilterInterface.InitModule = FilterInitErr_InitModule;

    SBN_AppMain();

    FilterInterface.InitModule = FilterInitModule_Nominal;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_FilterInitErr() */

static void LoadConf_ProtoNameErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module name XDP");

    char o = NominalTbl.Peers[0].ProtocolName[0];
    NominalTbl.Peers[0].ProtocolName[0] = 'X'; /* temporary make it "XDP" */

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    NominalTbl.Peers[0].ProtocolName[0] = o;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_ProtoNameErr() */

static void LoadConf_FiltNameErr(void)
{
    START();

    memset(&SBN, 0, sizeof(SBN)); /* will be loaded by LoadConfTbl() */

    UT_CheckEvent_Setup(SBN_TBL_EID, "Invalid filter name: XCSDS Endian");

    char o = NominalTbl.Peers[0].Filters[0][0];
    NominalTbl.Peers[0].Filters[0][0] = 'X';

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    NominalTbl.Peers[0].Filters[0][0] = o;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_FiltNameErr() */

static void LoadConf_TooManyNets(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "too many networks");

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    NominalTbl.Peers[0].NetNum = SBN_MAX_NETS + 1;

    SBN_AppMain();

    NominalTbl.Peers[0].NetNum = 0;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_TooManyNets() */

static void LoadConf_ReleaseAddrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to release address of conf tbl");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_ReleaseAddrErr() */

static void LoadConf_NetCntInc(void)
{
    START();

    SBN.NetCnt = 0;

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    UtAssert_INT32_EQ(SBN.NetCnt, 1);
}/* end LoadConf_NetCntInc() */

static void LoadConf_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_Nominal() */

static void Test_LoadConf(void)
{
    LoadConf_GetAddrErr();
    LoadConf_ProtoInitErr();
    LoadConf_FilterInitErr();
    LoadConf_ProtoNameErr();
    LoadConf_FiltNameErr();
    LoadConf_TooManyNets();
    LoadConf_ReleaseAddrErr();
    LoadConf_NetCntInc();
    LoadConf_Nominal();
}/* end Test_LoadConf() */

static void AppMain_MutSemCrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1);

    SBN_AppMain();

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 0, 0);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_MutSemCrErr() */

static int32 NoNetsHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    SBN.NetCnt = 0;
    return CFE_SUCCESS;
}/* end NoNetsHook() */

static void InitInt_NoNets(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "no networks configured");

    UT_SetHookFunction(UT_KEY(OS_MutSemCreate), NoNetsHook, NULL);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NoNets() */

static int32 NetConfHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    SBN.Nets[0].Configured = FALSE;
    return CFE_SUCCESS;
}/* end NetConfHook() */

static void InitInt_NetConfErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "network #0 not configured");

    UT_SetHookFunction(UT_KEY(OS_MutSemCreate), NetConfHook, NULL);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetConfErr() */

static void InitInt_NetChildErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for net 0");

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, -1);
    
    NetPtr->TaskFlags = SBN_TASK_RECV;

    SBN_AppMain();

    NetPtr->TaskFlags = SBN_TASK_POLL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetChildErr() */

static void InitInt_NetChildSuccess(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "configured, 1 nets");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1); /* fail just after InitInterfaces() */

    NetPtr->TaskFlags = SBN_TASK_RECV;

    SBN_AppMain();

    NetPtr->TaskFlags = SBN_TASK_POLL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetChildSuccess() */

static SBN_Status_t RecvFromPeer_Nominal(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr, CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    return SBN_SUCCESS;
}/* end RecvFromPeer_Nominal() */

static void InitInt_PeerChildRecvErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for 1");

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, -1);

    NominalTbl.Peers[1].TaskFlags = SBN_TASK_RECV;
    IfOps.RecvFromNet = NULL;
    IfOps.RecvFromPeer = RecvFromPeer_Nominal;

    SBN_AppMain();

    NominalTbl.Peers[1].TaskFlags = SBN_TASK_POLL;
    IfOps.RecvFromNet = RecvFromNet_Nominal;
    IfOps.RecvFromPeer = NULL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_PeerChildRecvErr() */

static void InitInt_PeerChildSendErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating send task for 1");

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 2, -1);

    NominalTbl.Peers[1].TaskFlags = SBN_TASKS;
    IfOps.RecvFromNet = NULL;
    IfOps.RecvFromPeer = RecvFromPeer_Nominal;

    SBN_AppMain();

    NominalTbl.Peers[1].TaskFlags = SBN_TASK_POLL;
    IfOps.RecvFromNet = RecvFromNet_Nominal;
    IfOps.RecvFromPeer = NULL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_PeerChildRecvErr() */

static void InitInt_PeerChildSuccess(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "configured, 1 nets");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1); /* fail just after InitInterfaces() */

    NominalTbl.Peers[1].TaskFlags = SBN_TASKS;
    IfOps.RecvFromNet = NULL;
    IfOps.RecvFromPeer = RecvFromPeer_Nominal;

    SBN_AppMain();

    NominalTbl.Peers[1].TaskFlags = SBN_TASK_POLL;
    IfOps.RecvFromNet = RecvFromNet_Nominal;
    IfOps.RecvFromPeer = NULL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_PeerChildRecvSuccess() */

static void Test_InitInt(void)
{
    InitInt_NoNets();
    InitInt_NetConfErr();
    InitInt_NetChildErr();
    InitInt_NetChildSuccess();
    InitInt_PeerChildRecvErr();
    InitInt_PeerChildSendErr();
    InitInt_PeerChildSuccess();
}/* end Test_InitInt() */

static void AppMain_SubPipeCrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create subscription pipe (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SB_BAD_ARGUMENT); /* fail just after InitInterfaces() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_SubPipeCrErr() */

static void AppMain_SubPipeAllSubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to allsubs (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_SubPipeAllSubErr() */

static void AppMain_SubPipeOneSubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to sub (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_SubPipeOneSubErr() */

static void AppMain_CmdPipeCrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create command pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_CmdPipeCrErr() */

static void AppMain_CmdPipeSubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to command pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_CmdPipeSubErr() */

static void SBStart_CrPipeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create event pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 3, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_CrPipeErr() */

static void SBStart_SubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to event pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_SubErr() */

static void SBStart_RcvMsgErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_MSG_EID, "err from rcvmsg on sub pipe");

    /* first call to SBN_CheckSubscriptionPipe should fail, giving us the event we are looking for */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_PIPE_RD_ERR); /* sub pipe err */

    /* ...but WaitForSBStartup() keeps trying the sub pipe, so go through the loop again */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_PIPE_RD_ERR);

    /* second time through CheckSubscriptionPipe will be nominal operation */
    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &MsgPtr, sizeof(MsgPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* second call to SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStartup() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_RcvMsgErr() */

static void SBStart_UnsubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to unsubscribe from event messages");

    /* SBN_CheckSubscriptionPipe()... */
    /* ...RcvMsg() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    /* ...GetMsgId() -> sub msg */
    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* WaitForSBStartup()... */
    /* ...CFE_SB_RcvMsg() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &EvtMsgPtr, sizeof(EvtMsgPtr), false);

    /* ...GetMsgId() -> event msg */
    mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStartup() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_UnsubErr() */

static void SBStart_RcvMsgSucc(void)
{
    START();

    /* call to SBN_CheckSubscriptionPipe should fail, giving us the event we are looking for */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_PIPE_RD_ERR); /* sub pipe err */

    /* ...CFE_SB_RcvMsg() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &EvtMsgPtr, sizeof(EvtMsgPtr), false);

    /* ...GetMsgId() -> event msg */
    CFE_SB_MsgId_t mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStartup() */

    SBN_AppMain();
}/* end SBStart_RcvMsgSucc() */

static void SBStart_DelPipeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to delete event pipe");

    /* SBN_CheckSubscriptionPipe() ...*/
    /* ...RcvMsg() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    /* ...GetMsgId() -> sub msg */
    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* WaitForSBStartup()...*/
    /* ...CFE_SB_RcvMsg() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &EvtMsgPtr, sizeof(EvtMsgPtr), false);

    /* ...GetMsgId() -> event msg */
    mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_DeletePipe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_DelPipeErr() */

static void Test_SBStart(void)
{
    SBStart_CrPipeErr();
    SBStart_SubErr();
    SBStart_RcvMsgErr();
    SBStart_UnsubErr();
    SBStart_RcvMsgSucc();
    SBStart_DelPipeErr();
}/* end Test_SBStart() */

static void W4W_NoMsg(void)
{
    START();

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_NO_MESSAGE);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();
}/* end W4W_NoMsg() */

static void CheckPP_SendTask(void)
{
    START();

    NetPtr->SendTaskID = 1;

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    NetPtr->SendTaskID = 0;
}/* end CheckPP_SendTask() */

static SBN_Status_t FilterSend_Empty(void *MsgBuf, SBN_Filter_Ctx_t *Context)
{
    return SBN_IF_EMPTY;
}/* end FilterSend_Empty() */

static void CheckPP_FilterEmpty(void)
{
    START();

    PeerPtr->Connected = 1;

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterSend = FilterSend_Empty;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt = 1;

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();
}/* end CheckPP_FilterEmpty() */

static SBN_Status_t FilterSend_Err(void *MsgBuf, SBN_Filter_Ctx_t *Context)
{
    return SBN_ERROR;
}/* end FilterSend_Empty() */

static void CheckPP_FilterErr(void)
{
    START();

    PeerPtr->Connected = 1;

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterSend = FilterSend_Err;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt = 1;

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();
}/* end CheckPP_FilterErr() */

static SBN_Status_t FilterSend_Nominal(void *MsgBuf, SBN_Filter_Ctx_t *Context)
{
    return SBN_SUCCESS;
}/* end FilterSend_Nominal() */

static void CheckPP_Nominal(void)
{
    START();

    PeerPtr->Connected = 1;

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterSend = FilterSend_Nominal;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt = 1;

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();
}/* end CheckPP_Nominal() */

static void W4W_RcvMsgErr(void)
{
    START();

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_PIPE_RD_ERR);

    SBN_AppMain();
}/* end W4W_RcvMsgErr() */

int32 PollPeerCnt = 0;
static SBN_Status_t PollPeer_Count(SBN_PeerInterface_t *Peer)
{
    PollPeerCnt++;
    return SBN_SUCCESS;
}/* end PollPeer_Nominal() */

static void PeerPoll_NetRecvTask(void)
{
    START();

    PollPeerCnt = 0;
    IfOps.PollPeer = PollPeer_Count;
    NetPtr->RecvTaskID = 1;

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    IfOps.PollPeer = PollPeer_Nominal;

    UtAssert_INT32_EQ(PollPeerCnt, 0);
}/* end PeerPoll_NetRecvTask() */

static void PeerPoll_PeerRecvTask(void)
{
    START();

    NetPtr->Peers[0].RecvTaskID = 1; /* not touched by LoadConf */

    PollPeerCnt = 0;
    IfOps.PollPeer = PollPeer_Count;

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    IfOps.PollPeer = PollPeer_Nominal;

    UtAssert_INT32_EQ(PollPeerCnt, 2);
}/* end PeerPoll_PeerRecvTask() */

static void Test_WaitForWakeup(void)
{
    W4W_NoMsg();
    W4W_RcvMsgErr();
    CheckPP_SendTask();
    CheckPP_FilterEmpty();
    CheckPP_FilterErr();
    CheckPP_Nominal();
    PeerPoll_NetRecvTask();
    PeerPoll_PeerRecvTask();
}/* end Test_SBStart() */

static void AppMain_Nominal(void)
{
    START();

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* CFE_SB_RcvMsg() in WaitForSBStartup() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &EvtMsgPtr, sizeof(EvtMsgPtr), false);

    mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();
}/* end AppMain_Nominal() */

static void Test_SBN_AppMain(void)
{
    AppMain_ESRegisterErr();
    AppMain_EVSRegisterErr();
    AppMain_AppIdErr();
    AppMain_TaskInfoErr();

    Test_AppMain_LoadConfTbl();

    Test_LoadConf_Module();

    Test_LoadConf();

    AppMain_MutSemCrErr();

    Test_InitInt();
    
    AppMain_SubPipeCrErr();
    AppMain_SubPipeAllSubErr();
    AppMain_SubPipeOneSubErr();
    AppMain_CmdPipeCrErr();

    AppMain_CmdPipeSubErr();

    Test_SBStart();

    Test_WaitForWakeup();

    AppMain_Nominal();
}/* end Test_SBN_AppMain() */

static void ProcessNetMsg_PeerErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unknown peer (ProcessorID=");

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID + 1, 0, NULL), SBN_ERROR);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* ProcessNetMsg_PeerErr() */

static void ProcessNetMsg_ProtoMsg_VerErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version mismatch with ProcessorID ");

    uint8 ver = SBN_PROTO_VER + 1;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID, sizeof(ver), &ver), SBN_SUCCESS);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ProcessNetMsg_ProtoMsg_VerErr() */

static void ProcessNetMsg_ProtoMsg_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version match with ProcessorID ");

    uint8 ver = SBN_PROTO_VER;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID, sizeof(ver), &ver), SBN_SUCCESS);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ProcessNetMsg_ProtoMsg_Nominal() */

static SBN_Status_t RecvFilter_Err(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_ERROR;
}/* end RecvFilter_Err() */

static void ProcessNetMsg_AppMsg_FiltErr(void)
{
    START();

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterRecv = RecvFilter_Err;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_ERROR);
}/* end ProcessNetMsg_AppMsg_FiltErr() */

static SBN_Status_t RecvFilter_Out(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_IF_EMPTY;
}/* end RecvFilter_Out() */

static void ProcessNetMsg_AppMsg_FiltOut(void)
{
    START();

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterRecv = RecvFilter_Out;

    PeerPtr->ProcessorID = ProcessorID;
    PeerPtr->SpacecraftID = SpacecraftID;
    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_IF_EMPTY);
}/* end ProcessNetMsg_AppMsg_FiltOut() */

static SBN_Status_t RecvFilter_Nominal(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_SUCCESS;
}/* end RecvFilter_Nominal() */

static void ProcessNetMsg_AppMsg_PassMsgErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SB_EID, "CFE_SB_PassMsg error (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_PassMsg), 1, -1);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_ERROR);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ProcessNetMsg_AppMsg_PassMsgErr() */

static void ProcessNetMsg_AppMsg_Nominal(void)
{
    START();

    SBN_FilterInterface_t Filter_Empty, Filter_Nominal;
    memset(&Filter_Empty, 0, sizeof(Filter_Empty));
    memset(&Filter_Nominal, 0, sizeof(Filter_Nominal));
    Filter_Nominal.FilterRecv = RecvFilter_Nominal;

    /* Filters[0].Recv is NULL, should skip */
    PeerPtr->Filters[0] = &Filter_Empty;
    PeerPtr->Filters[1] = &Filter_Nominal;
    PeerPtr->FilterCnt = 2;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_SUCCESS);
}/* end ProcessNetMsg_AppMsg_Nominal() */

static void ProcessNetMsg_SubMsg_Nominal(void)
{
    START();

    uint8 Buf[SBN_PACKED_SUB_SZ];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, SBN_PACKED_SUB_SZ, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = {0};
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_SUB_MSG, ProcessorID, sizeof(Buf), &Buf), SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->Subs[0].MsgID, MsgID);
}/* end ProcessNetMsg_SubMsg_Nominal() */

static void ProcessNetMsg_UnSubMsg_Nominal(void)
{
    START();

    uint8 Buf[SBN_PACKED_SUB_SZ];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, sizeof(Buf), 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);

    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS;
    Pack_Data(&Pack, &QoS, sizeof(QoS));

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_SUB_MSG, ProcessorID, sizeof(Buf), &Buf), SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->SubCnt, 1);
    UtAssert_INT32_EQ(PeerPtr->Subs[0].MsgID, MsgID);
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_UNSUB_MSG, ProcessorID, sizeof(Buf), &Buf), SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);
}/* end ProcessNetMsg_UnSubMsg_Nominal() */

static void ProcessNetMsg_NoMsg_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_NO_MSG, ProcessorID, 0, NULL), SBN_SUCCESS);
}/* end ProcessNetMsg_NoMsg_Nominal() */

static void ProcessNetMsg_MsgErr(void)
{
    START();

    /* send a net message of an invalid type */
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_NO_MSG + 100, ProcessorID, 0, NULL), SBN_ERROR);
}/* end ProcessNetMsg_MsgErr() */

static void Test_SBN_ProcessNetMsg(void)
{
    ProcessNetMsg_PeerErr();
    ProcessNetMsg_AppMsg_FiltErr();
    ProcessNetMsg_AppMsg_FiltOut();
    ProcessNetMsg_AppMsg_PassMsgErr();
    ProcessNetMsg_ProtoMsg_VerErr();
    ProcessNetMsg_MsgErr();

    ProcessNetMsg_AppMsg_Nominal();
    ProcessNetMsg_SubMsg_Nominal();
    ProcessNetMsg_UnSubMsg_Nominal();
    ProcessNetMsg_ProtoMsg_Nominal();
    ProcessNetMsg_NoMsg_Nominal();
}/* end Test_SBN_ProcessNetMsg() */

static void Connected_AlreadyErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "CPU 1234 already connected");

    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    UtAssert_INT32_EQ(PeerPtr->Connected, 1);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Connected_AlreadyErr() */

static void Connected_PipeOptErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "failed to set pipe options '");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SetPipeOpts), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Connected_PipeOptErr() */

static void Connected_SendErr(void)
{
    START();

    IfOps.Send = Send_Err;

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);

    IfOps.Send = Send_Nominal;
}/* end Connected_SendErr() */

static void Connected_CrPipeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "failed to create pipe '");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Connected_CrPipeErr() */

static void Connected_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->Connected, 1);
}/* end Connected_Nominal() */

static void Test_SBN_Connected(void)
{
    Connected_AlreadyErr();
    Connected_CrPipeErr();
    Connected_PipeOptErr();
    Connected_SendErr();
    Connected_Nominal();
}/* end Test_SBN_Connected() */

static void Disconnected_ConnErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "CPU 1234 not connected");

    SBN_PeerInterface_t *PeerPtr = &SBN.Nets[0].Peers[0];

    PeerPtr->ProcessorID = ProcessorID;

    UtAssert_INT32_EQ(SBN_Disconnected(PeerPtr), SBN_ERROR);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Disconnected_Nominal() */

static void Disconnected_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "CPU 1234 disconnected");

    SBN_PeerInterface_t *PeerPtr = &SBN.Nets[0].Peers[0];

    PeerPtr->ProcessorID = ProcessorID;
    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_Disconnected(PeerPtr), SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->Connected, 0);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Disconnected_Nominal() */

static void Test_SBN_Disconnected(void)
{
    Disconnected_ConnErr();
    Disconnected_Nominal();
}/* end Test_SBN_Disconnected() */

static SBN_Status_t UnloadNet_Err(SBN_NetInterface_t *Net)
{
    return SBN_ERROR;
}/* end UnloadNet_Nominal() */

static void ReloadConfTbl_UnloadNetErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload network ");

    IfOps.UnloadNet = UnloadNet_Err;

    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);

    IfOps.UnloadNet = UnloadNet_Nominal;
}/* end ReloadConfTbl_UnloadNetErr() */

static void ReloadConfTbl_ProtoUnloadErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload protocol module ID ");

    PeerPtr->Connected = 1;

    SBN.ProtocolModules[0] = 1;

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ReloadConfTbl_ProtoUnloadErr() */

static void ReloadConfTbl_FiltUnloadErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload filter module ID ");

    SBN.FilterModules[0] = 1;

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ReloadConfTbl_FiltUnloadErr() */

static void ReloadConfTbl_TblUpdErr(void)
{
    START();

    PeerPtr->Connected = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Update), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);
}/* end ReloadConfTbl_TblUpdErr() */

static void ReloadConfTbl_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_SUCCESS);
}/* end ReloadConfTbl_Nominal() */

static void Test_SBN_ReloadConfTbl(void)
{
    ReloadConfTbl_UnloadNetErr();
    ReloadConfTbl_ProtoUnloadErr();
    ReloadConfTbl_FiltUnloadErr();
    ReloadConfTbl_TblUpdErr();
    ReloadConfTbl_Nominal();
}/* end Test_SBN_ReloadConfTbl() */

static void Unpack_Empty(void)
{
    START();

    uint8 Buf[SBN_MAX_PACKED_MSG_SZ] = {0}, Payload[1] = {0};
    SBN_MsgSz_t MsgSz; SBN_MsgType_t MsgType;
    CFE_ProcessorID_t ProcID;

    SBN_PackMsg(Buf, 0, SBN_APP_MSG, ProcessorID, NULL);
    UtAssert_True(SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, Payload), "unpack of an empty pack");
    UtAssert_INT32_EQ(MsgSz, 0);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(ProcID, ProcessorID);
}/* end Unpack_Empty() */

static void Unpack_Err(void)
{
    START();

    uint8 Buf[SBN_MAX_PACKED_MSG_SZ] = {0}, Payload[1] = {0};
    SBN_MsgSz_t MsgSz; SBN_MsgType_t MsgType;
    CFE_ProcessorID_t ProcID;

    Pack_t Pack;
    Pack_Init(&Pack, Buf, SBN_MAX_PACKED_MSG_SZ + SBN_PACKED_HDR_SZ, 0);
    Pack_Int16(&Pack, -1); /* invalid msg size */
    Pack_UInt8(&Pack, SBN_APP_MSG);
    Pack_UInt32(&Pack, ProcessorID);

    UtAssert_True(!SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, Payload), "unpack of invalid pack");
}/* end Unpack_Err() */

static void Unpack_Nominal(void)
{
    START();

    uint8 Buf[SBN_MAX_PACKED_MSG_SZ] = {0}, Payload[1] = {0};
    uint8 TestData = 123;
    SBN_MsgSz_t MsgSz; SBN_MsgType_t MsgType;
    CFE_ProcessorID_t ProcID;

    SBN_PackMsg(Buf, 1, SBN_APP_MSG, ProcessorID, &TestData);

    UtAssert_True(SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, Payload), "unpack of a pack");

    UtAssert_INT32_EQ(MsgSz, 1);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(ProcID, ProcessorID);
    UtAssert_INT32_EQ((int32)TestData, (int32)Payload[0]);
}/* end Unpack_Nominal() */

static void Test_SBN_PackUnpack(void)
{
    Unpack_Empty();
    Unpack_Err();
    Unpack_Nominal();
}/* end Test_SBN_PackUnpack() */

void RecvNetMsgs_TaskRecv(void)
{
    START();

    NetPtr->TaskFlags = SBN_TASK_RECV;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);
}/* end RecvNetMsgs_TaskRecv() */

static SBN_Status_t RecvFromNet_Empty(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
    CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    *ProcessorIDPtr = 1235;

    return SBN_IF_EMPTY;
}/* end RecvFromNet_Empty() */

void RecvNetMsgs_NetEmpty(void)
{
    START();

    IfOps.RecvFromNet = RecvFromNet_Empty;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);

    IfOps.RecvFromNet = RecvFromNet_Nominal;
}/* end RecvNetMsgs_NetEmpty() */

void RecvNetMsgs_PeerRecv(void)
{
    START();

    IfOps.RecvFromNet = NULL;
    IfOps.RecvFromPeer = RecvFromPeer_Nominal;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);
    
    IfOps.RecvFromNet = RecvFromNet_Nominal;
    IfOps.RecvFromPeer = NULL;
}/* end RecvNetMsgs_PeerRecv() */

void RecvNetMsgs_NoRecv(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "neither RecvFromPeer nor RecvFromNet defined for net ");

    IfOps.RecvFromNet = NULL;
    IfOps.RecvFromPeer = NULL;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);

    IfOps.RecvFromNet = RecvFromNet_Nominal;
    IfOps.RecvFromPeer = NULL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end RecvNetMsgs_NoRecv() */

void RecvNetMsgs_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);
}/* end RecvNetMsgs_Nominal() */

void Test_SBN_RecvNetMsgs(void)
{
    RecvNetMsgs_NetEmpty();
    RecvNetMsgs_TaskRecv();
    RecvNetMsgs_PeerRecv();
    RecvNetMsgs_NoRecv();
    RecvNetMsgs_Nominal();
}/* end Test_SBN_RecvNetMsgs() */

void SendNetMsg_MutexTakeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "unable to take mutex");

    PeerPtr->SendTaskID = 1;
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemTake), 1, -1);

    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), -1);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SendNetMsg_MutexTakeErr() */

void SendNetMsg_MutexGiveErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "unable to give mutex");

    PeerPtr->SendTaskID = 1;
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemGive), 1, -1);

    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), -1);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SendNetMsg_MutexTakeErr() */

void SendNetMsg_SendErr(void)
{
    START();

    IfOps.Send = Send_Err;
    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), -1);
    UtAssert_INT32_EQ(PeerPtr->SendCnt, 0);
    UtAssert_INT32_EQ(PeerPtr->SendErrCnt, 1);

    IfOps.Send = Send_Nominal;

    SBN_SendNetMsg(0, 0, NULL, PeerPtr);

    UtAssert_INT32_EQ(PeerPtr->SendCnt, 1);
    UtAssert_INT32_EQ(PeerPtr->SendErrCnt, 1);
}/* end SendNetMsg_SendErr() */

void Test_SBN_SendNetMsg(void)
{
    SendNetMsg_MutexTakeErr();
    SendNetMsg_MutexGiveErr();
    SendNetMsg_SendErr();
}/* end Test_SBN_SendNetMsg() */

void UT_Setup(void)
{
}/* end UT_Setup() */

void UT_TearDown(void)
{
}/* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_AppMain);
    ADD_TEST(SBN_ProcessNetMsg);
    ADD_TEST(SBN_Connected);
    ADD_TEST(SBN_Disconnected);
    ADD_TEST(SBN_ReloadConfTbl);
    ADD_TEST(SBN_PackUnpack);
    ADD_TEST(SBN_RecvNetMsgs);
    ADD_TEST(SBN_SendNetMsg);
}
