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

#include "sbn_coveragetest_common.h"
#include "sbn_app.h"
#include "cfe_msgids.h"
#include "cfe_sb_eventids.h"
#include "cfe_evs_msg.h"
#include "sbn_pack.h"
#include "sbn_error.h"

/* #define STUB_TASKID 1073807361 */ /* TODO: should be replaced with a call to a stub util fn */
CFE_SB_MsgId_t MsgID = { .Value = 0x1818 };

static SBN_ConfTbl_t TestConfTbl;

/********************************** tests ************************************/
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

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: unable to get AppID");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetAppID), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(OS_TaskGetId, 0);
    EVENT_CNT(1);
} /* end AppMain_AppIdErr() */

static void AppMain_TaskInfoErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: SBN failed to get task info (");
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

static void CFE_TBL_GetAddress_Hook(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    void **PtrOut = (void **)UT_Hook_GetArgValueByName(Context, "TblPtr", void **);

    if (PtrOut != NULL)
    {
        *PtrOut = &TestConfTbl;
    }
}

static void LoadConf_Module_ProtoLibFNNull(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need exactly one protocol module to test the empty LibFileName error
     * - FilterCnt: 0 - Skip filter processing, focus only on protocol module loading
     * - PeerCnt: 0 - No peers needed for this module loading test
     * Expected Error: "invalid module" when protocol LibFileName is empty/null */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName, "\0", sizeof(TestConfTbl.ProtocolModules[0].LibFileName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=UDP)");

    char tmpFNFirst                                  = NominalTblPtr->ProtocolModules[0].LibFileName[0];
    NominalTblPtr->ProtocolModules[0].LibFileName[0] = '\0';

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);

    SBN_AppMain();

    NominalTblPtr->ProtocolModules[0].LibFileName[0] = tmpFNFirst;

    EVENT_CNT(1);
} /* end LoadConf_Module_ProtoLibFNNull() */

static void LoadConf_Module_FiltLibFNNull(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - Skip protocol processing entirely
     * - FilterCnt: 1 - Need exactly one filter module to test the empty LibFileName error
     * - PeerCnt: 0 - No peers needed for this module loading test
     * Expected Error: "invalid module" when filter LibFileName is empty/null */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0;
    TestConfTbl.FilterCnt   = 1;
    strncpy(TestConfTbl.FilterModules[0].Name, "CCSDS Endian", sizeof(TestConfTbl.FilterModules[0].Name));
    strncpy(TestConfTbl.FilterModules[0].LibSymbol, "CCSDS_Ops", sizeof(TestConfTbl.FilterModules[0].LibSymbol));
    TestConfTbl.FilterModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.FilterModules[0].LibFileName, "\0", sizeof(TestConfTbl.FilterModules[0].LibFileName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module (Name=CCSDS Endian)");

    char tmpFNFirst                                = NominalTblPtr->FilterModules[0].LibFileName[0];
    NominalTblPtr->FilterModules[0].LibFileName[0] = '\0';

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);

    SBN_AppMain();

    NominalTblPtr->FilterModules[0].LibFileName[0] = tmpFNFirst;

    EVENT_CNT(1);
} /* end LoadConf_Module_FiltLibFNNull() */

static void LoadConf_Module_ModLdErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one protocol module to test OS_ModuleLoad failure
     * - FilterCnt: 0 - Skip filter processing, focus only on protocol module loading
     * - PeerCnt: 0 - No peers needed for this module loading test
     * Expected Error: "invalid module file" when OS_ModuleLoad fails */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    TestConfTbl.FilterCnt   = 0;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module file (Name=UDP LibFileName=/cf/sbn_udp.so)");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_Module_ModLdErr() */

static int32 AlwaysErrHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    return 1;
}

static void LoadConf_Module_SymLookErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one protocol module to test OS_SymbolLookup failure
     * - FilterCnt: 0 - Skip filter processing, focus only on protocol module loading
     * - PeerCnt: 0 - No peers needed for this module loading test
     * Expected Error: "invalid symbol" when OS_SymbolLookup fails after successful module load */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    TestConfTbl.FilterCnt   = 0;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid symbol (Name=UDP LibSymbol=SBN_UDP_Ops)");

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), AlwaysErrHook, NULL);

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);

    SBN_AppMain();

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

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to get conf table address");

    /* make sure it does not return INFO_UPDATED */
    UT_ResetState(UT_KEY(CFE_TBL_GetAddress));
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED - 1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_GetAddrErr() */

static SBN_Status_t
ProtoInitErr_InitModule(int ProtocolVersion, CFE_EVS_EventID_t BaseEID, SBN_ProtocolOutlet_t *Outlet)
{
    return SBN_ERROR;
} /* end ProtoInitErr_InitModule */

static void OS_SymbolLookup_Hook(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    void **PtrOut = (void **)UT_Hook_GetArgValueByName(Context, "symbol_address", void **);

    if (PtrOut != NULL)
    {
        *PtrOut = IfOpsPtr;
    }
}

static void LoadConf_ProtoInitErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one protocol module to test InitModule failure
     * - FilterCnt: 0 - Skip filter processing, focus only on protocol initialization
     * - PeerCnt: 0 - No peers needed for this module initialization test
     * Expected Error: "error in protocol init" when protocol InitModule returns error */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    TestConfTbl.FilterCnt   = 0;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "error in protocol init");

    IfOpsPtr->InitModule = ProtoInitErr_InitModule;

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);

    SBN_AppMain();

    IfOpsPtr->InitModule = ProtoInitModule_Nominal;

    EVENT_CNT(1);
} /* end LoadConf_ProtoInitErr() */

static SBN_Status_t FilterInitErr_InitModule(int FilterVersion, CFE_EVS_EventID_t BaseEID)
{
    return SBN_ERROR;
} /* end FilterInitErr_InitModule */

static void OS_FilterSymbolLookup_Hook(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    void **PtrOut = (void **)UT_Hook_GetArgValueByName(Context, "symbol_address", void **);

    if (PtrOut != NULL)
    {
        *PtrOut = FilterInterfacePtr;
    }
}

static void LoadConf_FilterInitErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - Skip protocol processing, focus only on filter initialization
     * - FilterCnt: 1 - Need one filter module to test InitModule failure
     * - PeerCnt: 0 - No peers needed for this module initialization test
     * Expected Error: "error in filter init" when filter InitModule returns error */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0;
    TestConfTbl.FilterCnt   = 1;
    strncpy(TestConfTbl.FilterModules[0].Name, "CCSDS Endian", sizeof(TestConfTbl.FilterModules[0].Name));
    strncpy(TestConfTbl.FilterModules[0].LibSymbol, "CCSDS_Ops", sizeof(TestConfTbl.FilterModules[0].LibSymbol));
    TestConfTbl.FilterModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.FilterModules[0].LibFileName, "\0", sizeof(TestConfTbl.FilterModules[0].LibFileName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "error in filter init");

    FilterInterfacePtr->InitModule = FilterInitErr_InitModule;

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_FilterSymbolLookup_Hook, NULL);

    SBN_AppMain();

    FilterInterfacePtr->InitModule = FilterInitModule_Nominal;

    EVENT_CNT(1);
} /* end LoadConf_FilterInitErr() */

static void LoadConf_ProtoNameErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one valid protocol module ("UDP")
     * - FilterCnt: 0 - Skip filter processing, focus on peer protocol validation
     * - PeerCnt: 1 - Need one peer that references an invalid protocol name
     * Expected Error: "invalid module name XDP" when peer references non-existent protocol */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 1;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    TestConfTbl.Peers[0].SpacecraftID = 1234;
    TestConfTbl.Peers[0].ProcessorID  = 5678;
    strncpy(TestConfTbl.Peers[0].ProtocolName, "XDP", sizeof(TestConfTbl.Peers[0].ProtocolName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "invalid module name XDP");

    char o                                  = NominalTblPtr->Peers[0].ProtocolName[0];
    NominalTblPtr->Peers[0].ProtocolName[0] = 'X'; /* temporary make it "XDP" */

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);
    // UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    NominalTblPtr->Peers[0].ProtocolName[0] = o;

    EVENT_CNT(1);
} /* end LoadConf_ProtoNameErr() */

