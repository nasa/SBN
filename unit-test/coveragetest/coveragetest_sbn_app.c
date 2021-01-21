#include "sbn_coveragetest_common.h"
#include "sbn_app.h"
#include "cfe_msgids.h"
#include "cfe_sb_events.h"
#include "sbn_pack.h"

/* #define STUB_TASKID 1073807361 *//* TODO: should be replaced with a call to a stub util fn */
CFE_SB_MsgId_t MsgID = 0x1818;
/********************************** tests ************************************/
static void AppMain_ESRegisterErr(void)
{
    START();
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RegisterApp), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_EVS_Register, 0);
} /* end AppMain_ESRegisterErr() */

static void AppMain_EVSRegisterErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(CFE_EVS_Register), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_ES_GetAppID, 0);
} /* end AppMain_EVSRegisterErr() */

static void AppMain_AppIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to get AppID");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetAppID), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(OS_TaskGetId, 0);
    EVENT_CNT(1);
} /* end AppMain_AppIdErr() */

static void AppMain_TaskInfoErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "SBN failed to get task info (");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetTaskInfo), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_TaskInfoErr() */

/********************************** load conf tbl tests  ************************************/

static void LoadConfTbl_RegisterErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to register conf tbl handle");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConfTbl_RegisterErr() */

static void LoadConfTbl_LoadErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to load conf tbl /cf/sbn_conf_tbl.tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConfTbl_LoadErr() */

static void LoadConfTbl_ManageErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to manage conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConfTbl_ManageErr() */

static void LoadConfTbl_NotifyErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConfTbl_NotifyErr() */

static void Test_AppMain_LoadConfTbl(void)
{
    LoadConfTbl_RegisterErr();
    LoadConfTbl_LoadErr();
    LoadConfTbl_ManageErr();
    LoadConfTbl_NotifyErr();
} /* end Test_AppMain_LoadConfTbl() */

static void LoadConf_Module_ProtoLibFNNull(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=UDP)");

    char tmpFNFirst                                  = NominalTblPtr->ProtocolModules[0].LibFileName[0];
    NominalTblPtr->ProtocolModules[0].LibFileName[0] = '\0';

    SBN_AppMain();

    NominalTblPtr->ProtocolModules[0].LibFileName[0] = tmpFNFirst;

    EVENT_CNT(1);
} /* end LoadConf_Module_ProtoLibFNNull() */

static void LoadConf_Module_FiltLibFNNull(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=CCSDS Endian)");

    char tmpFNFirst                                = NominalTblPtr->FilterModules[0].LibFileName[0];
    NominalTblPtr->FilterModules[0].LibFileName[0] = '\0';

    SBN_AppMain();

    NominalTblPtr->FilterModules[0].LibFileName[0] = tmpFNFirst;

    EVENT_CNT(1);
} /* end LoadConf_Module_FiltLibFNNull() */

static void LoadConf_Module_ModLdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module file (Name=UDP LibFileName=/cf/sbn_udp.so)");

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_Module_ModLdErr() */

static int32 AlwaysErrHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
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

    EVENT_CNT(1);
} /* end LoadConf_Module_SymLookErr() */

static void Test_LoadConf_Module(void)
{
    LoadConf_Module_ProtoLibFNNull();
    LoadConf_Module_FiltLibFNNull();
    LoadConf_Module_ModLdErr();
    LoadConf_Module_SymLookErr();
} /* end Test_LoadConf_Module() */

static void LoadConf_GetAddrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to get conf table address");

    /* make sure it does not return INFO_UPDATED */
    UT_ResetState(UT_KEY(CFE_TBL_GetAddress));
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED - 1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_GetAddrErr() */

static SBN_Status_t ProtoInitErr_InitModule(int ProtocolVersion, CFE_EVS_EventID_t BaseEID, SBN_ProtocolOutlet_t *Outlet)
{
    return 1;
} /* end ProtoInitErr_InitModule */

static void LoadConf_ProtoInitErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "error in protocol init");

    IfOpsPtr->InitModule = ProtoInitErr_InitModule;

    SBN_AppMain();

    IfOpsPtr->InitModule = ProtoInitModule_Nominal;

    EVENT_CNT(1);
} /* end LoadConf_ProtoInitErr() */

static SBN_Status_t FilterInitErr_InitModule(int FilterVersion, CFE_EVS_EventID_t BaseEID)
{
    return 1;
} /* end FilterInitErr_InitModule */

static void LoadConf_FilterInitErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "error in filter init");

    FilterInterfacePtr->InitModule = FilterInitErr_InitModule;

    SBN_AppMain();

    FilterInterfacePtr->InitModule = FilterInitModule_Nominal;

    EVENT_CNT(1);
} /* end LoadConf_FilterInitErr() */

