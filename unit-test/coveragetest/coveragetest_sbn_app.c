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
    return SBN_SUCCESS;
}/* end Send_Nominal() */

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
            .ProcessorID = 0,
            .SpacecraftID = 0,
            .NetNum = 0,
            .ProtocolName = "UDP",
            .Filters = {
                "CCSDS Endian"
            },
            .Address = "127.0.0.1:2234",
            .TaskFlags = SBN_TASK_POLL
        },
        { /* [1] */
            .ProcessorID = 1,
            .SpacecraftID = 0,
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

/********************************** tests ************************************/

#define START UT_ResetState(0); printf("Start item %s (%d)\n", __func__, __LINE__); memset(&SBN, 0, sizeof(SBN))

static void AppMain_ESRegisterErr(void)
{
    START;
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RegisterApp), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_EVS_Register, 0);
}/* end AppMain_ESRegisterErr() */

static void AppMain_EVSRegisterErr(void)
{
    START;

    UT_SetDeferredRetcode(UT_KEY(CFE_EVS_Register), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_ES_GetAppID, 0);
}/* end AppMain_EVSRegisterErr() */

static void AppMain_AppIdErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to get AppID");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetAppID), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(OS_TaskGetId, 0);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_AppIdErr() */

static void AppMain_TaskInfoErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "SBN failed to get task info (");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetTaskInfo), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_TaskInfoErr() */

/********************************** load conf tbl tests  ************************************/

static void LoadConfTbl_RegisterErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to register conf tbl handle");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_RegisterErr() */

static void LoadConfTbl_LoadErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to load conf tbl /cf/sbn_conf_tbl.tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_LoadErr() */

static void LoadConfTbl_ManageErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to manage conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConfTbl_ManageErr() */

static void LoadConfTbl_NotifyErr(void)
{
    START;

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

static void LoadConf_Module_ProtoLibFNNull(void)
{
    START;

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
    START;

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
    START;

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module file (Name=UDP LibFileName=/cf/sbn_udp.so)");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

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
    START;

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
    START;

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
    START;

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
    START;

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
    START;

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module name XDP");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    char o = NominalTbl.Peers[0].ProtocolName[0];
    NominalTbl.Peers[0].ProtocolName[0] = 'X'; /* temporary make it "XDP" */

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    NominalTbl.Peers[0].ProtocolName[0] = o;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_ProtoNameErr() */

static void LoadConf_TooManyNets(void)
{
    START;

    UT_CheckEvent_Setup(SBN_TBL_EID, "too many networks");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    int i = 2;
    for (; i < SBN_MAX_NETS + 1; i++)
    {
        strcpy(NominalTbl.Peers[i].ProtocolName, "UDP");
        NominalTbl.Peers[i].NetNum = i;
        NominalTbl.Peers[i].ProcessorID = 0;
        NominalTbl.Peers[i].SpacecraftID = 0;
    }/* end for */
    NominalTbl.PeerCnt = i;

    SBN_AppMain();

    NominalTbl.PeerCnt = 2;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_TooManyNets() */

static void LoadConf_ReleaseAddrErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to release address of conf tbl");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, -1);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end LoadConf_ReleaseAddrErr() */

static void LoadConf_Nominal(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
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
    LoadConf_TooManyNets();
    LoadConf_ReleaseAddrErr();
    LoadConf_Nominal();
}/* end Test_LoadConf() */

static void AppMain_MutSemCrErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

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
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "no networks configured");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);
    UT_SetHookFunction(UT_KEY(OS_MutSemCreate), NoNetsHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

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
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "network #0 not configured");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);
    UT_SetHookFunction(UT_KEY(OS_MutSemCreate), NetConfHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetConfErr() */

static void InitInt_NetChildErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for net 0");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, CFE_ES_ERR_CHILD_TASK_CREATE);

    NominalTbl.Peers[0].TaskFlags = SBN_TASK_RECV;

    SBN_AppMain();

    NominalTbl.Peers[0].TaskFlags = SBN_TASK_POLL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetChildErr() */

static void InitInt_NetChildSuccess(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "configured, 1 nets");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1); /* fail just after InitInterfaces() */

    NominalTbl.Peers[0].TaskFlags = SBN_TASK_RECV;

    SBN_AppMain();

    NominalTbl.Peers[0].TaskFlags = SBN_TASK_POLL;

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end InitInt_NetChildSuccess() */