static void LoadConf_FiltNameErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 2 - Need at least one valid protocol module for peer reference
     * - FilterCnt: 1 - Need one valid filter module ("CCSDS_Endian")
     * - PeerCnt: 1 - Need one peer that references an invalid filter name
     * Expected Error: "Invalid filter name" when peer references non-existent filter */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 2;
    TestConfTbl.FilterCnt   = 1;
    TestConfTbl.PeerCnt     = 1;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    strncpy(TestConfTbl.FilterModules[0].Name, "CCSDS_Endian", sizeof(TestConfTbl.FilterModules[0].Name));
    strncpy(TestConfTbl.FilterModules[0].LibSymbol, "SBN_CCSDS_Ops", sizeof(TestConfTbl.FilterModules[0].LibSymbol));
    strncpy(TestConfTbl.FilterModules[0].LibFileName,
            "/cf/sbn_ccsds.so",
            sizeof(TestConfTbl.FilterModules[0].LibFileName));
    TestConfTbl.FilterModules[0].BaseEID = 2000;
    TestConfTbl.Peers[0].SpacecraftID    = 1234;
    TestConfTbl.Peers[0].ProcessorID     = 5678;
    TestConfTbl.Peers[0].NetNum          = 0;
    strncpy(TestConfTbl.Peers[0].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[0].ProtocolName));
    strncpy((char *)TestConfTbl.Peers[0].Address, "127.0.0.1:1234", sizeof(TestConfTbl.Peers[0].Address));
    strncpy(TestConfTbl.Peers[0].Filters[0], "INVALID_FILTER", sizeof(TestConfTbl.Peers[0].Filters[0]));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "Invalid filter name");

    NominalTblPtr->Peers[0].Filters[0][0] = 'X';

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);
    // UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1); /* fail just after LoadConfTbl() */

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_FiltNameErr() */

static void LoadConf_TooManyNets(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 2 - Need at least one valid protocol module for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on network validation
     * - PeerCnt: 1 - Need one peer with NetNum that exceeds maximum allowed
     * Expected Error: "network index too large" when peer NetNum > SBN_MAX_NETS */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 2;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 1;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    TestConfTbl.Peers[0].SpacecraftID = 1234;
    TestConfTbl.Peers[0].ProcessorID  = 5678;
    TestConfTbl.Peers[0].NetNum       = SBN_MAX_NETS + 1;
    strncpy(TestConfTbl.Peers[0].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[0].ProtocolName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "network index too large");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_TooManyNets() */

static void LoadConf_ReleaseAddrErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 2 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on table release error
     * - PeerCnt: 1 - Need at least one peer to get past peer processing to table release
     * Expected Error: "unable to release address of conf tbl" when CFE_TBL_ReleaseAddress fails */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 2;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 1;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    TestConfTbl.Peers[0].SpacecraftID = 1234;
    TestConfTbl.Peers[0].ProcessorID  = 5678;
    TestConfTbl.Peers[0].NetNum       = 0;
    strncpy(TestConfTbl.Peers[0].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[0].ProtocolName));

    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to release address of conf tbl");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_ReleaseAddrErr() */

static void LoadConf_NetCntInc(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 2 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on network count increase
     * - PeerCnt: 1 - Need one peer to trigger network creation and count increase
     * Expected Info Event: "increasing net count to" when NetCnt is increased */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 2;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 1;
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    TestConfTbl.Peers[0].SpacecraftID = 1234;
    TestConfTbl.Peers[0].ProcessorID  = 5678;
    TestConfTbl.Peers[0].NetNum       = 0;
    strncpy(TestConfTbl.Peers[0].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[0].ProtocolName));

    START();

    UT_ResetState(0);
    SBN_AppData.NetCnt = 0;
    UT_CheckEvent_Setup(SBN_TBL_EID, "increasing net count to");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_NetCntInc() */

static void LoadConf_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: error creating mutex for send tasks");

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

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: error creating mutex for send tasks");

    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_MutSemCrErr() */

static int32 NoNetsHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    SBN_AppData.NetCnt = 0;
    return CFE_SUCCESS;
} /* end NoNetsHook() */

static void InitInt_NoNets(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocol modules needed since we're testing the "no networks" error condition
     * - FilterCnt: 0 - No filter modules needed for this network validation test
     * - PeerCnt: 0 - No peers configured to ensure NetCnt remains 0, triggering the expected error
     *
     * Additionally, SBN_AppData.NetCnt is explicitly set to 0 and maintained that way using NoNetsHook
     * to test the "no networks configured" error path in InitInterfaces */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 0;

    START();

    UT_ResetState(0);
    SBN_AppData.NetCnt = 0;
    UT_CheckEvent_Setup(SBN_PEER_EID, "no networks configured");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);
    UT_SetHookFunction(UT_KEY(OS_MutSemCreate), NoNetsHook, NULL);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end InitInt_NoNets() */

/* Mock IfOps structure for testing */
static SBN_Status_t Mock_UnloadNet(SBN_NetInterface_t *Net)
{
    return SBN_SUCCESS;
}

static SBN_Status_t Mock_InitNet(SBN_NetInterface_t *Net)
{
    return SBN_SUCCESS;
}

static SBN_Status_t Mock_InitPeer(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
}

static SBN_IfOps_t MockIfOps = { .InitModule   = NULL,
                                 .InitNet      = Mock_InitNet,
                                 .InitPeer     = Mock_InitPeer,
                                 .LoadNet      = NULL,
                                 .LoadPeer     = NULL,
                                 .PollPeer     = NULL,
                                 .Send         = NULL,
                                 .RecvFromNet  = NULL,
                                 .RecvFromPeer = NULL,
                                 .UnloadNet    = Mock_UnloadNet,
                                 .UnloadPeer   = NULL };

/* Hook function to modify SBN after LoadConf completes but before InitInterfaces */
static int32 SetupNetConfErr_Hook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    /* At this point LoadConf has completed, modify SBN to have an unconfigured network */
    SBN_AppData.NetCnt = 1;
    memset(&SBN_AppData.Nets[0], 0, sizeof(SBN_AppData.Nets[0]));
    SBN_AppData.Nets[0].Configured = false;      /* This will trigger the error in InitInterfaces */
    SBN_AppData.Nets[0].IfOps      = &MockIfOps; /* Prevent segfault in UnloadNets */
    SBN_AppData.Nets[0].PeerCnt    = 0;          /* No peers */

    return StubRetcode;
}

static void InitInt_NetConfErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocol modules needed since we're artificially creating the error condition
     * - FilterCnt: 0 - No filter modules needed for this network configuration validation test
     * - PeerCnt: 0 - No peers needed since SetupNetConfErr_Hook will artificially create an unconfigured network
     *
     * The hook function SetupNetConfErr_Hook modifies SBN after LoadConf completes to create
     * a network with Configured=false, simulating a network that failed to initialize properly.
     * This tests InitInterfaces' ability to detect and report unconfigured networks. */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0; /* No protocols needed */
    TestConfTbl.FilterCnt   = 0; /* No filters needed */
    TestConfTbl.PeerCnt     = 0; /* No peers needed */

    UT_ResetState(0);

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_PEER_EID, "network #0 not configured");

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* THE KEY HOOK: Modify SBN structure after LoadConf completes */
    UT_SetHookFunction(UT_KEY(CFE_TBL_ReleaseAddress), SetupNetConfErr_Hook, NULL);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, CFE_SUCCESS);

    /* Set up mutex creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 2, OS_SUCCESS);

    /* Set up command pipe creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SUCCESS);

    /* Let LoadConfTbl succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    /* Run the main function */
    SBN_AppMain();

    /* Verify the expected event was generated */
    EVENT_CNT(1);
} /* end InitInt_NetConfErr() */