static void LoadConf_ProtoNameErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module name XDP");

    char o                                  = NominalTblPtr->Peers[0].ProtocolName[0];
    NominalTblPtr->Peers[0].ProtocolName[0] = 'X'; /* temporary make it "XDP" */

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    NominalTblPtr->Peers[0].ProtocolName[0] = o;

    EVENT_CNT(1);
} /* end LoadConf_ProtoNameErr() */

static void LoadConf_FiltNameErr(void)
{
    START();

    memset(&SBN, 0, sizeof(SBN)); /* will be loaded by LoadConfTbl() */

    UT_CheckEvent_Setup(SBN_TBL_EID, "Invalid filter name: XCSDS Endian");

    char o                                = NominalTblPtr->Peers[0].Filters[0][0];
    NominalTblPtr->Peers[0].Filters[0][0] = 'X';

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    NominalTblPtr->Peers[0].Filters[0][0] = o;

    EVENT_CNT(1);
} /* end LoadConf_FiltNameErr() */

static void LoadConf_TooManyNets(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "too many networks");

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    NominalTblPtr->Peers[0].NetNum = SBN_MAX_NETS + 1;

    SBN_AppMain();

    NominalTblPtr->Peers[0].NetNum = 0;

    EVENT_CNT(1);
} /* end LoadConf_TooManyNets() */

static void LoadConf_ReleaseAddrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to release address of conf tbl");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_ReleaseAddrErr() */

static void LoadConf_NetCntInc(void)
{
    START();

    SBN.NetCnt = 0;

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    UtAssert_INT32_EQ(SBN.NetCnt, 1);
} /* end LoadConf_NetCntInc() */

static void LoadConf_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_Nominal() */

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
} /* end Test_LoadConf() */

static void AppMain_MutSemCrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "error creating mutex for send tasks");

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1);

    SBN_AppMain();

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 0, 0);
    EVENT_CNT(1);
} /* end AppMain_MutSemCrErr() */

static int32 NoNetsHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    SBN.NetCnt = 0;
    return CFE_SUCCESS;
} /* end NoNetsHook() */

static void InitInt_NoNets(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "no networks configured");

    UT_SetHookFunction(UT_KEY(OS_MutSemCreate), NoNetsHook, NULL);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end InitInt_NoNets() */

static int32 NetConfHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    SBN.Nets[0].Configured = false;
    return CFE_SUCCESS;
} /* end NetConfHook() */

static void InitInt_NetConfErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "network #0 not configured");

    UT_SetHookFunction(UT_KEY(OS_MutSemCreate), NetConfHook, NULL);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end InitInt_NetConfErr() */

static SBN_Status_t RecvFromPeer_Nominal(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr,
                                         SBN_MsgSz_t *MsgSzPtr, CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    return SBN_SUCCESS;
} /* end RecvFromPeer_Nominal() */

static void Test_InitInt(void)
{
    InitInt_NoNets();
    InitInt_NetConfErr();
} /* end Test_InitInt() */

static void AppMain_SubPipeCrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create subscription pipe (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SB_BAD_ARGUMENT); /* fail just after InitInterfaces() */

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeCrErr() */

static void AppMain_SubPipeAllSubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to allsubs (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeAllSubErr() */

static void AppMain_SubPipeOneSubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to sub (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeOneSubErr() */

static void AppMain_CmdPipeCrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create command pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_CmdPipeCrErr() */

static void AppMain_CmdPipeSubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to command pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_CmdPipeSubErr() */

static void SBStart_CrPipeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create event pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 3, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end SBStart_CrPipeErr() */

static void SBStart_SubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to event pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end SBStart_SubErr() */

static void SBStart_RcvMsgErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    UT_CheckEvent_Setup(SBN_MSG_EID, "err from rcvmsg on sub pipe");

    /* first time through, CheckSubscriptionPipe should generate an error */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_PIPE_RD_ERR);

    /* second time through CheckSubscriptionPipe will be nominal operation */
    CFE_SB_SingleSubscriptionTlm_t Msg, *MsgPtr;
    MsgPtr = &Msg;
    memset(MsgPtr, 0, sizeof(Msg));
    Msg.Payload.SubType = CFE_SB_SUBSCRIPTION;
    Msg.Payload.MsgId   = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &MsgPtr, sizeof(MsgPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* second call to SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStartup() */

    SBN_AppMain();

    EVENT_CNT(1);

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end SBStart_RcvMsgErr() */

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
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    /* ...GetMsgId() -> sub msg */
    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* WaitForSBStartup()... */
    /* ...CFE_SB_ReceiveBuffer() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &EvtMsgPtr, sizeof(EvtMsgPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    /* ...GetMsgId() -> event msg */
    mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStartup() */

    SBN_AppMain();

    EVENT_CNT(1);
} /* end SBStart_UnsubErr() */

static void SBStart_RcvMsgSucc(void)
{
    START();

    /* call to SBN_CheckSubscriptionPipe should fail, giving us the event we are looking for */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_PIPE_RD_ERR); /* sub pipe err */

    /* ...CFE_SB_ReceiveBuffer() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &EvtMsgPtr, sizeof(EvtMsgPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    /* ...GetMsgId() -> event msg */
    CFE_SB_MsgId_t mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Unsubscribe), 1, CFE_SB_BAD_ARGUMENT); /* fail out of WaitForSBStartup() */

    SBN_AppMain();
} /* end SBStart_RcvMsgSucc() */