static SBN_Status_t RecvFromPeer_Nominal(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr, CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    return SBN_SUCCESS;
}/* end RecvFromPeer_Nominal() */

static void InitInt_PeerChildRecvErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for 1");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
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
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating send task for 1");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
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
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "configured, 1 nets");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
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
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create subscription pipe (Status=");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SB_BAD_ARGUMENT); /* fail just after InitInterfaces() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_SubPipeCrErr() */

static void AppMain_SubPipeAllSubErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to allsubs (Status=");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_SubPipeAllSubErr() */

static void AppMain_SubPipeOneSubErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to sub (Status=");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_SubPipeOneSubErr() */

static void AppMain_CmdPipeCrErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create command pipe (");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_CmdPipeCrErr() */

static void AppMain_CmdPipeSubErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to command pipe (");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end AppMain_CmdPipeSubErr() */

static void SBStart_CrPipeErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create event pipe (");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 3, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_CrPipeErr() */

static void SBStart_SubErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to event pipe (");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_SubErr() */

static void SBStart_RcvMsgErr(void)
{
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = 0x1234;

    START;

    UT_CheckEvent_Setup(SBN_MSG_EID, "err from rcvmsg on sub pipe");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    /* first call to SBN_CheckSubscriptionPipe should fail, giving us the event we are looking for */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_PIPE_RD_ERR); /* sub pipe err */

    /* ...but WaitForSBStart() keeps trying the sub pipe, so go through the loop again */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_PIPE_RD_ERR);

    /* second time through CheckSubscriptionPipe will be nominal operation */
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* second call to SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), false);


    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStart() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_RcvMsgErr() */

static void SBStart_UnsubErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to unsubscribe from event messages");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = 0x1234;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* CFE_SB_RcvMsg() in WaitForSBStart() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &EvtMsgPtr, sizeof(EvtMsgPtr), false);

    mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStart() */

    SBN_AppMain();

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end SBStart_UnsubErr() */

static void Test_SBStart(void)
{
    SBStart_CrPipeErr();
    SBStart_SubErr();
    SBStart_RcvMsgErr();
    SBStart_UnsubErr();
}/* end Test_SBStart() */

static void Test_CheckSubPipe(void)
{
}/* end Test_CheckSubPipe() */

static void AppMain_Nominal(void)
{
    START;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    /* CFE_SB_RcvMsg() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId = 0x1234;
    UT_SetDataBuffer(UT_KEY(CFE_SB_RcvMsg), &SubRprtPtr, sizeof(SubRprtPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_SB_GetMsgId), &mid, sizeof(mid), true);

    /* CFE_SB_RcvMsg() in WaitForSBStart() should succeed */
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

    Test_CheckSubPipe();

    AppMain_Nominal();
}/* end Test_SBN_AppMain() */

static void ProcessNetMsg_ProtoMsg_VerErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version mismatch with ProcessorID ");

    SBN_NetInterface_t Net;
    uint8 ver = SBN_PROTO_VER + 1;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_PROTO_MSG, 1234, sizeof(ver), &ver), SBN_SUCCESS);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ProcessNetMsg_ProtoMsg_VerErr() */

static void ProcessNetMsg_ProtoMsg_Nominal(void)
{
    START;

    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version match with ProcessorID ");

    SBN_NetInterface_t Net;
    uint8 ver = SBN_PROTO_VER;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_PROTO_MSG, 1234, sizeof(ver), &ver), SBN_SUCCESS);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ProcessNetMsg_ProtoMsg_Nominal() */

static SBN_Status_t RecvFilter_Err(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_ERROR;
}/* end RecvFilter_Err() */

static void ProcessNetMsg_AppMsg_FiltErr(void)
{
    START;

    SBN_NetInterface_t Net;
    SBN_FilterInterface_t Filter;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.Peers[0].SpacecraftID = 5678;
    Filter.FilterRecv = RecvFilter_Err;
    Net.Peers[0].Filters[0] = &Filter;
    Net.Peers[0].FilterCnt = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1234);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 5678);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_APP_MSG, 1234, 0, NULL), SBN_ERROR);
}/* end ProcessNetMsg_AppMsg_FiltErr() */