static SBN_Status_t RecvFromPeer_Nominal(SBN_NetInterface_t  *Net,
                                         SBN_PeerInterface_t *Peer,
                                         SBN_MsgType_t       *MsgTypePtr,
                                         SBN_MsgSz_t         *MsgSzPtr,
                                         CFE_ProcessorID_t   *ProcessorIDPtr,
                                         CFE_SpacecraftID_t  *SpacecraftIDPtr,
                                         void                *PayloadBuffer)
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

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on subscription pipe creation failure
     * - FilterCnt: 0 - No filters needed for pipe creation error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach SetupSubPipe where error occurs
     * This allows LoadConf and InitInterfaces to succeed, then tests subscription pipe creation failure */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0; /* No protocols needed */
    TestConfTbl.FilterCnt   = 0; /* No filters needed */
    TestConfTbl.PeerCnt     = 0; /* No peers needed */

    UT_ResetState(0);

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create subscription pipe (Status=");

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Let LoadConfTbl succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    /* Set up command pipe creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SB_BAD_ARGUMENT); /* fail just after InitInterfaces() */

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeCrErr() */

static void AppMain_SubPipeAllSubErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on subscription to all-subscriptions failure
     * - FilterCnt: 0 - No filters needed for subscription error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach CFE_SB_SubscribeLocal failure
     * This allows pipe creation to succeed, then tests the first subscription (allsubs) failure */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0; /* No protocols needed */
    TestConfTbl.FilterCnt   = 0; /* No filters needed */
    TestConfTbl.PeerCnt     = 0; /* No peers needed */

    UT_ResetState(0);

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to allsubs (Status=");

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Let LoadConfTbl succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    /* Set up command pipe creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeAllSubErr() */

static void AppMain_SubPipeOneSubErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on subscription to single-subscription failure
     * - FilterCnt: 0 - No filters needed for subscription error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach second CFE_SB_SubscribeLocal failure
     * This allows pipe creation and first subscription to succeed, then tests second subscription failure */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0; /* No protocols needed */
    TestConfTbl.FilterCnt   = 0; /* No filters needed */
    TestConfTbl.PeerCnt     = 0; /* No peers needed */

    UT_ResetState(0);

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to sub (Status=");

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Let LoadConfTbl succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    /* Set up command pipe creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeOneSubErr() */

static void AppMain_CmdPipeCrErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on command pipe creation failure
     * - FilterCnt: 0 - No filters needed for command pipe error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach command pipe creation
     * This allows LoadConf to succeed, then tests the very first CFE_SB_CreatePipe failure */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0; /* No protocols needed */
    TestConfTbl.FilterCnt   = 0; /* No filters needed */
    TestConfTbl.PeerCnt     = 0; /* No peers needed */

    UT_ResetState(0);

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: failed to create command pipe (");

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Let LoadConfTbl succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_CmdPipeCrErr() */

static void AppMain_CmdPipeSubErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on command pipe subscription failure
     * - FilterCnt: 0 - No filters needed for command pipe subscription error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach command pipe subscription
     * This allows command pipe creation to succeed, then tests CFE_SB_Subscribe failure */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 0; /* No protocols needed */
    TestConfTbl.FilterCnt   = 0; /* No filters needed */
    TestConfTbl.PeerCnt     = 0; /* No peers needed */

    UT_ResetState(0);

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: failed to subscribe to command pipe (");

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Let LoadConfTbl succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_CmdPipeSubErr() */

static void SBStart_DelPipeErr(void)
{
    START();

    UT_ResetState(0);
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
    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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
    mid = CFE_SB_ValueToMsgId(CFE_EVS_LONG_EVENT_MSG_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_DeletePipe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end SBStart_DelPipeErr() */

static void Test_SBStart(void)
{
    SBStart_DelPipeErr();
} /* end Test_SBStart() */

static void W4W_NoMsg(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_INIT_EID, "unable to delete event pipe");

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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
    SBN_AppData.Nets[0].Peers[1].Connected = true;
    return ProcessorID;
} /* end PeerConnHook() */

static void CheckPP_SendTask(void)
{
    START();

    UT_ResetState(0);
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

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    UT_ResetState(0);
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

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 2 - Two filters to test different filter behaviors
     * - Filters[0]: FilterNull (NULL FilterSend function) - should be skipped
     * - Filters[1]: FilterEmpty (returns SBN_IF_EMPTY) - should filter out the message
     * - PeerPtr->Connected = 1 - Peer must be connected for message processing to occur */
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

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 1 - Single filter to test error condition
     * - Filters[0]: Filter with FilterSend_Err (returns SBN_ERROR) - triggers filter error handling
     * - PeerPtr->Connected = 1 - Peer must be connected for message processing to occur */
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

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 1 - Single filter to test successful processing
     * - Filters[0]: Filter with FilterSend_Nominal (returns SBN_SUCCESS) - allows message through
     * - PeerPtr->Connected = 1 - Peer must be connected for message processing to occur */
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

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    UT_ResetState(0);

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_PIPE_RD_ERR);

    SBN_AppMain();
} /* end W4W_RcvMsgErr() */

static void PeerPoll_RecvNetTask_ChildTaskErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one protocol module (UDP) to create a network interface
     * - FilterCnt: 0 - No filters needed for task creation error testing
     * - PeerCnt: 1 - Need one peer configured as local processor to create a network
     *
     * Peer Configuration:
     * - SpacecraftID: 41 (same as local) - Makes this peer represent the local processor
     * - ProcessorID: 1 (same as local) - Confirms this is the local processor's network
     * - TaskFlags: SBN_TASK_RECV - Enables network-level receive task creation
     * - IfOpsPtr->RecvFromNet: RecvFromNet_Nominal - Must be set to trigger task creation */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 1;

    /* Set up protocol module */
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;

    /* Set up peer that will be this processor (creates network) */
    TestConfTbl.Peers[0].SpacecraftID = 41; /* Same as local */
    TestConfTbl.Peers[0].ProcessorID  = 1;  /* Same as local */
    TestConfTbl.Peers[0].NetNum       = 0;
    TestConfTbl.Peers[0].TaskFlags    = SBN_TASK_RECV; /* Enable receive task */
    strncpy(TestConfTbl.Peers[0].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[0].ProtocolName));
    snprintf((char *)TestConfTbl.Peers[0].Address, sizeof(TestConfTbl.Peers[0].Address), "127.0.0.1:1234");

    START();

    UT_ResetState(0);

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for net");

    /* Set up the interface operations to have the necessary functions */
    IfOpsPtr->InitModule  = ProtoInitModule_Nominal; /* Use your existing nominal function */
    IfOpsPtr->LoadNet     = LoadNet_Nominal;         /* Use your existing nominal function */
    IfOpsPtr->InitNet     = InitNet_Nominal;         /* Use your existing nominal function */
    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;     /* This is key - enables receive task creation */

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Set up OS_SymbolLookup to succeed for module loading */
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);

    /* Set up CFE_TBL_ReleaseAddress to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, CFE_SUCCESS);

    /* Set up processor/spacecraft IDs to match this "peer" entry */
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 41);

    /* Set up mutex creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 2, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemTake), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemGive), 1, OS_SUCCESS);

    /* Set up pipe creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 2, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 2, CFE_SUCCESS);

    /* Set up table functions to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* Set up main loop handling */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, true);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 2, false);

    /* Set up WaitForWakeup to complete quickly */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_TIME_OUT);

    /* Set up SBN_CheckSubscriptionPipe to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, CFE_SB_NO_MESSAGE);

    /* THIS IS THE KEY SETUP: Make CFE_ES_CreateChildTask fail for receive task creation */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, CFE_ES_ERR_CHILD_TASK_CREATE);

    /* Run the main function */
    SBN_AppMain();

    /* Restore the interface operations */
    IfOpsPtr->InitModule  = ProtoInitModule_Nominal;
    IfOpsPtr->LoadNet     = LoadNet_Nominal;
    IfOpsPtr->InitNet     = InitNet_Nominal;
    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;

    /* Verify the expected event was generated */
    EVENT_CNT(1);
} /* end PeerPoll_RecvNetTask_ChildTaskErr() */