static void SBStart_DelPipeErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* SBN_CheckSubscriptionPipe() ...*/
    /* ...RcvMsg() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);
    /* ...GetMsgId() -> sub msg */
    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* WaitForSBStartup()...*/
    /* ...CFE_SB_ReceiveBuffer() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &EvtMsgPtr, sizeof(EvtMsgPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    /* ...GetMsgId() -> event msg */
    mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_DeletePipe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end SBStart_DelPipeErr() */

static void Test_SBStart(void)
{
    SBStart_CrPipeErr();
    SBStart_SubErr();
    SBStart_RcvMsgErr();
    SBStart_UnsubErr();
    SBStart_RcvMsgSucc();
    SBStart_DelPipeErr();
} /* end Test_SBStart() */

static void W4W_NoMsg(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to delete event pipe");

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_NO_MESSAGE);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_NO_MESSAGE);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end W4W_NoMsg() */

static int32 PeerConnHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    SBN.Nets[0].Peers[1].Connected = true;
    return ProcessorID;
} /* end PeerConnHook() */

static void CheckPP_SendTask(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_SEND;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    UT_SetHookFunction(UT_KEY(CFE_PSP_GetProcessorId), PeerConnHook, NULL);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_POLL;

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end CheckPP_SendTask() */

static void CheckPP_SendTaskErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_SEND;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, -1);

    UT_SetHookFunction(UT_KEY(CFE_PSP_GetProcessorId), PeerConnHook, NULL);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_POLL;

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end CheckPP_SendTaskErr() */

static void CheckPP_RecvErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 3, -1);

    UT_SetHookFunction(UT_KEY(CFE_PSP_GetProcessorId), PeerConnHook, NULL);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end CheckPP_RecvErr() */

static SBN_Status_t FilterSend_Empty(void *MsgBuf, SBN_Filter_Ctx_t *Context)
{
    return SBN_IF_EMPTY;
} /* end FilterSend_Empty() */

static void CheckPP_FilterEmpty(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    PeerPtr->Connected = 1;

    SBN_FilterInterface_t FilterEmpty, FilterNull;
    memset(&FilterNull, 0, sizeof(FilterNull));
    memset(&FilterEmpty, 0, sizeof(FilterEmpty));
    FilterEmpty.FilterSend = FilterSend_Empty;

    PeerPtr->Filters[0] = &FilterNull;
    PeerPtr->Filters[1] = &FilterEmpty;
    PeerPtr->FilterCnt  = 2;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end CheckPP_FilterEmpty() */

static SBN_Status_t FilterSend_Err(void *MsgBuf, SBN_Filter_Ctx_t *Context)
{
    return SBN_ERROR;
} /* end FilterSend_Empty() */

static void CheckPP_FilterErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    PeerPtr->Connected = 1;

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterSend = FilterSend_Err;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt  = 1;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end CheckPP_FilterErr() */

static SBN_Status_t FilterSend_Nominal(void *MsgBuf, SBN_Filter_Ctx_t *Context)
{
    return SBN_SUCCESS;
} /* end FilterSend_Nominal() */

static void CheckPP_Nominal(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    PeerPtr->Connected = 1;

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterSend = FilterSend_Nominal;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt  = 1;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end CheckPP_Nominal() */

static void W4W_RcvMsgErr(void)
{
    START();

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_PIPE_RD_ERR);

    SBN_AppMain();
} /* end W4W_RcvMsgErr() */

static void PeerPoll_RecvNetTask_ChildTaskErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for net ");

    SBN.NetCnt          = 0;
    SBN.Nets[0].PeerCnt = 0;

    PeerPtr->Connected                = 1;
    NominalTblPtr->Peers[0].TaskFlags = SBN_TASK_RECV;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, -1);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    EVENT_CNT(1);

    NominalTblPtr->Peers[0].TaskFlags = SBN_TASK_POLL;

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end PeerPoll_RecvNetTask_ChildTaskErr() */