static SBN_Status_t RecvFilter_Out(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_IF_EMPTY;
}/* end RecvFilter_Out() */

static void ProcessNetMsg_AppMsg_FiltOut(void)
{
    START;

    SBN_NetInterface_t Net;
    SBN_FilterInterface_t Filter;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.Peers[0].SpacecraftID = 5678;
    Filter.FilterRecv = RecvFilter_Out;
    Net.Peers[0].Filters[0] = &Filter;
    Net.Peers[0].FilterCnt = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1234);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 5678);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_APP_MSG, 1234, 0, NULL), SBN_IF_EMPTY);
}/* end ProcessNetMsg_AppMsg_FiltOut() */

static SBN_Status_t RecvFilter_Nominal(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_SUCCESS;
}/* end RecvFilter_Nominal() */

static void ProcessNetMsg_AppMsg_PassMsgErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_SB_EID, "CFE_SB_PassMsg error (Status=");

    SBN_NetInterface_t Net;
    SBN_FilterInterface_t Filter_Empty, Filter_Nominal;

    memset(&Net, 0, sizeof(Net));
    memset(&Filter_Empty, 0, sizeof(Filter_Empty));
    memset(&Filter_Nominal, 0, sizeof(Filter_Nominal));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.Peers[0].SpacecraftID = 5678;
    /* Filters[0].Recv is NULL, should skip */
    Net.Peers[0].Filters[0] = &Filter_Empty;
    Filter_Nominal.FilterRecv = RecvFilter_Nominal;
    Net.Peers[0].Filters[1] = &Filter_Nominal;
    Net.Peers[0].FilterCnt = 2;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1234);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 5678);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_PassMsg), 1, -1);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_APP_MSG, 1234, 0, NULL), SBN_ERROR);

    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end ProcessNetMsg_AppMsg_PassMsgErr() */

static void ProcessNetMsg_AppMsg_Nominal(void)
{
    START;

    SBN_NetInterface_t Net;
    SBN_FilterInterface_t Filter_Empty, Filter_Nominal;

    memset(&Net, 0, sizeof(Net));
    memset(&Filter_Empty, 0, sizeof(Filter_Empty));
    memset(&Filter_Nominal, 0, sizeof(Filter_Nominal));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.Peers[0].SpacecraftID = 5678;
    /* Filters[0].Recv is NULL, should skip */
    Net.Peers[0].Filters[0] = &Filter_Empty;
    Filter_Nominal.FilterRecv = RecvFilter_Nominal;
    Net.Peers[0].Filters[1] = &Filter_Nominal;
    Net.Peers[0].FilterCnt = 2;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1234);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 5678);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_APP_MSG, 1234, 0, NULL), SBN_SUCCESS);
}/* end ProcessNetMsg_AppMsg_Nominal() */

static void ProcessNetMsg_SubMsg_Nominal(void)
{
    START;

    uint8 Buf[SBN_PACKED_SUB_SZ];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, sizeof(Buf), 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);

    Pack_MsgID(&Pack, 0xDEAD);
    CFE_SB_Qos_t QoS;
    Pack_Data(&Pack, &QoS, sizeof(QoS));

    SBN_NetInterface_t Net;
    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.Peers[0].SpacecraftID = 5678;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1234);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 5678);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_SUB_MSG, 1234, sizeof(Buf), &Buf), SBN_SUCCESS);
    UtAssert_INT32_EQ(Net.Peers[0].Subs[0].MsgID, 0xDEAD);
}/* end ProcessNetMsg_SubMsg_Nominal() */