static void PeerPoll_RecvNetTask_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Runtime Configuration:
     * - SBN_AppData.NetCnt: 0 - Reset to ensure clean state for network counting
     * - SBN_AppData.Nets[0].PeerCnt: 0 - Reset to ensure clean state for peer counting
     * - PeerPtr->Connected: 1 - Peer must be connected for task creation to be considered
     * - NominalTblPtr->Peers[0].TaskFlags: SBN_TASK_RECV - Enable network receive task creation */
    SBN_AppData.NetCnt          = 0;
    SBN_AppData.Nets[0].PeerCnt = 0;

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

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one protocol module (UDP) to create a network interface
     * - FilterCnt: 0 - No filters needed for task creation error testing
     * - PeerCnt: 2 - Need two peers: one local (creates network) and one remote with task flag
     *
     * Peer Configuration:
     * - Peers[0]: Local peer (SpacecraftID=41, ProcessorID=1, TaskFlags=0) - Creates network
     * - Peers[1]: Remote peer (SpacecraftID=42, ProcessorID=2, TaskFlags=SBN_TASK_RECV) - For task creation test
     * - IfOpsPtr->RecvFromPeer: RecvFromPeer_Nominal - Must be set to trigger task creation
     * - IfOpsPtr->RecvFromNet: NULL - Disables network-level receive task creation */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 2;

    /* Set up protocol module */
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;

    /* Set up local peer (creates network) */
    TestConfTbl.Peers[0].SpacecraftID = 41;
    TestConfTbl.Peers[0].ProcessorID  = 1;
    TestConfTbl.Peers[0].NetNum       = 0;
    TestConfTbl.Peers[0].TaskFlags    = 0;
    strncpy(TestConfTbl.Peers[0].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[0].ProtocolName));
    snprintf((char *)TestConfTbl.Peers[0].Address, sizeof(TestConfTbl.Peers[0].Address), "127.0.0.1:1234");

    /* Set up remote peer with receive task flag */
    TestConfTbl.Peers[1].SpacecraftID = 42;
    TestConfTbl.Peers[1].ProcessorID  = 2;
    TestConfTbl.Peers[1].NetNum       = 0;
    TestConfTbl.Peers[1].TaskFlags    = SBN_TASK_RECV;
    strncpy(TestConfTbl.Peers[1].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[1].ProtocolName));
    snprintf((char *)TestConfTbl.Peers[1].Address, sizeof(TestConfTbl.Peers[1].Address), "127.0.0.1:5678");

    START();

    UT_ResetState(0);

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_PEER_EID, "error creating task for");

    /* Set up the interface operations */
    IfOpsPtr->InitModule   = ProtoInitModule_Nominal;
    IfOpsPtr->LoadNet      = LoadNet_Nominal;
    IfOpsPtr->InitNet      = InitNet_Nominal;
    IfOpsPtr->LoadPeer     = LoadPeer_Nominal;
    IfOpsPtr->InitPeer     = InitPeer_Nominal;
    IfOpsPtr->RecvFromPeer = RecvFromPeer_Nominal;
    IfOpsPtr->RecvFromNet  = NULL;

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Set up OS_SymbolLookup to succeed for module loading */
    UT_SetDeferredRetcode(UT_KEY(OS_SymbolLookup), 1, 0);
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);

    /* Set up CFE_TBL_ReleaseAddress to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, CFE_SUCCESS);

    /* Set up processor/spacecraft IDs to match local peer */
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 41);

    /* Set up mutex creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemCreate), 2, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemTake), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemGive), 1, OS_SUCCESS);

    /* Set up pipe creation to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 2, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 2, CFE_SUCCESS);

    /* Set up table functions to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Manage), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, CFE_SUCCESS);

    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
    /* SBN_CheckSubscriptionPipe should succeed to return a sub msg */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* Set up main loop handling */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, true);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 2, false);

    /* Set up WaitForWakeup to complete quickly - command pipe timeout */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_TIME_OUT);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, CFE_SB_NO_MESSAGE);

    /* THIS IS THE KEY SETUP: Make CFE_ES_CreateChildTask fail for peer receive task creation */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_CreateChildTask), 1, CFE_ES_ERR_CHILD_TASK_CREATE);

    /* Run the main function */
    SBN_AppMain();

    /* Restore the interface operations */
    IfOpsPtr->RecvFromPeer = NULL;
    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;

    /* Verify the expected event was generated */
    EVENT_CNT(1);
} /* end PeerPoll_RecvPeerTask_ChildTaskErr() */

static osal_task test_osal_task_entry(void)
{
    /* do nothing */
} /* end osal_task_entry() */

static void PeerPoll_RecvPeerTask_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Runtime Configuration:
     * - SBN_AppData.NetCnt: 0 - Reset to ensure clean state for network counting
     * - SBN_AppData.Nets[0].PeerCnt: 0 - Reset to ensure clean state for peer counting
     * - PeerPtr->Connected: 1 - Peer must be connected for task creation to be considered
     * - NominalTblPtr->Peers[1].TaskFlags: SBN_TASK_RECV - Enable peer-level receive task creation
     * - IfOpsPtr->RecvFromPeer: RecvFromPeer_Nominal - Must be set to trigger peer task creation
     * - IfOpsPtr->RecvFromNet: NULL - Disables network-level receive task creation */
    SBN_AppData.NetCnt          = 0;
    SBN_AppData.Nets[0].PeerCnt = 0;

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

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);
    /* PeerPtr->RecvTaskID = STUB_TASKID; */

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* CFE_SB_ReceiveBuffer() in SBN_CheckSubscriptionPipe() should succeed */
    CFE_SB_SingleSubscriptionTlm_t SubRprt, *SubRprtPtr;
    SubRprtPtr = &SubRprt;
    memset(SubRprtPtr, 0, sizeof(SubRprt));
    SubRprt.Payload.SubType = CFE_SB_SUBSCRIPTION;
    SubRprt.Payload.MsgId   = MsgID;
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &SubRprtPtr, sizeof(SubRprtPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    CFE_SB_MsgId_t mid = CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID);
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

    mid = CFE_SB_ValueToMsgId(CFE_EVS_LONG_EVENT_MSG_MID);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetMsgId), &mid, sizeof(mid), true);

    /* go through main loop once */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, 0);

    SBN_AppMain();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end AppMain_Nominal() */

static void Test_SBN_AppMain(void)
{
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

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "ERROR: could not process peer message: unknown peer (ProcessorID=");

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID + 1, SpacecraftID, 0, NULL), SBN_ERROR);

    EVENT_CNT(1);
} /* ProcessNetMsg_PeerErr() */

static void ProcessNetMsg_ProtoMsg_VerErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_SB_EID, "ERROR: could not process peer message: SBN protocol version mismatch with peer ");

    uint8 ver = SBN_PROTO_VER + 1;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID, SpacecraftID, sizeof(ver), &ver),
                      SBN_SUCCESS);

    EVENT_CNT(1);
} /* end ProcessNetMsg_ProtoMsg_VerErr() */

static void ProcessNetMsg_ProtoMsg_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_SB_EID, "SBN protocol version match with peer ");

    uint8 ver = SBN_PROTO_VER;

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_PROTO_MSG, ProcessorID, SpacecraftID, sizeof(ver), &ver),
                      SBN_SUCCESS);

    EVENT_CNT(1);
} /* end ProcessNetMsg_ProtoMsg_Nominal() */

static SBN_Status_t RecvFilter_Err(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_ERROR;
} /* end RecvFilter_Err() */

static void ProcessNetMsg_AppMsg_FiltErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 1 - Single filter to test error condition
     * - Filters[0]: Filter with RecvFilter_Err (returns SBN_ERROR) - Triggers filter error during message processing
     * - ProcessorID/SpacecraftID: Set to match the message being processed to ensure peer lookup succeeds */
    UT_ResetState(0);
    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterRecv = RecvFilter_Err;

    PeerPtr->Filters[0] = &Filter;
    PeerPtr->FilterCnt  = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, SpacecraftID, 0, NULL), SBN_ERROR);
} /* end ProcessNetMsg_AppMsg_FiltErr() */

static SBN_Status_t RecvFilter_Out(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_IF_EMPTY;
} /* end RecvFilter_Out() */