static void PeerPoll_RecvNetTask_Nominal(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    SBN.NetCnt          = 0;
    SBN.Nets[0].PeerCnt = 0;

    PeerPtr->Connected                = 1;
    NominalTblPtr->Peers[0].TaskFlags = SBN_TASK_RECV;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    NominalTblPtr->Peers[0].TaskFlags = SBN_TASK_POLL;

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end PeerPoll_RecvNetTask_Nominal() */

static void PeerPoll_RecvPeerTask_ChildTaskErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for ");

    SBN.NetCnt          = 0;
    SBN.Nets[0].PeerCnt = 0;

    PeerPtr->Connected                = 1;
    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_RECV;

    IfOpsPtr->RecvFromPeer = RecvFromPeer_Nominal;
    IfOpsPtr->RecvFromNet  = NULL;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, -1);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    EVENT_CNT(1);

    IfOpsPtr->RecvFromPeer            = NULL;
    IfOpsPtr->RecvFromNet             = RecvFromNet_Nominal;
    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_POLL;

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end PeerPoll_RecvPeerTask_ChildTaskErr() */

static osal_task test_osal_task_entry(void)
{
    /* do nothing */
} /* end osal_task_entry() */

static void PeerPoll_RecvPeerTask_Nominal(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    SBN.NetCnt          = 0;
    SBN.Nets[0].PeerCnt = 0;

    PeerPtr->Connected                = 1;
    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_RECV;

    IfOpsPtr->RecvFromPeer = RecvFromPeer_Nominal;
    IfOpsPtr->RecvFromNet  = NULL;

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);
    OS_TaskCreate(&PeerPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);
    /* PeerPtr->RecvTaskID = STUB_TASKID; */

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    IfOpsPtr->RecvFromPeer            = NULL;
    IfOpsPtr->RecvFromNet             = RecvFromNet_Nominal;
    NominalTblPtr->Peers[1].TaskFlags = SBN_TASK_POLL;

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end PeerPoll_RecvPeerTask() */

static void Test_WaitForWakeup(void)
{
    W4W_NoMsg();
    W4W_RcvMsgErr();
    CheckPP_SendTask();
    CheckPP_SendTaskErr();
    CheckPP_RecvErr();
    CheckPP_FilterEmpty();
    CheckPP_FilterErr();
    CheckPP_Nominal();
    PeerPoll_RecvNetTask_ChildTaskErr();
    PeerPoll_RecvNetTask_Nominal();
    PeerPoll_RecvPeerTask_ChildTaskErr();
    PeerPoll_RecvPeerTask_Nominal();
} /* end Test_SBStart() */

static void AppMain_Nominal(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ONESUB_TLM_MID;
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* CFE_SB_ReceiveBuffer() in WaitForSBStartup() should succeed */
    CFE_EVS_LongEventTlm_t EvtMsg, *EvtMsgPtr;
    EvtMsgPtr = &EvtMsg;
    memset(EvtMsgPtr, 0, sizeof(EvtMsg));
    strcpy(EvtMsg.Payload.PacketID.AppName, "CFE_SB");
    EvtMsg.Payload.PacketID.EventID = CFE_SB_INIT_EID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &EvtMsgPtr, sizeof(EvtMsgPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    mid = CFE_EVS_LONG_EVENT_MSG_MID;
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end AppMain_Nominal() */

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
} /* end Test_SBN_AppMain() */

static void ProcessNetMsg_PeerErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unknown peer (ProcessorID=");

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID + 1, 0, NULL), SBN_ERROR);

    EVENT_CNT(1);
} /* ProcessNetMsg_PeerErr() */

static void ProcessNetMsg_ProtoMsg_VerErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version mismatch with ProcessorID ");

    uint8 ver = SBN_PROTO_VER + 1;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID, sizeof(ver), &ver), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end ProcessNetMsg_ProtoMsg_VerErr() */

static void ProcessNetMsg_ProtoMsg_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version match with ProcessorID ");

    uint8 ver = SBN_PROTO_VER;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID, sizeof(ver), &ver), SBN_SUCCESS);

    EVENT_CNT(1);
} /* end ProcessNetMsg_ProtoMsg_Nominal() */

static SBN_Status_t RecvFilter_Err(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_ERROR;
} /* end RecvFilter_Err() */

static void ProcessNetMsg_AppMsg_FiltErr(void)
{
    START();

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterRecv = RecvFilter_Err;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt  = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_ERROR);
} /* end ProcessNetMsg_AppMsg_FiltErr() */

static SBN_Status_t RecvFilter_Out(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_IF_EMPTY;
} /* end RecvFilter_Out() */

static void ProcessNetMsg_AppMsg_FiltOut(void)
{
    START();

    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterRecv = RecvFilter_Out;

    PeerPtr->ProcessorID  = ProcessorID;
    PeerPtr->SpacecraftID = SpacecraftID;
    PeerPtr->Filters[0]   = &Filter;
    PeerPtr->FilterCnt    = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_IF_EMPTY);
} /* end ProcessNetMsg_AppMsg_FiltOut() */

static SBN_Status_t RecvFilter_Nominal(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_SUCCESS;
} /* end RecvFilter_Nominal() */