static void ProcessNetMsg_UnSubMsg_Nominal(void)
{
    START;

    uint8 Buf[SBN_PACKED_SUB_SZ];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, sizeof(Buf), 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);

    Pack_MsgID(&Pack, 0xDEAD);
    CFE_SB_Qos_t QoS;
    Pack_Data(&Pack, &QoS, sizeof(QoS));

    SBN_NetInterface_t Net;
    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.Peers[0].SpacecraftID = 5678;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1234);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 5678);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_SUB_MSG, 1234, sizeof(Buf), &Buf), SBN_SUCCESS);
    UtAssert_INT32_EQ(Net.Peers[0].SubCnt, 1);
    UtAssert_INT32_EQ(Net.Peers[0].Subs[0].MsgID, 0xDEAD);
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_UNSUB_MSG, 1234, sizeof(Buf), &Buf), SBN_SUCCESS);
    UtAssert_INT32_EQ(Net.Peers[0].SubCnt, 0);
}/* end ProcessNetMsg_UnSubMsg_Nominal() */

static void ProcessNetMsg_NoMsg_Nominal(void)
{
    START;

    SBN_NetInterface_t Net;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_NO_MSG, 1234, 0, NULL), SBN_SUCCESS);
}/* end ProcessNetMsg_NoMsg_Nominal() */

static void ProcessNetMsg_MsgErr(void)
{
    START;

    SBN_NetInterface_t Net;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;

    /* send a net message of an invalid type */
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(&Net, SBN_NO_MSG + 100, 1234, 0, NULL), SBN_ERROR);
}/* end ProcessNetMsg_MsgErr() */

static void Test_SBN_ProcessNetMsg(void)
{
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
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "CPU 1234 already connected");

    SBN_NetInterface_t Net;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.IfOps = &IfOps;
    Net.Peers[0].Net = &Net;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    Net.Peers[0].Connected = 1;

    UtAssert_INT32_EQ(SBN_Connected(&Net.Peers[0]), SBN_ERROR);
    UtAssert_INT32_EQ(Net.Peers[0].Connected, 1);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Connected_AlreadyErr() */

static void Connected_PipeOptErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "failed to set pipe options '");

    SBN_NetInterface_t Net;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.IfOps = &IfOps;
    Net.Peers[0].Net = &Net;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SetPipeOpts), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(&Net.Peers[0]), SBN_ERROR);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Connected_PipeOptErr() */

static SBN_MsgSz_t Send_Err(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload)
{
    return SBN_ERROR;
}/* end Send_Nominal() */

static void Connected_SendErr(void)
{
    START;

    SBN_NetInterface_t Net;

    IfOps.Send = Send_Err;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.IfOps = &IfOps;
    Net.Peers[0].Net = &Net;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UtAssert_INT32_EQ(SBN_Connected(&Net.Peers[0]), SBN_ERROR);

    IfOps.Send = Send_Nominal;
}/* end Connected_SendErr() */

static void Connected_CrPipeErr(void)
{
    START;

    UT_CheckEvent_Setup(SBN_PEER_EID, "failed to create pipe '");

    SBN_NetInterface_t Net;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.IfOps = &IfOps;
    Net.Peers[0].Net = &Net;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(&Net.Peers[0]), SBN_ERROR);
    UtAssert_True(EventTest.MatchCount == 1, "EID generated (%d)", EventTest.MatchCount);
}/* end Connected_CrPipeErr() */

static void Connected_Nominal(void)
{
    START;

    SBN_NetInterface_t Net;

    memset(&Net, 0, sizeof(Net));
    Net.PeerCnt = 1;
    Net.Peers[0].ProcessorID = 1234;
    Net.IfOps = &IfOps;
    Net.Peers[0].Net = &Net;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UtAssert_INT32_EQ(SBN_Connected(&Net.Peers[0]), SBN_SUCCESS);
    UtAssert_INT32_EQ(Net.Peers[0].Connected, 1);
}/* end Connected_Nominal() */

static void Test_SBN_Connected(void)
{
    Connected_AlreadyErr();
    Connected_CrPipeErr();
    Connected_PipeOptErr();
    Connected_SendErr();
    Connected_Nominal();
}/* end Test_SBN_Connected() */

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
    #if 0
    ADD_TEST(SBN_Disconnected);
    ADD_TEST(SBN_ReloadConfTbl);
    #endif
}