static void ProcessNetMsg_AppMsg_FiltOut(void)
{
    START();

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 1 - Single filter to test message filtering
     * - Filters[0]: Filter with RecvFilter_Out (returns SBN_IF_EMPTY) - Filters out the message
     * - PeerPtr->ProcessorID/SpacecraftID: Set to match message parameters to ensure peer lookup succeeds
     * - ProcessorID/SpacecraftID: Set to match the message being processed */
    UT_ResetState(0);
    SBN_FilterInterface_t Filter;
    memset(&Filter, 0, sizeof(Filter));
    Filter.FilterRecv = RecvFilter_Out;

    PeerPtr->ProcessorID  = ProcessorID;
    PeerPtr->SpacecraftID = SpacecraftID;
    PeerPtr->Filters[0]   = &Filter;
    PeerPtr->FilterCnt    = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, SpacecraftID, 0, NULL), SBN_IF_EMPTY);
} /* end ProcessNetMsg_AppMsg_FiltOut() */

static SBN_Status_t RecvFilter_Nominal(void *Data, SBN_Filter_Ctx_t *CtxPtr)
{
    return SBN_SUCCESS;
} /* end RecvFilter_Nominal() */

static void ProcessNetMsg_AppMsg_PassMsgErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_SB_EID, "ERROR: could not process peer message: CFE_SB_PassMsg error (Status=");

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_TransmitMsg), 1, -1);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, SpacecraftID, 0, NULL), SBN_ERROR);

    EVENT_CNT(1);
} /* end ProcessNetMsg_AppMsg_PassMsgErr() */

static void ProcessNetMsg_AppMsg_Nominal(void)
{
    START();

    /* Configuration Setup Rationale:
     * Filter Setup:
     * - FilterCnt: 2 - Two filters to test filter chain processing
     * - Filters[0]: Filter_Empty (NULL FilterRecv function) - Should be skipped during processing
     * - Filters[1]: Filter_Nominal (returns SBN_SUCCESS) - Allows message to pass through
     * - ProcessorID/SpacecraftID: Set to match the message being processed to ensure peer lookup succeeds */
    UT_ResetState(0);
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

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_APP_MSG, ProcessorID, SpacecraftID, 0, NULL), SBN_SUCCESS);
} /* end ProcessNetMsg_AppMsg_Nominal() */

static void ProcessNetMsg_SubMsg_Nominal(void)
{
    START();

    UT_ResetState(0);
    uint8  Buf[SBN_PACKED_SUB_SZ];
    Pack_t Pack;
    Pack_Init(&Pack, &Buf, SBN_PACKED_SUB_SZ, 0);
    Pack_Data(&Pack, (void *)SBN_IDENT, SBN_IDENT_LEN);
    Pack_UInt16(&Pack, 1);
    Pack_MsgID(&Pack, MsgID);
    CFE_SB_Qos_t QoS = { 0 };
    Pack_Data(&Pack, (void *)&QoS, sizeof(QoS));

    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, ProcessorID);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, SpacecraftID);

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_SUB_MSG, ProcessorID, SpacecraftID, sizeof(Buf), &Buf),
                      SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->Subs[0].MsgID.Value, MsgID.Value);
} /* end ProcessNetMsg_SubMsg_Nominal() */

static void ProcessNetMsg_UnSubMsg_Nominal(void)
{
    START();

    UT_ResetState(0);
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

    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_SUB_MSG, ProcessorID, SpacecraftID, sizeof(Buf), &Buf),
                      SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->SubCnt, 1);
    UtAssert_INT32_EQ(PeerPtr->Subs[0].MsgID.Value, MsgID.Value);
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_UNSUB_MSG, ProcessorID, SpacecraftID, sizeof(Buf), &Buf),
                      SBN_SUCCESS);
    UtAssert_INT32_EQ(PeerPtr->SubCnt, 0);
} /* end ProcessNetMsg_UnSubMsg_Nominal() */

static void ProcessNetMsg_NoMsg_Nominal(void)
{
    START();

    UT_ResetState(0);
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_NO_MSG, ProcessorID, SpacecraftID, 0, NULL), SBN_SUCCESS);
} /* end ProcessNetMsg_NoMsg_Nominal() */

static void ProcessNetMsg_MsgErr(void)
{
    START();

    UT_ResetState(0);

    /* send a net message of an invalid type */
    UtAssert_INT32_EQ(SBN_ProcessNetMsg(NetPtr, SBN_NO_MSG + 100, ProcessorID, SpacecraftID, 0, NULL), SBN_ERROR);
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

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "ERROR: could not disconnect peer: peer 5678:1234 already connected");

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: 1 - Pre-set peer as already connected to trigger the error condition */
    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    UtAssert_INT32_EQ(PeerPtr->Connected, 1);
    EVENT_CNT(1);
} /* end Connected_AlreadyErr() */

static void Connected_PipeOptErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID,
                        "ERROR: could not disconnect peer:: could not set pipe options 'SBN_1234_5678_Pipe'");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SetPipeOpts), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    EVENT_CNT(1);
} /* end Connected_PipeOptErr() */

static void Connected_SendErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->Send: Send_Err - Set to return error when sending protocol message
     * - PeerPtr->Connected: 0 (default) - Peer starts disconnected to allow connection attempt */
    UT_ResetState(0);
    IfOpsPtr->Send = Send_Err;

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);

    IfOpsPtr->Send = Send_Nominal;
} /* end Connected_SendErr() */

static void Connected_CrPipeErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID,
                        "ERROR: could not disconnect peer:: could not create peer pipe 'SBN_1234_5678_Pipe'");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, -1);

    UtAssert_INT32_EQ(SBN_Connected(PeerPtr), SBN_ERROR);
    EVENT_CNT(1);
} /* end Connected_CrPipeErr() */

static void Connected_Nominal(void)
{
    START();

    UT_ResetState(0);
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

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "ERROR: could not disconnect peer: already not connected to peer ");

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: 0 (default) - Peer starts disconnected to trigger the error condition
     * - PeerPtr->ProcessorID: ProcessorID - Set for error message identification */
    SBN_PeerInterface_t *PeerPtr = &SBN_AppData.Nets[0].Peers[0];

    PeerPtr->ProcessorID = ProcessorID;

    UtAssert_INT32_EQ(SBN_Disconnected(PeerPtr), SBN_ERROR);
    EVENT_CNT(1);
} /* end Disconnected_Nominal() */

static void Disconnected_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "Disconnected from peer ");

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->ProcessorID: ProcessorID - Set for event message identification
     * - PeerPtr->Connected: 1 - Pre-set peer as connected to allow valid disconnection */
    SBN_PeerInterface_t *PeerPtr = &SBN_AppData.Nets[0].Peers[0];

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

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload network ");

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->UnloadNet: UnloadNet_Err - Set to return error when unloading network
     *
     * Peer Setup:
     * - PeerPtr->Connected: 1 - Pre-set peer as connected to ensure UnloadNet is called */
    IfOpsPtr->UnloadNet = UnloadNet_Err;

    PeerPtr->Connected = 1;

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    EVENT_CNT(1);

    IfOpsPtr->UnloadNet = UnloadNet_Nominal;
} /* end ReloadConfTbl_UnloadNetErr() */

static void ReloadConfTbl_ProtoUnloadErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload protocol module ID ");

    /* Configuration Setup Rationale:
     * Module Setup:
     * - SBN_AppData.ProtocolModules[0]: OS_ObjectIdFromInteger(1) - Set to valid module ID to trigger unload attempt
     * - OS_ModuleUnload: Set to fail (-1) - Simulates protocol module unload failure
     *
     * Peer Setup:
     * - PeerPtr->Connected: 1 - Pre-set peer as connected to ensure cleanup phase is reached */
    PeerPtr->Connected = 1;

    SBN_AppData.ProtocolModules[0] = OS_ObjectIdFromInteger(1);

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    EVENT_CNT(1);
} /* end ReloadConfTbl_ProtoUnloadErr() */