static void ProcessNetMsg_AppMsg_PassMsgErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_SB_EID, "CFE_SB_PassMsg error (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_TransmitMsg), 1, -1);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_ERROR);

    EVENT_CNT(1);
} /* end ProcessNetMsg_AppMsg_PassMsgErr() */

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
    PeerPtr->FilterCnt  = 2;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, 0, NULL), SBN_SUCCESS);
} /* end ProcessNetMsg_AppMsg_Nominal() */

static void ProcessNetMsg_SubMsg_Nominal(void)
{
    START();

    uint8  Buf[SBN_PACKED_SUB_SZ];
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
} /* end ProcessNetMsg_SubMsg_Nominal() */

static void ProcessNetMsg_UnSubMsg_Nominal(void)
{
    START();

    uint8  Buf[SBN_PACKED_SUB_SZ];
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
} /* end ProcessNetMsg_UnSubMsg_Nominal() */

static void ProcessNetMsg_NoMsg_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_NO_MSG, ProcessorID, 0, NULL), SBN_SUCCESS);
} /* end ProcessNetMsg_NoMsg_Nominal() */

static void ProcessNetMsg_MsgErr(void)
{
    START();

    /* send a net message of an invalid type */
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_NO_MSG + 100, ProcessorID, 0, NULL), SBN_ERROR);
} /* end ProcessNetMsg_MsgErr() */

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
} /* end Test_SBN_ProcessNetMsg() */

static void Connected_AlreadyErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "CPU 1234 already connected");

    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    UtAssert_INT32_EQ(PeerPtr->Connected, 1);
    EVENT_CNT(1);
} /* end Connected_AlreadyErr() */

static void Connected_PipeOptErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "failed to set pipe options '");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SetPipeOpts), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    EVENT_CNT(1);
} /* end Connected_PipeOptErr() */

static void Connected_SendErr(void)
{
    START();

    IfOpsPtr->Send = Send_Err;

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);

    IfOpsPtr->Send = Send_Nominal;
} /* end Connected_SendErr() */

static void Connected_CrPipeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "failed to create pipe '");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    EVENT_CNT(1);
} /* end Connected_CrPipeErr() */

static void Connected_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_SUCCESS);

    UtAssert_INT32_EQ(PeerPtr->Connected, 1);
} /* end Connected_Nominal() */

static void Test_SBN_Connected(void)
{
    Connected_AlreadyErr();
    Connected_CrPipeErr();
    Connected_PipeOptErr();
    Connected_SendErr();
    Connected_Nominal();
} /* end Test_SBN_Connected() */

static void Disconnected_ConnErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "CPU 1234 not connected");

    SBN_PeerInterface_t *PeerPtr = &SBN.Nets[0].Peers[0];

    PeerPtr->ProcessorID = ProcessorID;

    UtAssert_INT32_EQ(SBN_Disconnected(PeerPtr), SBN_ERROR);
    EVENT_CNT(1);
} /* end Disconnected_Nominal() */

static void Disconnected_Nominal(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "CPU 1234 disconnected");

    SBN_PeerInterface_t *PeerPtr = &SBN.Nets[0].Peers[0];

    PeerPtr->ProcessorID = ProcessorID;
    PeerPtr->Connected   = 1;

    UtAssert_INT32_EQ(SBN_Disconnected(PeerPtr), SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->Connected, 0);
    EVENT_CNT(1);
} /* end Disconnected_Nominal() */

static void Test_SBN_Disconnected(void)
{
    Disconnected_ConnErr();
    Disconnected_Nominal();
} /* end Test_SBN_Disconnected() */

static SBN_Status_t UnloadNet_Err(SBN_NetInterface_t *Net)
{
    return SBN_ERROR;
} /* end UnloadNet_Nominal() */

static void ReloadConfTbl_UnloadNetErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload network ");

    IfOpsPtr->UnloadNet = UnloadNet_Err;

    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    EVENT_CNT(1);

    IfOpsPtr->UnloadNet = UnloadNet_Nominal;
} /* end ReloadConfTbl_UnloadNetErr() */

static void ReloadConfTbl_ProtoUnloadErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload protocol module ID ");

    PeerPtr->Connected = 1;

    SBN.ProtocolModules[0] = 1;

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    EVENT_CNT(1);
} /* end ReloadConfTbl_ProtoUnloadErr() */

static void ReloadConfTbl_FiltUnloadErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload filter module ID ");

    SBN.FilterModules[0] = 1;

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    EVENT_CNT(1);
} /* end ReloadConfTbl_FiltUnloadErr() */

static void ReloadConfTbl_TblUpdErr(void)
{
    START();

    PeerPtr->Connected = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Update), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);
} /* end ReloadConfTbl_TblUpdErr() */

static void ReloadConfTbl_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_SUCCESS);
} /* end ReloadConfTbl_Nominal() */

static void Test_SBN_ReloadConfTbl(void)
{
    ReloadConfTbl_UnloadNetErr();
    ReloadConfTbl_ProtoUnloadErr();
    ReloadConfTbl_FiltUnloadErr();
    ReloadConfTbl_TblUpdErr();
    ReloadConfTbl_Nominal();
} /* end Test_SBN_ReloadConfTbl() */

static void Unpack_Empty(void)
{
    START();

    uint8             Buf[SBN_MAX_PACKED_MSG_SZ] = {0}, Payload[1] = {0};
    SBN_MsgSz_t       MsgSz;
    SBN_MsgType_t     MsgType;
    CFE_ProcessorID_t ProcID;

    SBN_PackMsg(Buf, 0, SBN_APP_MSG, ProcessorID, NULL);
    UtAssert_True(SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, Payload), "unpack of an empty pack");
    UtAssert_INT32_EQ(MsgSz, 0);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(ProcID, ProcessorID);
} /* end Unpack_Empty() */

static void Unpack_Err(void)
{
    START();

    uint8             Buf[SBN_MAX_PACKED_MSG_SZ] = {0}, Payload[1] = {0};
    SBN_MsgSz_t       MsgSz;
    SBN_MsgType_t     MsgType;
    CFE_ProcessorID_t ProcID;

    Pack_t Pack;
    Pack_Init(&Pack, Buf, SBN_MAX_PACKED_MSG_SZ + SBN_PACKED_HDR_SZ, 0);
    Pack_Int16(&Pack, -1); /* invalid msg size */
    Pack_UInt8(&Pack, SBN_APP_MSG);
    Pack_UInt32(&Pack, ProcessorID);

    UtAssert_True(!SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, Payload), "unpack of invalid pack");
} /* end Unpack_Err() */

static void Unpack_Nominal(void)
{
    START();

    uint8             Buf[SBN_MAX_PACKED_MSG_SZ] = {0}, Payload[1] = {0};
    uint8             TestData = 123;
    SBN_MsgSz_t       MsgSz;
    SBN_MsgType_t     MsgType;
    CFE_ProcessorID_t ProcID;

    SBN_PackMsg(Buf, 1, SBN_APP_MSG, ProcessorID, &TestData);

    UtAssert_True(SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, Payload), "unpack of a pack");

    UtAssert_INT32_EQ(MsgSz, 1);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(ProcID, ProcessorID);
    UtAssert_INT32_EQ((int32)TestData, (int32)Payload[0]);
} /* end Unpack_Nominal() */

static void Test_SBN_PackUnpack(void)
{
    Unpack_Empty();
    Unpack_Err();
    Unpack_Nominal();
} /* end Test_SBN_PackUnpack() */

void RecvNetMsgs_TaskRecv(void)
{
    START();

    NetPtr->TaskFlags = SBN_TASK_RECV;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);
} /* end RecvNetMsgs_TaskRecv() */

static SBN_Status_t RecvFromNet_Empty(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
                                      CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    *ProcessorIDPtr = 1235;

    return SBN_IF_EMPTY;
} /* end RecvFromNet_Empty() */

void RecvNetMsgs_NetEmpty(void)
{
    START();

    IfOpsPtr->RecvFromNet = RecvFromNet_Empty;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetMsgs_NetEmpty() */

void RecvNetMsgs_PeerRecv(void)
{
    START();

    IfOpsPtr->RecvFromNet  = NULL;
    IfOpsPtr->RecvFromPeer = RecvFromPeer_Nominal;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);

    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
    IfOpsPtr->RecvFromPeer = NULL;
} /* end RecvNetMsgs_PeerRecv() */

void RecvNetMsgs_NoRecv(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "neither RecvFromPeer nor RecvFromNet defined for net ");

    IfOpsPtr->RecvFromNet  = NULL;
    IfOpsPtr->RecvFromPeer = NULL;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);

    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
    IfOpsPtr->RecvFromPeer = NULL;

    EVENT_CNT(1);
} /* end RecvNetMsgs_NoRecv() */

void RecvNetMsgs_Nominal(void)
{
    START();

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);
} /* end RecvNetMsgs_Nominal() */

void Test_SBN_RecvNetMsgs(void)
{
    RecvNetMsgs_NetEmpty();
    RecvNetMsgs_TaskRecv();
    RecvNetMsgs_PeerRecv();
    RecvNetMsgs_NoRecv();
    RecvNetMsgs_Nominal();
} /* end Test_SBN_RecvNetMsgs() */

static void RecvPeerTask_RegChildErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unable to register child task");

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RegisterChildTask), 1, -1);

    SBN_RecvPeerTask();

    EVENT_CNT(1);
} /* end RecvPeerTask_RegChildErr() */

static void RecvPeerTask_NetConfErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unable to connect task to peer struct");

    NetPtr->Configured = false;

    SBN_RecvPeerTask();

    EVENT_CNT(1);
} /* end RecvPeerTask_NetConfErr() */