static void ReloadConfTbl_FiltUnloadErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload filter module ID ");

    /*
     * Configuration Setup Rationale:
     * Module Setup:
     * - SBN_AppData.FilterModules[0]: OS_ObjectIdFromInteger(1) - Set to valid module ID to trigger unload attempt
     * - OS_ModuleUnload: Set to fail (-1) - Simulates filter module unload failure */
    SBN_AppData.FilterModules[0] = OS_ObjectIdFromInteger(1);

    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);

    EVENT_CNT(1);
} /* end ReloadConfTbl_FiltUnloadErr() */

static void ReloadConfTbl_TblUpdErr(void)
{
    START();

    /*
     * Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: 1 - Pre-set peer as connected to ensure cleanup phase is reached
     * - CFE_TBL_Update: Set to fail (-1) - Simulates table update failure at the end of reload process */
    UT_ResetState(0);
    PeerPtr->Connected = 1;

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Update), 1, -1);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_ERROR);
} /* end ReloadConfTbl_TblUpdErr() */

static void ReloadConfTbl_Nominal(void)
{
    START();

    UT_ResetState(0);

    /*
     * Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one protocol module to create a minimal valid configuration
     * - FilterCnt: 0 - No filters needed for nominal reload testing
     * - PeerCnt: 1 - Need one peer configured as local processor to create a network
     *
     * Peer Configuration:
     * - SpacecraftID: 41 (same as local) - Makes this peer represent the local processor
     * - ProcessorID: 1 (same as local) - Confirms this is the local processor's network
     * - TaskFlags: 0 - No special task handling needed for reload test */
    memset(&TestConfTbl, 0, sizeof(TestConfTbl));
    TestConfTbl.ProtocolCnt = 1;
    TestConfTbl.FilterCnt   = 0;
    TestConfTbl.PeerCnt     = 1;

    /* Set up protocol module */
    strncpy(TestConfTbl.ProtocolModules[0].Name, "UDP", sizeof(TestConfTbl.ProtocolModules[0].Name));
    strncpy(TestConfTbl.ProtocolModules[0].LibSymbol, "SBN_UDP_Ops", sizeof(TestConfTbl.ProtocolModules[0].LibSymbol));
    strncpy(TestConfTbl.ProtocolModules[0].LibFileName,
            "/cf/sbn_udp.so",
            sizeof(TestConfTbl.ProtocolModules[0].LibFileName));
    TestConfTbl.ProtocolModules[0].BaseEID = 1234;

    /* Set up peer that will be this processor (creates network) */
    TestConfTbl.Peers[0].SpacecraftID = 41; /* Same as local */
    TestConfTbl.Peers[0].ProcessorID  = 1;  /* Same as local */
    TestConfTbl.Peers[0].NetNum       = 0;
    TestConfTbl.Peers[0].TaskFlags    = 0;
    strncpy(TestConfTbl.Peers[0].ProtocolName, "UDP", sizeof(TestConfTbl.Peers[0].ProtocolName));
    snprintf((char *)TestConfTbl.Peers[0].Address, sizeof(TestConfTbl.Peers[0].Address), "127.0.0.1:1234");

    /* Set up interface operations */
    IfOpsPtr->InitModule = ProtoInitModule_Nominal;
    IfOpsPtr->LoadNet    = LoadNet_Nominal;
    /* These functions are inside the structure, don't use them directly */
    /* IfOpsPtr->InitNet = InitNet_Nominal; */
    /* IfOpsPtr->InitPeer = InitPeer_Nominal; */

    /* Set up CFE_TBL_GetAddress to return our test table */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 2, CFE_TBL_INFO_UPDATED);
    UT_SetHandlerFunction(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_GetAddress_Hook, NULL);

    /* Set up OS_SymbolLookup to succeed */
    UT_SetHandlerFunction(UT_KEY(OS_SymbolLookup), OS_SymbolLookup_Hook, NULL);

    /* Set up OS_ModuleLoad to succeed */
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleLoad), 1, OS_SUCCESS);

    /* Set up processor/spacecraft IDs to match this "peer" entry */
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 41);

    /* Set up CFE_TBL_ReleaseAddress to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 2, CFE_SUCCESS);

    /* Set up mutex operations */
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemTake), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemGive), 1, OS_SUCCESS);
    SBN_AppData.ConfMutex = 1; /* Valid non-zero value */

    /* Set up for Cleanup */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_DeletePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 1, OS_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(OS_ModuleUnload), 2, OS_SUCCESS);

    /* Set up for Init - SetupSubPipe */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SUCCESS);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 2, CFE_SUCCESS);

    /* Set up CFE_ES_WaitForSystemState to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_WaitForSystemState), 1, CFE_SUCCESS);

    /* Set up CFE_TBL_Update to succeed */
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Update), 1, CFE_SUCCESS);

    /* Call the function under test */
    SBN_Status_t Result = SBN_ReloadConfTbl();

    /* Verify the result */
    UtAssert_INT32_EQ(Result, SBN_SUCCESS);
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

    UT_ResetState(0);

    uint8              Buf[SBN_MAX_PACKED_MSG_SZ] = { 0 }, Payload[1] = { 0 };
    SBN_MsgSz_t        MsgSz;
    SBN_MsgType_t      MsgType;
    CFE_ProcessorID_t  ProcID;
    CFE_SpacecraftID_t SpaceID;

    SBN_PackMsg(Buf, 0, SBN_APP_MSG, ProcessorID, SpacecraftID, NULL);
    UtAssert_True(SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, &SpaceID, Payload), "unpack of an empty pack");
    UtAssert_INT32_EQ(MsgSz, 0);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(ProcID, ProcessorID);
    UtAssert_INT32_EQ(SpaceID, SpacecraftID);
} /* end Unpack_Empty() */

static void Unpack_Err(void)
{
    START();

    UT_ResetState(0);

    uint8              Buf[SBN_MAX_PACKED_MSG_SZ] = { 0 }, Payload[1] = { 0 };
    SBN_MsgSz_t        MsgSz;
    SBN_MsgType_t      MsgType;
    CFE_ProcessorID_t  ProcID;
    CFE_SpacecraftID_t SpaceID;

    Pack_t Pack;
    Pack_Init(&Pack, Buf, SBN_MAX_PACKED_MSG_SZ + SBN_PACKED_HDR_SZ, 0);
    Pack_Int16(&Pack, -1); /* invalid msg size */
    Pack_UInt8(&Pack, SBN_APP_MSG);
    Pack_UInt32(&Pack, ProcessorID);
    Pack_UInt32(&Pack, SpacecraftID);

    UtAssert_True(!SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, &SpaceID, Payload), "unpack of invalid pack");
} /* end Unpack_Err() */

static void Unpack_Nominal(void)
{
    START();

    UT_ResetState(0);

    uint8              Buf[SBN_MAX_PACKED_MSG_SZ] = { 0 }, Payload[1] = { 0 };
    uint8              TestData = 123;
    SBN_MsgSz_t        MsgSz;
    SBN_MsgType_t      MsgType;
    CFE_ProcessorID_t  ProcID;
    CFE_SpacecraftID_t SpaceID;

    SBN_PackMsg(Buf, 1, SBN_APP_MSG, ProcessorID, SpacecraftID, &TestData);

    UtAssert_True(SBN_UnpackMsg(Buf, &MsgSz, &MsgType, &ProcID, &SpaceID, Payload), "unpack of a pack");

    UtAssert_INT32_EQ(MsgSz, 1);
    UtAssert_INT32_EQ(MsgType, SBN_APP_MSG);
    UtAssert_INT32_EQ(ProcID, ProcessorID);
    UtAssert_INT32_EQ(SpaceID, SpacecraftID);
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

    UT_ResetState(0);
    NetPtr->TaskFlags = SBN_TASK_RECV;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);
} /* end RecvNetMsgs_TaskRecv() */

static SBN_Status_t RecvFromNet_Empty(SBN_NetInterface_t *Net,
                                      SBN_MsgType_t      *MsgTypePtr,
                                      SBN_MsgSz_t        *MsgSzPtr,
                                      CFE_ProcessorID_t  *ProcessorIDPtr,
                                      CFE_SpacecraftID_t *SpacecraftIDPtr,
                                      void               *PayloadBuffer)
{
    *ProcessorIDPtr = 1235;

    return SBN_IF_EMPTY;
} /* end RecvFromNet_Empty() */