static SBN_Status_t RecvFromPeer_EmptyOne(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr,
                                          SBN_MsgSz_t *MsgSzPtr, CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    static int c = 0;

    if (c++ == 0)
        return SBN_IF_EMPTY;
    return SBN_ERROR;
} /* end RecvFromPeer_EmptyOne() */

static void RecvPeerTask_Empty(void)
{
    START();

    IfOpsPtr->RecvFromNet  = NULL;
    IfOpsPtr->RecvFromPeer = RecvFromPeer_EmptyOne;

    OS_TaskCreate(&PeerPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvPeerTask();

    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
    IfOpsPtr->RecvFromPeer = NULL;
} /* end RecvPeerTask_Empty() */

static SBN_Status_t RecvFromPeer_One(SBN_NetInterface_t *Net, SBN_PeerInterface_t *Peer, SBN_MsgType_t *MsgTypePtr,
                                     SBN_MsgSz_t *MsgSzPtr, CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    static int c = 0;

    if (c++ == 0)
        return SBN_SUCCESS;
    return SBN_ERROR;
} /* end RecvFromPeer_One() */

static void RecvPeerTask_Nominal(void)
{
    START();

    IfOpsPtr->RecvFromNet  = NULL;
    IfOpsPtr->RecvFromPeer = RecvFromPeer_One;

    OS_TaskCreate(&PeerPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvPeerTask();

    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
    IfOpsPtr->RecvFromPeer = NULL;
} /* end RecvPeerTask_Nominal() */

static void Test_SBN_RecvPeerTask(void)
{
    RecvPeerTask_RegChildErr();
    RecvPeerTask_NetConfErr();
    RecvPeerTask_Empty();
    RecvPeerTask_Nominal();
} /* end Test_SBN_RecvPeerTask() */

static void RecvNetTask_RegChildErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unable to register child task");

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RegisterChildTask), 1, -1);

    SBN_RecvNetTask();

    EVENT_CNT(1);
} /* end RecvNetTask_RegChildErr() */

static void RecvNetTask_NetConfErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unable to connect task to net struct");

    NetPtr->Configured = false;

    SBN_RecvNetTask();

    EVENT_CNT(1);
} /* end RecvNetTask_NetConfErr() */

static SBN_Status_t RecvFromNet_EmptyOne(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
                                         CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    static int c = 0;

    if (c++ == 0)
        return SBN_IF_EMPTY;
    return SBN_ERROR;
} /* end RecvFromNet_EmptyOne() */

static void RecvNetTask_Empty(void)
{
    START();

    IfOpsPtr->RecvFromNet = RecvFromNet_EmptyOne;

    OS_TaskCreate(&NetPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvNetTask();

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetTask_Empty() */

static SBN_Status_t RecvFromNet_BadPeer(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
                                        CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    *ProcessorIDPtr = 0;

    return SBN_SUCCESS;
} /* end RecvFromNet_BadPeer() */

static void RecvNetTask_PeerErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unknown peer (ProcessorID=0)");

    IfOpsPtr->RecvFromNet = RecvFromNet_BadPeer;

    OS_TaskCreate(&NetPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvNetTask();

    EVENT_CNT(1);

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetTask_PeerErr() */

static SBN_Status_t RecvFromNet_One(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
                                    CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    static int c = 0;

    if (c++ == 0)
    {
        *MsgTypePtr     = SBN_NO_MSG + 10; /* bogus type */
        *ProcessorIDPtr = 1234;
        return SBN_SUCCESS;
    } /* end if */
    return SBN_ERROR;
} /* end RecvFromNet_One() */

static void RecvNetTask_Nominal(void)
{
    START();

    IfOpsPtr->RecvFromNet = RecvFromNet_One;

    OS_TaskCreate(&NetPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvNetTask();

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetTask_Nominal() */

static void Test_SBN_RecvNetTask(void)
{
    RecvNetTask_RegChildErr();
    RecvNetTask_NetConfErr();
    RecvNetTask_Empty();
    RecvNetTask_PeerErr();
    RecvNetTask_Nominal();
} /* end Test_SBN_RecvNetTask() */

static void SendTask_RegChildErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unable to register child task");

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RegisterChildTask), 1, -1);

    SBN_SendTask();

    EVENT_CNT(1);
} /* end SendTask_RegChildErr() */

static void SendTask_ConnTaskErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    PeerPtr->Connected  = true;
    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, -1);

    SBN_SendTask();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end SendTask_ConnTaskErr() */

static int32 TaskDelayConn(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    static int c = 0;

    if (c++ > 0)
    {
        SBN.Nets[0].Peers[0].Connected = true;
    } /* end if */

    return CFE_SUCCESS;
}

static void SendTask_PeerNotConn(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    PeerPtr->Connected  = false;
    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, -1);

    UT_SetHookFunction(UT_KEY(OS_TaskDelay), TaskDelayConn, NULL);

    SBN_SendTask();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end SendTask_PeerNotConn() */

static SBN_Status_t SendFilter_Err(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_ERROR;
} /* end SendFilter_Err() */

static void SendTask_FiltErr(void)
{
    START();

    PeerPtr->Connected  = true;
    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_FilterInterface_t Filter_Err;
    memset(&Filter_Err, 0, sizeof(Filter_Err));
    Filter_Err.FilterSend = SendFilter_Err;

    /* Filters[0].Recv is NULL, should skip */
    PeerPtr->Filters[0] = &Filter_Err;
    PeerPtr->FilterCnt  = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, -1);

    SBN_SendTask();
} /* end SendTask_FiltErr() */