void RecvNetMsgs_NetEmpty(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: RecvFromNet_Empty - Set to return SBN_IF_EMPTY (no messages available) */
    UT_ResetState(0);
    IfOpsPtr->RecvFromNet = RecvFromNet_Empty;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetMsgs_NetEmpty() */

void RecvNetMsgs_PeerRecv(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: NULL - Disable network-level receive to force peer-level processing
     * - IfOpsPtr->RecvFromPeer: RecvFromPeer_Nominal - Enable peer-level receive processing */
    UT_ResetState(0);
    IfOpsPtr->RecvFromNet  = NULL;
    IfOpsPtr->RecvFromPeer = RecvFromPeer_Nominal;

    UtAssert_INT32_EQ(SBN_RecvNetMsgs(), SBN_SUCCESS);

    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
    IfOpsPtr->RecvFromPeer = NULL;
} /* end RecvNetMsgs_PeerRecv() */

void RecvNetMsgs_NoRecv(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: NULL - Disable network-level receive
     * - IfOpsPtr->RecvFromPeer: NULL - Disable peer-level receive */
    UT_ResetState(0);
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

    UT_ResetState(0);
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

static void RecvPeerTask_NetConfErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unable to connect task to peer struct");

    /* Configuration Setup Rationale:
     * Network Setup:
     * - NetPtr->Configured: false - Set network as unconfigured to trigger error condition */
    NetPtr->Configured = false;

    SBN_RecvPeerTask();

    EVENT_CNT(1);
} /* end RecvPeerTask_NetConfErr() */

static SBN_Status_t RecvFromPeer_EmptyOne(SBN_NetInterface_t  *Net,
                                          SBN_PeerInterface_t *Peer,
                                          SBN_MsgType_t       *MsgTypePtr,
                                          SBN_MsgSz_t         *MsgSzPtr,
                                          CFE_ProcessorID_t   *ProcessorIDPtr,
                                          CFE_SpacecraftID_t  *SpacecraftIDPtr,
                                          void                *PayloadBuffer)
{
    static int c = 0;

    if (c++ == 0)
        return SBN_IF_EMPTY;
    return SBN_ERROR;
} /* end RecvFromPeer_EmptyOne() */

static void RecvPeerTask_Empty(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: NULL - Disable network-level receive
     * - IfOpsPtr->RecvFromPeer: RecvFromPeer_EmptyOne - Returns SBN_IF_EMPTY first call, then SBN_ERROR to exit loop
     * - PeerPtr->RecvTaskID: Pre-created task ID - Simulates successful task creation */
    UT_ResetState(0);
    IfOpsPtr->RecvFromNet  = NULL;
    IfOpsPtr->RecvFromPeer = RecvFromPeer_EmptyOne;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvPeerTask();

    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
    IfOpsPtr->RecvFromPeer = NULL;
} /* end RecvPeerTask_Empty() */

static SBN_Status_t RecvFromPeer_One(SBN_NetInterface_t  *Net,
                                     SBN_PeerInterface_t *Peer,
                                     SBN_MsgType_t       *MsgTypePtr,
                                     SBN_MsgSz_t         *MsgSzPtr,
                                     CFE_ProcessorID_t   *ProcessorIDPtr,
                                     CFE_SpacecraftID_t  *SpacecraftIDPtr,
                                     void                *PayloadBuffer)
{
    static int c = 0;

    if (c++ == 0)
        return SBN_SUCCESS;
    return SBN_ERROR;
} /* end RecvFromPeer_One() */

static void RecvPeerTask_Nominal(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: NULL - Disable network-level receive
     * - IfOpsPtr->RecvFromPeer: RecvFromPeer_One - Returns SBN_SUCCESS first call, then SBN_ERROR to exit loop
     * - PeerPtr->RecvTaskID: Pre-created task ID - Simulates successful task creation */
    UT_ResetState(0);
    IfOpsPtr->RecvFromNet  = NULL;
    IfOpsPtr->RecvFromPeer = RecvFromPeer_One;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvPeerTask();

    IfOpsPtr->RecvFromNet  = RecvFromNet_Nominal;
    IfOpsPtr->RecvFromPeer = NULL;
} /* end RecvPeerTask_Nominal() */

static void Test_SBN_RecvPeerTask(void)
{
    RecvPeerTask_NetConfErr();
    RecvPeerTask_Empty();
    RecvPeerTask_Nominal();
} /* end Test_SBN_RecvPeerTask() */

static void RecvNetTask_NetConfErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEERTASK_EID,
                        "ERROR: could not start SBN Receive Net Task: unable to connect task to net struct");

    /* Configuration Setup Rationale:
     * Network Setup:
     * - NetPtr->Configured: false - Set network as unconfigured to trigger error condition */
    NetPtr->Configured = false;

    SBN_RecvNetTask();

    EVENT_CNT(1);
} /* end RecvNetTask_NetConfErr() */

static SBN_Status_t RecvFromNet_EmptyOne(SBN_NetInterface_t *Net,
                                         SBN_MsgType_t      *MsgTypePtr,
                                         SBN_MsgSz_t        *MsgSzPtr,
                                         CFE_ProcessorID_t  *ProcessorIDPtr,
                                         CFE_SpacecraftID_t *SpacecraftIDPtr,
                                         void               *PayloadBuffer)
{
    static int c = 0;

    if (c++ == 0)
        return SBN_IF_EMPTY;
    return SBN_ERROR;
} /* end RecvFromNet_EmptyOne() */

static void RecvNetTask_Empty(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: RecvFromNet_EmptyOne - Returns SBN_IF_EMPTY first call, then SBN_ERROR to exit loop
     * - NetPtr->RecvTaskID: Pre-created task ID - Simulates successful task creation */
    UT_ResetState(0);
    IfOpsPtr->RecvFromNet = RecvFromNet_EmptyOne;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&NetPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvNetTask();

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetTask_Empty() */

static SBN_Status_t RecvFromNet_BadPeer(SBN_NetInterface_t *Net,
                                        SBN_MsgType_t      *MsgTypePtr,
                                        SBN_MsgSz_t        *MsgSzPtr,
                                        CFE_ProcessorID_t  *ProcessorIDPtr,
                                        CFE_SpacecraftID_t *SpacecraftIDPtr,
                                        void               *PayloadBuffer)
{
    *ProcessorIDPtr = 0;

    return SBN_SUCCESS;
} /* end RecvFromNet_BadPeer() */

static void RecvNetTask_PeerErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEERTASK_EID, "unknown peer (ProcessorID=0)");

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: RecvFromNet_BadPeer - Returns ProcessorID=0 to simulate unknown peer
     * - NetPtr->RecvTaskID: Pre-created task ID - Simulates successful task creation */
    IfOpsPtr->RecvFromNet = RecvFromNet_BadPeer;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&NetPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvNetTask();

    EVENT_CNT(1);

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetTask_PeerErr() */

static SBN_Status_t RecvFromNet_One(SBN_NetInterface_t *Net,
                                    SBN_MsgType_t      *MsgTypePtr,
                                    SBN_MsgSz_t        *MsgSzPtr,
                                    CFE_ProcessorID_t  *ProcessorIDPtr,
                                    CFE_SpacecraftID_t *SpacecraftIDPtr,
                                    void               *PayloadBuffer)
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

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->RecvFromNet: RecvFromNet_One - Returns SBN_SUCCESS first call with bogus message type,
     *                                           then SBN_ERROR to exit loop
     * - NetPtr->RecvTaskID: Pre-created task ID - Simulates successful task creation */
    UT_ResetState(0);
    IfOpsPtr->RecvFromNet = RecvFromNet_One;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&NetPtr->RecvTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    SBN_RecvNetTask();

    IfOpsPtr->RecvFromNet = RecvFromNet_Nominal;
} /* end RecvNetTask_Nominal() */

static void Test_SBN_RecvNetTask(void)
{
    RecvNetTask_NetConfErr();
    RecvNetTask_Empty();
    RecvNetTask_PeerErr();
    RecvNetTask_Nominal();
} /* end Test_SBN_RecvNetTask() */

static void SendTask_ConnTaskErr(void)
{
    START();

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: true - Peer must be connected for send task processing to occur
     * - PeerPtr->SendTaskID: Pre-created task ID - Simulates successful task creation
     *
     * Message Setup:
     * - CFE_SB_ReceiveBuffer: Default CFE_SB_NO_MESSAGE, then error (-1) on 2nd call - Triggers task exit */
    PeerPtr->Connected = true;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, -1);

    SBN_SendTask();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end SendTask_ConnTaskErr() */

static int32 TaskDelayConn(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    static int c = 0;

    if (c++ > 0)
    {
        SBN_AppData.Nets[0].Peers[0].Connected = true;
    } /* end if */

    return CFE_SUCCESS;
}

static void SendTask_PeerNotConn(void)
{
    START();

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: false - Peer starts disconnected to test wait-for-connection behavior
     * - PeerPtr->SendTaskID: Pre-created task ID - Simulates successful task creation
     *
     * Message Setup:
     * - CFE_SB_ReceiveBuffer: Default CFE_SB_NO_MESSAGE, then error (-1) on 2nd call - Triggers task exit
     * - OS_TaskDelay: TaskDelayConn hook - Sets peer connected after first delay to simulate connection */
    PeerPtr->Connected = false;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

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

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: true - Peer must be connected for send task processing to occur
     * - PeerPtr->SendTaskID: Pre-created task ID - Simulates successful task creation
     *
     * Filter Setup:
     * - FilterCnt: 1 - Single filter to test error condition
     * - Filters[0]: Filter_Err with SendFilter_Err (returns SBN_ERROR) - Triggers filter error handling
     *
     * Message Setup:
     * - CFE_SB_ReceiveBuffer: Error (-1) on 2nd call - Triggers task exit after filter processing */
    UT_ResetState(0);
    PeerPtr->Connected = true;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

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

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: true - Peer must be connected for send task processing to occur
     * - PeerPtr->SendTaskID: Pre-created task ID - Simulates successful task creation
     *
     * Filter Setup:
     * - FilterCnt: 3 - Three filters to test different filter behaviors in chain
     * - Filters[0]: Filter_Empty (NULL FilterSend function) - Should be skipped during processing
     * - Filters[1]: Filter_Nominal (returns SBN_SUCCESS) - Allows message to pass through
     * - Filters[2]: Filter_Out (returns SBN_IF_EMPTY) - Filters out the message
     *
     * Message Setup:
     * - CFE_SB_ReceiveBuffer: Error (-1) on 2nd call - Triggers task exit after filter processing */
    UT_ResetState(0);
    PeerPtr->Connected = true;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

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

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: true - Peer must be connected for send task processing to occur
     * - PeerPtr->FilterCnt: 0 - No filters for simplicity to focus on SendNetMsg error
     * - PeerPtr->SendTaskID: Pre-created task ID - Simulates successful task creation
     *
     * Message Setup:
     * - CFE_SB_ReceiveBuffer: Returns test message buffer on first call, then CFE_SB_NO_MESSAGE default
     * - CFE_MSG_GetSize: Returns 100 bytes - Simulates valid message size
     * - CFE_ES_GetTaskID: Returns matching task ID - Ensures task validation succeeds
     * - CFE_PSP_GetProcessorId/SpacecraftId: Set to valid IDs for message processing
     *
     * Error Injection:
     * - SBN_SendNetMsg: Set to return SBN_ERROR - Triggers error handling and task exit */
    CFE_SB_Buffer_t  TestBuffer;
    CFE_SB_Buffer_t *TestBufferPtr = &TestBuffer;
    CFE_MSG_Size_t   TestMsgSize   = 100;
    CFE_ES_TaskId_t  TestTaskID;

    /*
     * Test Case: SBN_SendTask with SBN_SendNetMsg error
     * This should cause the task to exit the main loop
     */

    /* Set up the peer to be connected */
    PeerPtr->Connected = true;
    PeerPtr->FilterCnt = 0; /* No filters for simplicity */

    /* Create a valid task ID and assign it to the peer */
    CFE_ES_CreateChildTask(&TestTaskID, "test_task", test_osal_task_entry, NULL, 0, 0, 0);
    PeerPtr->SendTaskID = TestTaskID;

    /* Set up CFE_ES_GetTaskID to return the same task ID */
    UT_SetDataBuffer(UT_KEY(CFE_ES_GetTaskID), &TestTaskID, sizeof(TestTaskID), false);

    /* Set up message reception - first call succeeds, providing a message */
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &TestBufferPtr, sizeof(TestBufferPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SUCCESS);

    /* Set up message size */
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &TestMsgSize, sizeof(TestMsgSize), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_MSG_GetSize), 1, CFE_SUCCESS);

    /* Set up processor/spacecraft IDs */
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetProcessorId), 1, 1);
    UT_SetDeferredRetcode(UT_KEY(CFE_PSP_GetSpacecraftId), 1, 42);

    /* Set up SBN_SendNetMsg to return error - this should cause task to exit */
    UT_SetDeferredRetcode(UT_KEY(SBN_SendNetMsg), 1, SBN_ERROR);

    /* Run the task - it should exit due to SBN_SendNetMsg error */
    SBN_SendTask();

    /* Verify that the task ID was cleared */
    UtAssert_True(CFE_RESOURCEID_TEST_EQUAL(PeerPtr->SendTaskID, CFE_ES_TASKID_UNDEFINED),
                  "SendTaskID should be cleared after error");

    /* Verify that CFE_SB_ReceiveBuffer was called */
    UtAssert_STUB_COUNT(CFE_SB_ReceiveBuffer, 2);

    /* Verify that SBN_SendNetMsg was called (via OS_GetLocalTime check) */
    UtAssert_STUB_COUNT(OS_GetLocalTime, 1);
} /* end SendTask_SendNetMsgErr() */

static void SendTask_Nominal(void)
{
    START();

    UT_ResetState(0);
    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SB_NO_MESSAGE);

    /* Configuration Setup Rationale:
     * Peer Setup:
     * - PeerPtr->Connected: true - Peer must be connected for send task processing to occur
     * - PeerPtr->SendTaskID: Pre-created task ID - Simulates successful task creation
     * - PeerPtr->FilterCnt: 0 (default) - No filters configured for simplicity
     *
     * Message Setup:
     * - CFE_SB_ReceiveBuffer: Default CFE_SB_NO_MESSAGE, then error (-1) on 2nd call - Triggers clean task exit */
    PeerPtr->Connected = true;

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 2, -1);

    SBN_SendTask();

    UT_SetDefaultReturnValue(UT_KEY(CFE_SB_ReceiveBuffer), CFE_SUCCESS);
} /* end SendTask_Nominal() */

static void Test_SBN_SendTask(void)
{
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

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "unable to take send mutex");

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemTake), 1, -1);

    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), SBN_ERROR);

    EVENT_CNT(1);
} /* end SendNetMsg_MutexTakeErr() */

void SendNetMsg_MutexGiveErr(void)
{
    START();

    UT_ResetState(0);
    UT_CheckEvent_Setup(SBN_PEER_EID, "unable to give send mutex");

    /* This calls a stub that fills in the task ID. Other parameters are ignored. */
    CFE_ES_CreateChildTask(&PeerPtr->SendTaskID, "coverage", test_osal_task_entry, NULL, 0, 0, 0);
    UT_SetDeferredRetcode(UT_KEY(OS_MutSemGive), 1, -1);

    UtAssert_INT32_EQ(SBN_SendNetMsg(0, 0, NULL, PeerPtr), SBN_ERROR);

    EVENT_CNT(1);
} /* end SendNetMsg_MutexTakeErr() */

void SendNetMsg_SendErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * Interface Setup:
     * - IfOpsPtr->Send: Set to Send_Err first, then Send_Nominal - Tests both error and success paths */
    UT_ResetState(0);
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

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

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