static SBN_Status_t SendFilter_Out(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_IF_EMPTY;
} /* end SendFilter_Out() */

static SBN_Status_t SendFilter_Nominal(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_SUCCESS;
} /* end SendFilter_Nominal() */

static void SendTask_Filters(void)
{
    START();

    PeerPtr->Connected  = true;
    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_FilterInterface_t Filter_Empty, Filter_Nominal, Filter_Out;
    memset(&Filter_Empty, 0, sizeof(Filter_Empty));
    memset(&Filter_Nominal, 0, sizeof(Filter_Nominal));
    memset(&Filter_Out, 0, sizeof(Filter_Out));
    Filter_Nominal.FilterSend = SendFilter_Nominal;
    Filter_Out.FilterSend     = SendFilter_Out;

    /* Filters[0].Recv is NULL, should skip */
    PeerPtr->Filters[0] = &Filter_Empty;
    PeerPtr->Filters[1] = &Filter_Nominal;
    PeerPtr->Filters[2] = &Filter_Out;
    PeerPtr->FilterCnt  = 3;

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, -1);

    SBN_SendTask();
} /* end SendTask_Filters() */

static void SendTask_SendNetMsgErr(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    PeerPtr->Connected  = true;
    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    IfOpsPtr->Send = Send_Err;

    SBN_SendTask();

    UtAssert_INT32_EQ(PeerPtr->SendTaskID, OS_TaskGetId());

    IfOpsPtr->Send = Send_Nominal;

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end SendTask_SendNetMsgErr() */

static void SendTask_Nominal(void)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    PeerPtr->Connected  = true;
    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, -1);

    SBN_SendTask();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end SendTask_Nominal() */

static void Test_SBN_SendTask(void)
{
    SendTask_RegChildErr();
    SendTask_ConnTaskErr();
    SendTask_PeerNotConn();
    SendTask_FiltErr();
    SendTask_Filters();
    SendTask_SendNetMsgErr();
    SendTask_Nominal();
} /* end Test_SBN_SendTask() */

void SendNetMsg_MutexTakeErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "unable to take mutex");

    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemTake), 1, -1);

    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), SBN_ERROR);

    EVENT_CNT(1);
} /* end SendNetMsg_MutexTakeErr() */

void SendNetMsg_MutexGiveErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_PEER_EID, "unable to give mutex");

    OS_TaskCreate(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemGive), 1, -1);

    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), SBN_ERROR);

    EVENT_CNT(1);
} /* end SendNetMsg_MutexTakeErr() */

void SendNetMsg_SendErr(void)
{
    START();

    IfOpsPtr->Send = Send_Err;
    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), SBN_ERROR);
    UtAssert_INT32_EQ(PeerPtr->SendCnt, 0);
    UtAssert_INT32_EQ(PeerPtr->SendErrCnt, 1);

    IfOpsPtr->Send = Send_Nominal;

    SBN_SendNetMsg(0, 0, NULL, PeerPtr);

    UtAssert_INT32_EQ(PeerPtr->SendCnt, 1);
    UtAssert_INT32_EQ(PeerPtr->SendErrCnt, 1);
} /* end SendNetMsg_SendErr() */

void Test_SBN_SendNetMsg(void)
{
    SendNetMsg_MutexTakeErr();
    SendNetMsg_MutexGiveErr();
    SendNetMsg_SendErr();
} /* end Test_SBN_SendNetMsg() */

void UT_Setup(void) {} /* end UT_Setup() */

void UT_TearDown(void) {} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_AppMain);
    ADD_TEST(SBN_ProcessNetMsg);
    ADD_TEST(SBN_Connected);
    ADD_TEST(SBN_Disconnected);
    ADD_TEST(SBN_ReloadConfTbl);
    ADD_TEST(SBN_PackUnpack);
    ADD_TEST(SBN_RecvNetMsgs);
    ADD_TEST(SBN_RecvPeerTask);
    ADD_TEST(SBN_RecvNetTask);
    ADD_TEST(SBN_SendTask);
    ADD_TEST(SBN_SendNetMsg);
}
