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
#include "cfe_psp_endian.h"
#include "cfe_sb_eventids.h"
#include "cfe_evs_msg.h"
#include "sbn_pack.h"
#include "sbn_error.h"
#include "sbn_dispatch.h"
#include "sbn_module.h"
#include "sbn_worker.h"

// jphfix static const CFE_SB_MsgId_t SBN_UT_MsgID = CFE_SB_MSGID_WRAP_VALUE(0x1818);
static SBN_ConfTbl_t SBN_UT_TestConfTbl;

static const SBN_ConfTbl_t SBN_UT_VALID_CONF =
{
    .FilterCnt = 2,
    .FilterModules =
    {
        { .LibSymbol = "UTF1s", .Name = "UTF1n" },
        { .LibSymbol = "UTF2s", .Name = "UTF2n" },
    },
    .ProtocolCnt = 2,
    .ProtocolModules =
    {
        { .LibSymbol = "UTP1s", .Name = "UTP1n" },
        { .LibSymbol = "UTP2s", .Name = "UTP2n" },
    },
    .PeerCnt = 5,
    .Peers =
    {
        {
            .NetNum = 0,
            .ProtocolName = "UTP1n",
            .ProcessorID = SBN_UT_LOCAL_PROCESSOR_ID,
            .SpacecraftID = SBN_UT_SPACECRAFT_ID,
            .Filters = { "UTF1n", "UTF2n" }
        },
        {
            .NetNum = 0,
            .ProtocolName = "UTP1n",
            .ProcessorID = SBN_UT_PEER_PROCESSOR_ID,
            .SpacecraftID = SBN_UT_SPACECRAFT_ID,
            .Filters = { "UTF1n" }
        },
        {
            .NetNum = 0,
            .ProtocolName = "UTP1n",
            .ProcessorID = SBN_UT_LOCAL_PROCESSOR_ID,
            .SpacecraftID = SBN_UT_SPACECRAFT_ID + 1,
            .Filters = { "UTF1n" }
        },
        {
            .NetNum = 1,
            .ProcessorID = SBN_UT_LOCAL_PROCESSOR_ID,
            .SpacecraftID = SBN_UT_SPACECRAFT_ID,
            .ProtocolName = "UTP2n",
            .Filters = { "UTF2n" }
        },
        {
            .NetNum = 2,
            .ProcessorID = SBN_UT_PEER_PROCESSOR_ID + 2,
            .SpacecraftID = SBN_UT_SPACECRAFT_ID,
            .ProtocolName = "UTP1n",
            .Filters = { "UTF1n" }
        },
    }
};

static SBN_Status_t SBN_UT_InitNet_Err(SBN_NetInterface_t *Host)
{
    return SBN_ERROR;
}
static SBN_Status_t SBN_UT_InitPeer_Err(SBN_PeerInterface_t *Peer)
{
    return SBN_ERROR;
}

/********************************** tests ************************************/
void Test_SBN_AppMain_EVSRegisterErr(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(CFE_EVS_Register), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(CFE_ES_GetAppID, 0);
} /* end AppMain_EVSRegisterErr() */

void Test_SBN_AppMain_AppIdErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: unable to get AppID");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetAppID), 1, -1);

    SBN_AppMain();

    UtAssert_STUB_COUNT(OS_TaskGetId, 0);
    EVENT_CNT(1);
} /* end AppMain_AppIdErr() */

void Test_SBN_AppMain_TaskInfoErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: SBN failed to get task info (");
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_GetTaskInfo), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_TaskInfoErr() */

/********************************** load conf tbl tests  ************************************/

void Test_SBN_LoadConfTbl_RegisterErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to register conf tbl handle");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConfTbl_RegisterErr() */

void Test_SBN_LoadConfTbl_LoadErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to load conf tbl /cf/sbn_conf_tbl.tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Load), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConfTbl_LoadErr() */

void Test_SBN_LoadConfTbl_NotifyErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_NotifyByMessage), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConfTbl_NotifyErr() */

void Test_SBN_LoadConf_GetAddrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to get conf table address");

    /* make sure it does not return INFO_UPDATED */
    UT_ResetState(UT_KEY(CFE_TBL_GetAddress));
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_BAD_TABLE_HANDLE);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_GetAddrErr() */

#ifdef jphfix
static SBN_Status_t ProtoInitErr_InitModule(int ProtocolVersion, SBN_EventID_t BaseEID, SBN_ProtocolOutlet_t *Outlet)
{
    return SBN_ERROR;
} /* end ProtoInitErr_InitModule */
#endif

static void CFE_SB_CreatePipe_Hook(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    CFE_SB_PipeId_t *PipeIdPtr = UT_Hook_GetArgValueByName(Context, "PipeIdPtr", CFE_SB_PipeId_t *);
    int32            Status;

    UT_Stub_GetInt32StatusCode(Context, &Status);

    if (PipeIdPtr != NULL && Status >= 0)
    {
        *PipeIdPtr = CFE_SB_PIPEID_C(CFE_ResourceId_FromInteger(UT_GetStubCount(FuncKey)));
    }
}

#ifdef jphfix
void Test_SBN_LoadConf_ProtoInitErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one protocol module to test InitModule failure
     * - FilterCnt: 0 - Skip filter processing, focus only on protocol initialization
     * - PeerCnt: 0 - No peers needed for this module initialization test
     * Expected Error: "error in protocol init" when protocol InitModule returns error */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");

    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "error in protocol init");

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);

    IfOpsPtr->InitModule = ProtoInitErr_InitModule;

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    SBN_AppMain();

    IfOpsPtr->InitModule = ProtoInitModule_Nominal;

    EVENT_CNT(1);
} /* end LoadConf_ProtoInitErr() */

static SBN_Status_t FilterInitErr_InitModule(int FilterVersion, SBN_EventID_t BaseEID)
{
    return SBN_ERROR;
} /* end FilterInitErr_InitModule */

void Test_SBN_LoadConf_FilterInitErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - Skip protocol processing, focus only on filter initialization
     * - FilterCnt: 1 - Need one filter module to test InitModule failure
     * - PeerCnt: 0 - No peers needed for this module initialization test
     * Expected Error: "error in filter init" when filter InitModule returns error */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));
    UT_SBN_AddFilterConfEntry(&SBN_UT_TestConfTbl, 0, "Endian");

    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "error in filter init");

    UT_SBN_SetupModuleBinding("Endian", FilterInterfacePtr);

    FilterInterfacePtr->InitModule = FilterInitErr_InitModule;

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    SBN_AppMain();

    FilterInterfacePtr->InitModule = FilterInitModule_Nominal;

    EVENT_CNT(1);
} /* end LoadConf_FilterInitErr() */

void Test_SBN_LoadConf_ProtoNameErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need one valid protocol module ("UDP")
     * - FilterCnt: 0 - Skip filter processing, focus on peer protocol validation
     * - PeerCnt: 1 - Need one peer that references an invalid protocol name
     * Expected Error: "invalid module name XDP" when peer references non-existent protocol */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 0, "XDP", NULL);

    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "protocol XDP is not available");

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);

    char o = NominalTblPtr->Peers[0].ProtocolName[0];

    NominalTblPtr->Peers[0].ProtocolName[0] = 'X'; /* temporary make it "XDP" */

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    SBN_AppMain();

    NominalTblPtr->Peers[0].ProtocolName[0] = o;

    EVENT_CNT(1);
} /* end LoadConf_ProtoNameErr() */

void Test_SBN_LoadConf_FiltNameErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need at least one valid protocol module for peer reference
     * - FilterCnt: 1 - Need one valid filter module ("CCSDS_Endian")
     * - PeerCnt: 1 - Need one peer that references an invalid filter name
     * Expected Error: "Invalid filter name" when peer references non-existent filter */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddFilterConfEntry(&SBN_UT_TestConfTbl, 0, "Endian");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 0, "UDP", "INVALID");

    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "cannot map filter INVALID");

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);
    UT_SBN_SetupModuleBinding("Endian", FilterInterfacePtr);

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end LoadConf_FiltNameErr() */
#endif

void Test_SBN_LoadConf_ReleaseAddrErr(void)
{
    /* Configuration Setup Rationale:
     * - ProtocolCnt: 2 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on table release error
     * - PeerCnt: 1 - Need at least one peer to get past peer processing to table release
     * Expected Error: "unable to release address of conf tbl" when CFE_TBL_ReleaseAddress fails */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 0, "UDP", NULL);

    START();

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to release address of conf tbl");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_ReleaseAddress), 1, -1);

    SBN_ReloadConfTbl();

    EVENT_CNT(1);
} /* end LoadConf_ReleaseAddrErr() */

void Test_SBN_AppMain_DeletePipeErr(void)
{
    START();

    /* Test Case for:
     * void SBN_AppMain(void);
     */

    UT_SetHandlerFunction(UT_KEY(CFE_SB_CreatePipe), CFE_SB_CreatePipe_Hook, NULL);
    UtAssert_VOIDCALL(SBN_AppMain());

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_DeletePipe), 1, -1);
    UtAssert_VOIDCALL(SBN_AppMain());
}

void Test_SBN_GetPeerRecord(void)
{
    START();

    /* Test Case for:
     * SBN_PeerRecord_t *SBN_GetPeerRecord(SBN_NetRecord_t *NetRec, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t
     * SpacecraftID);
     */
    SBN_NetRecord_t   *NetRec       = &SBN_AppData.NetRecords[0];
    SBN_ProcessorID_t  ProcessorID  = SBN_UT_PEER_PROCESSOR_ID;
    SBN_SpacecraftID_t SpacecraftID = SBN_UT_SPACECRAFT_ID;

    UtAssert_NOT_NULL(SBN_GetPeerRecord(NetRec, ProcessorID, SpacecraftID));
    UtAssert_NULL(SBN_GetPeerRecord(NetRec, ProcessorID + 1, SpacecraftID));
    UtAssert_NULL(SBN_GetPeerRecord(NetRec, ProcessorID, SpacecraftID + 1));

    UT_ResetState(UT_KEY(SBN_LocatePeerRecordInNet));
    UtAssert_NULL(SBN_GetPeerRecord(NetRec, ProcessorID, SpacecraftID));
}

void SBN_UT_RegisterNet_Handler(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_NetId_t     *NetIdOut = UT_Hook_GetArgValueByName(Context, "NetIdOut", SBN_NetId_t *);
    SBN_NetRecord_t *NetRec;
    uint32           IdValue;

    IdValue  = SBN_PLATFORM_MAX_NETS << 10;
    IdValue += UT_GetStubCount(FuncKey) - 1; /* so it starts at 0 */

    NetRec = &SBN_AppData.NetRecords[IdValue % SBN_PLATFORM_MAX_NETS];

    NetRec->NetId = SBN_NETID_C(CFE_ResourceId_FromInteger(IdValue));

    if (NetIdOut != NULL)
    {
        *NetIdOut = NetRec->NetId;
    }
}

void SBN_UT_RegisterPeer_Handler(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_PeerId_t     *PeerIdOut = UT_Hook_GetArgValueByName(Context, "PeerIdOut", SBN_PeerId_t *);
    SBN_PeerRecord_t *PeerRec;
    uint32            IdValue;

    IdValue  = SBN_MISSION_MAX_PEER_CNT << 10;
    IdValue += UT_GetStubCount(FuncKey) - 1;

    PeerRec = &SBN_AppData.PeerRecords[IdValue % SBN_MISSION_MAX_PEER_CNT];

    PeerRec->PeerId = SBN_PEERID_C(CFE_ResourceId_FromInteger(IdValue));

    if (PeerIdOut != NULL)
    {
        *PeerIdOut = PeerRec->PeerId;
    }
}

void SBN_UT_LocateNetRecordByID_Handler(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_NetId_t      NetId = UT_Hook_GetArgValueByName(Context, "NetId", SBN_NetId_t);
    SBN_NetRecord_t *NetRec;
    uint32           IdValue;
    int32            Status;

    UT_Stub_GetInt32StatusCode(Context, &Status);
    if (Status >= 0)
    {
        IdValue = CFE_RESOURCEID_TO_ULONG(NetId);

        NetRec = &SBN_AppData.NetRecords[IdValue % SBN_PLATFORM_MAX_NETS];

        UT_Stub_SetReturnValue(FuncKey, NetRec);
    }
}

void SBN_UT_LocatePeerRecordByID_Handler(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    SBN_PeerId_t      PeerId = UT_Hook_GetArgValueByName(Context, "PeerId", SBN_PeerId_t);
    SBN_PeerRecord_t *PeerRec;
    uint32            IdValue;
    int32             Status;

    UT_Stub_GetInt32StatusCode(Context, &Status);
    if (Status >= 0)
    {
        IdValue = CFE_RESOURCEID_TO_ULONG(PeerId);

        PeerRec = &SBN_AppData.PeerRecords[IdValue % SBN_MISSION_MAX_PEER_CNT];

        UT_Stub_SetReturnValue(FuncKey, PeerRec);
    }
}

void Test_SBN_ReloadConfTbl(void)
{
    /* For these do not start with a config */

    /* Test Case for:
     * SBN_Status_t SBN_ReloadConfTbl(void);
     */
    void *TblDataPtr;

    memset(&SBN_AppData, 0, sizeof(SBN_AppData));

    SBN_UT_TestConfTbl = SBN_UT_VALID_CONF;
    TblDataPtr         = &SBN_UT_TestConfTbl;

    /* All these calls have to do something akin to the correct behavior */
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblDataPtr, sizeof(TblDataPtr), false);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterNet), SBN_UT_RegisterNet_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterPeer), SBN_UT_RegisterPeer_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocateNetRecordByID), SBN_UT_LocateNetRecordByID_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocatePeerRecordByID), SBN_UT_LocatePeerRecordByID_Handler, NULL);

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_SUCCESS);

    /* Cover some off nominal/incorrect table values
     * (validation does most of this checking but this should not do anything bad) */
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblDataPtr, sizeof(TblDataPtr), false);
    SBN_UT_TestConfTbl.FilterCnt = SBN_MISSION_MAX_MOD_CNT + 1;
    UT_SetDeferredRetcode(UT_KEY(SBN_Module_GetFilterIf), 3, SBN_ERROR);
    SBN_UT_TestConfTbl.ProtocolCnt = SBN_MISSION_MAX_MOD_CNT + 1;
    UT_SetDeferredRetcode(UT_KEY(SBN_Module_GetProtocolIf), 3, SBN_ERROR);
    SBN_UT_TestConfTbl.PeerCnt = SBN_MISSION_MAX_PEER_CNT + 1;

    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_SUCCESS);

    /* A case where the filter fails to map */
    SBN_UT_TestConfTbl = SBN_UT_VALID_CONF;
    UT_CheckEvent_Setup(SBN_TBL_EID, "cannot map filter");

    SBN_UT_TestConfTbl.Peers[0].Filters[0][0] = 'X'; /* make it not match */
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblDataPtr, sizeof(TblDataPtr), false);
    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_SUCCESS);
    EVENT_CNT(1);
    SBN_UT_TestConfTbl.Peers[0].Filters[0][0] = 0;

    /* cause peers to fail registratiom */
    UT_CheckEvent_Setup(SBN_TBL_EID, "no peer record found");
    UT_ResetState(UT_KEY(SBN_RegisterPeer));
    UT_ResetState(UT_KEY(SBN_LocatePeerRecordByID));
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblDataPtr, sizeof(TblDataPtr), false);
    UtAssert_INT32_EQ(SBN_ReloadConfTbl(), SBN_SUCCESS);
    EVENT_CNT(3);
}

/* Hook function to modify SBN after SBN_LoadConf completes but before SBN_InitInterfaces */
static int32 SetupNetConfErr_Hook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    SBN_AppData.NetRecords->Intf.Configured = false;
    return StubRetcode;
}

void Test_SBN_ReloadConfTbl_NetConfErr1(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on network count increase
     * - PeerCnt: 1 - Need one peer to trigger network creation and count increase
     * Expected Info Event: "increasing net count to" when NetCnt is increased */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 2, "UDP", NULL);

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_INIT_EID, "network #0 not configured");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterNet), SBN_UT_RegisterNet_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterPeer), SBN_UT_RegisterPeer_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocateNetRecordByID), SBN_UT_LocateNetRecordByID_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocatePeerRecordByID), SBN_UT_LocatePeerRecordByID_Handler, NULL);

    /* THE KEY HOOK: Modify SBN structure after SBN_LoadConf completes */
    UT_SetHookFunction(UT_KEY(CFE_TBL_ReleaseAddress), SetupNetConfErr_Hook, NULL);

    /* Run the main function */
    SBN_ReloadConfTbl();

    /* Verify the expected event was generated */
    EVENT_CNT(1);
}

void Test_SBN_ReloadConfTbl_NetConfErr2(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on network count increase
     * - PeerCnt: 1 - Need one peer to trigger network creation and count increase
     * Expected Info Event: "increasing net count to" when NetCnt is increased */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 0, "UDP", NULL);
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 1, 0, "UDP", NULL);

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterNet), SBN_UT_RegisterNet_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterPeer), SBN_UT_RegisterPeer_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocateNetRecordByID), SBN_UT_LocateNetRecordByID_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocatePeerRecordByID), SBN_UT_LocatePeerRecordByID_Handler, NULL);
    UT_ResetState(UT_KEY(SBN_LocatePeerRecordInNet));

    /* Run the main function again */
    SBN_ReloadConfTbl();
}

void Test_SBN_ReloadConfTbl_NetConfErr3(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on network count increase
     * - PeerCnt: 1 - Need one peer to trigger network creation and count increase
     * Expected Info Event: "increasing net count to" when NetCnt is increased */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 2, "UDP", NULL);

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_INIT_EID, "network #0 not configured");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterNet), SBN_UT_RegisterNet_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterPeer), SBN_UT_RegisterPeer_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocateNetRecordByID), SBN_UT_LocateNetRecordByID_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocatePeerRecordByID), SBN_UT_LocatePeerRecordByID_Handler, NULL);
    UT_ResetState(UT_KEY(SBN_LocateNetRecordByExtIdx));

    /* Run the main function again */
    SBN_ReloadConfTbl();

    EVENT_CNT(1);
} /* end InitInt_NetConfErr() */

void Test_SBN_ReloadConfTbl_InitProtoErr1(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on network count increase
     * - PeerCnt: 1 - Need one peer to trigger network creation and count increase
     * Expected Info Event: "increasing net count to" when NetCnt is increased */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 2, "UDP", NULL);

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_TBL_EID, "Failed to map protocol");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDeferredRetcode(UT_KEY(SBN_Module_GetProtocolIf), 1, -1);
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    /* Run function */
    SBN_ReloadConfTbl();

    EVENT_CNT(1);
} /* end InitInt_NetConfErr() */

void Test_SBN_ReloadConfTbl_InitErr(void)
{
    START();

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not initialize SBN: unable to initialize interfaces");

    IfOpsPtr->InitNet  = SBN_UT_InitNet_Err;
    IfOpsPtr->InitPeer = SBN_UT_InitPeer_Err;

    /* Set up CFE_TBL_GetAddress to return our test table */
    SBN_UT_TestConfTbl = SBN_UT_VALID_CONF;

    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDeferredRetcode(UT_KEY(SBN_Module_GetProtocolIf), 1, -1);
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterNet), SBN_UT_RegisterNet_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_RegisterPeer), SBN_UT_RegisterPeer_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocateNetRecordByID), SBN_UT_LocateNetRecordByID_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocatePeerRecordByID), SBN_UT_LocatePeerRecordByID_Handler, NULL);

    /* Run function */
    SBN_ReloadConfTbl();

    IfOpsPtr->InitNet  = InitNet_Nominal;
    IfOpsPtr->InitPeer = InitPeer_Nominal;
}

void SBN_UT_IncrementErr_Handler(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    uint32 *Arg = UT_Hook_GetArgValueByName(Context, "Arg", uint32 *);

    if (Arg)
    {
        ++(*Arg);
    }
}

void Test_SBN_ReloadConfTbl_UnloadErr(void)
{
    START();

    UT_SetHandlerFunction(UT_KEY(SBN_ForEachConfiguredNet), SBN_UT_IncrementErr_Handler, NULL);
    UT_CheckEvent_Setup(SBN_PEER_EID, "unable to unload configuration");

    /* Run function */
    SBN_ReloadConfTbl();
    EVENT_CNT(1);
}

static SBN_Status_t ProtoInitModule_Err(int ProtoVersion, SBN_EventID_t BaseEID, SBN_ProtocolOutlet_t *Outlet)
{
    return SBN_ERROR;
} /* end ProtoInitModule_Nominal() */

void Test_SBN_ReloadConfTbl_InitProtoErr2(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 1 - Need at least one valid protocol for peer reference
     * - FilterCnt: 0 - Skip filter processing, focus on network count increase
     * - PeerCnt: 1 - Need one peer to trigger network creation and count increase
     * Expected Info Event: "increasing net count to" when NetCnt is increased */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddProtoConfEntry(&SBN_UT_TestConfTbl, 0, "UDP");
    UT_SBN_AddPeerConfEntry(&SBN_UT_TestConfTbl, 0, 2, "UDP", NULL);

    UT_SBN_SetupModuleBinding("UDP", IfOpsPtr);

    /* Set up event checking for the expected error */
    IfOpsPtr->InitModule = ProtoInitModule_Err;
    UT_CheckEvent_Setup(SBN_TBL_EID, "error in protocol init");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    /* Run function */
    SBN_ReloadConfTbl();

    EVENT_CNT(1);
    IfOpsPtr->InitModule = ProtoInitModule_Nominal;
} /* end InitInt_NetConfErr() */

void Test_SBN_ReloadConfTbl_InitFiltErr1(void)
{
    START();

    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddFilterConfEntry(&SBN_UT_TestConfTbl, 0, "F");

    /* Set up event checking for the expected error */
    UT_CheckEvent_Setup(SBN_TBL_EID, "Failed to map filter");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDeferredRetcode(UT_KEY(SBN_Module_GetFilterIf), 1, -1);
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    /* Run function */
    SBN_ReloadConfTbl();

    EVENT_CNT(1);
} /* end InitInt_NetConfErr() */

static SBN_Status_t FilterInitModule_Err(int FilterVersion, SBN_EventID_t BaseEID)
{
    return SBN_ERROR;
} /* end InitFilterModule() */

void Test_SBN_ReloadConfTbl_InitFiltErr2(void)
{
    START();

    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_SBN_AddFilterConfEntry(&SBN_UT_TestConfTbl, 0, "F");

    /* Set up event checking for the expected error */
    FilterInterfacePtr->InitModule = FilterInitModule_Err;
    UT_CheckEvent_Setup(SBN_TBL_EID, "error in filter init");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    /* Run function */
    SBN_ReloadConfTbl();

    EVENT_CNT(1);
    FilterInterfacePtr->InitModule = FilterInitModule_Nominal;
} /* end InitInt_NetConfErr() */

void Test_SBN_AppMain_SubPipeCrErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on subscription pipe creation failure
     * - FilterCnt: 0 - No filters needed for pipe creation error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach SBN_SetupSubPipe where error occurs
     * This allows SBN_LoadConf and SBN_InitInterfaces to succeed, then tests subscription pipe creation failure */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to create subscription pipe (Status=");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 2, CFE_SB_BAD_ARGUMENT); /* fail just after SBN_InitInterfaces() */

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeCrErr() */

void Test_SBN_AppMain_SubPipeAllSubErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on subscription to all-subscriptions failure
     * - FilterCnt: 0 - No filters needed for subscription error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach CFE_SB_SubscribeLocal failure
     * This allows pipe creation to succeed, then tests the first subscription (allsubs) failure */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to allsubs (Status=");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeAllSubErr() */

void Test_SBN_AppMain_SubPipeOneSubErr(void)
{
    START();

    /* Configuration Setup Rationale:
     * - ProtocolCnt: 0 - No protocols needed, test focuses on subscription to single-subscription failure
     * - FilterCnt: 0 - No filters needed for subscription error testing
     * - PeerCnt: 0 - No peers needed, minimal config to reach second CFE_SB_SubscribeLocal failure
     * This allows pipe creation and first subscription to succeed, then tests second subscription failure */
    memset(&SBN_UT_TestConfTbl, 0, sizeof(SBN_UT_TestConfTbl));

    UT_CheckEvent_Setup(SBN_INIT_EID, "failed to subscribe to sub (Status=");

    /* Set up CFE_TBL_GetAddress to return our test table */
    void *TblPtr = &SBN_UT_TestConfTbl;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_SubscribeLocal), 2, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_SubPipeOneSubErr() */

void Test_SBN_AppMain_CmdPipeCrErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: failed to create command pipe (");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_CmdPipeCrErr() */

void Test_SBN_AppMain_CmdPipeSubErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: failed to subscribe to command pipe (");
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SB_BAD_ARGUMENT);

    SBN_AppMain();

    EVENT_CNT(1);
} /* end AppMain_CmdPipeSubErr() */

void Test_SBN_AppMain_SetupWorkerErr(void)
{
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not start SBN: unable to set up worker tasks");
    UT_SetDeferredRetcode(UT_KEY(SBN_Worker_Setup), 1, -1);

    SBN_AppMain();

    EVENT_CNT(1);
}

static void SBN_UT_CheckInitEndian(bool BigEndian)
{
    START();

    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_IsBigEndian), BigEndian);
    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetProcessorId), 1);
    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetSpacecraftId), 41);
    UT_CheckEvent_Setup(SBN_INIT_EID,
                        BigEndian ? "initialized (ProcessorID=1 SpacecraftId=41 big-endian"
                                  : "initialized (ProcessorID=1 SpacecraftId=41 little-endian");

    SBN_AppMain();

    EVENT_CNT(1);
    UtAssert_STUB_COUNT(CFE_PSP_IsBigEndian, 1);
}

void Test_SBN_InitBigEndian(void)
{
    SBN_UT_CheckInitEndian(true);
}

void Test_SBN_InitLittleEndian(void)
{
    SBN_UT_CheckInitEndian(false);
}

void Test_SBN_AppMain_RunLoop(void)
{
    START();

    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, true);

    SBN_AppMain();
}

void Test_SBN_AppMain_CleanupErr(void)
{
#ifdef jphfix
    START();

    UT_CheckEvent_Setup(SBN_INIT_EID, "ERROR: could not clean up SBN");
    // EVSSendErr(SBN_INIT_EID, "failed to delete subscription pipe (Status=%d)", (int)Status);

    SBN_AppMain();

    EVENT_CNT(1);
#endif
}

void Test_SBN_FindActiveProtocol(void)
{
    /* Test Case For:
     * const SBN_ProtocolModInfo_t *SBN_FindActiveProtocol(const char *ProtocolName);
     */
    memset(&SBN_AppData.ProtocolModInfo, 0, sizeof(SBN_AppData.ProtocolModInfo));

    strcpy(SBN_AppData.ProtocolModInfo[0].ProtocolName, "UTP1n");
    strcpy(SBN_AppData.ProtocolModInfo[1].ProtocolName, "UTP2n");
    SBN_AppData.ProtocolModInfo[1].Intf = IfOpsPtr;

    UtAssert_NULL(SBN_FindActiveProtocol("INVALID"));
    UtAssert_NULL(SBN_FindActiveProtocol("UTP1n"));
    UtAssert_ADDRESS_EQ(SBN_FindActiveProtocol("UTP2n"), &SBN_AppData.ProtocolModInfo[1]);
}

void Test_SBN_FindActiveFilter(void)
{
    /* Test Case For:
     * const SBN_FilterModInfo_t *SBN_FindActiveFilter(const char *FilterName);
     */
    memset(&SBN_AppData.FilterModInfo, 0, sizeof(SBN_AppData.FilterModInfo));

    strcpy(SBN_AppData.FilterModInfo[0].FilterName, "UTF1n");
    strcpy(SBN_AppData.FilterModInfo[1].FilterName, "UTF2n");
    SBN_AppData.FilterModInfo[1].Intf = FilterInterfacePtr;

    UtAssert_NULL(SBN_FindActiveFilter("INVALID"));
    UtAssert_NULL(SBN_FindActiveFilter("UTF1n"));
    UtAssert_ADDRESS_EQ(SBN_FindActiveFilter("UTF2n"), &SBN_AppData.FilterModInfo[1]);
}

void Test_SBN_InitInterfaces(void)
{
    START();

    /* Test Case For:
     * SBN_Status_t SBN_InitInterfaces(void)
     */

    /* This should skip the init */
    IfOpsPtr->InitNet  = NULL;
    IfOpsPtr->InitPeer = NULL;
    UtAssert_INT32_EQ(SBN_InitInterfaces(), SBN_SUCCESS);

    /* This should report failure */
    IfOpsPtr->InitNet  = SBN_UT_InitNet_Err;
    IfOpsPtr->InitPeer = SBN_UT_InitPeer_Err;
    UtAssert_INT32_EQ(SBN_InitInterfaces(), SBN_ERROR);

    /* Restore normal ops */
    IfOpsPtr->InitNet  = InitNet_Nominal;
    IfOpsPtr->InitPeer = InitPeer_Nominal;
}

void Test_SBN_InitializePeerCounters(void)
{
    START();

    /* Test Case for:
     * void SBN_InitializePeerCounters(SBN_PeerRecord_t *PeerRec);
     */
    SBN_PeerRecord_t *PeerRec = &SBN_AppData.PeerRecords[0];

    UtAssert_VOIDCALL(SBN_InitializePeerCounters(PeerRec));
}

void Test_SBN_InitializeCounters(void)
{
    START();

    /* Test Case for:
     * void SBN_InitializeCounters(void);
     */
    UtAssert_VOIDCALL(SBN_InitializeCounters());
}

void Test_SBN_ValidateConfTbl(void)
{
    START();

    /* Test Case for:
     * SBN_Status_t SBN_ValidateConfTbl(void *TableData);
     */
    void *TableData = &SBN_UT_TestConfTbl;

    SBN_UT_TestConfTbl = SBN_UT_VALID_CONF;

    UtAssert_INT32_EQ(SBN_ValidateConfTbl(TableData), SBN_SUCCESS);

    /* This should complain about the extras */
    SBN_UT_TestConfTbl.FilterCnt = SBN_MISSION_MAX_MOD_CNT + 1;
    UtAssert_INT32_EQ(SBN_ValidateConfTbl(TableData), SBN_ERROR);
    SBN_UT_TestConfTbl.FilterCnt = 2;

    SBN_UT_TestConfTbl.ProtocolCnt = SBN_MISSION_MAX_MOD_CNT + 1;
    UtAssert_INT32_EQ(SBN_ValidateConfTbl(TableData), SBN_ERROR);
    SBN_UT_TestConfTbl.ProtocolCnt = 2;

    SBN_UT_TestConfTbl.PeerCnt = SBN_MISSION_MAX_PEER_CNT + 1;
    UtAssert_INT32_EQ(SBN_ValidateConfTbl(TableData), SBN_ERROR);
    SBN_UT_TestConfTbl.PeerCnt = 5;

    SBN_UT_TestConfTbl.Peers[0].ProtocolName[0] = 'X';
    UtAssert_INT32_EQ(SBN_ValidateConfTbl(TableData), SBN_ERROR);
    SBN_UT_TestConfTbl.Peers[0].ProtocolName[0] = 'U';

    SBN_UT_TestConfTbl.Peers[0].NetNum = 255;
    UtAssert_INT32_EQ(SBN_ValidateConfTbl(TableData), SBN_ERROR);
    SBN_UT_TestConfTbl.Peers[0].NetNum = 0;
}

void Test_SBN_GetPeer(void)
{
    START();

    /* Test Case for:
     * SBN_PeerInterface_t *SBN_GetPeer(SBN_NetInterface_t *Net, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t
     * SpacecraftID);
     */
    SBN_NetInterface_t *Net          = &SBN_AppData.NetRecords[0].Intf;
    SBN_ProcessorID_t   ProcessorID  = SBN_UT_PEER_PROCESSOR_ID;
    SBN_SpacecraftID_t  SpacecraftID = SBN_UT_SPACECRAFT_ID;

    UtAssert_NOT_NULL(SBN_GetPeer(Net, ProcessorID, SpacecraftID));
}

void Test_SBN_PackMsg(void)
{
    START();

    /* Test Case for:
     * void SBN_PackMsg(void *SBNBuf, SBN_MsgSz_t MsgSz, SBN_MsgType_t MsgType, SBN_ProcessorID_t ProcessorID,
     * SBN_SpacecraftID_t SpacecraftID, void *Msg);
     */
    void              *SBNBuf       = NULL;
    SBN_MsgSz_t        MsgSz        = 0;
    SBN_MsgType_t      MsgType      = SBN_NO_MSG;
    SBN_ProcessorID_t  ProcessorID  = SBN_UT_PEER_PROCESSOR_ID;
    SBN_SpacecraftID_t SpacecraftID = SBN_UT_SPACECRAFT_ID;
    void              *Msg          = NULL;
    uint8              MsgBuf[16];

    memset(MsgBuf, 0x55, sizeof(MsgBuf));

    UtAssert_VOIDCALL(SBN_PackMsg(SBNBuf, MsgSz, MsgType, ProcessorID, SpacecraftID, Msg));
    UtAssert_STUB_COUNT(SBN_Pack_Data, 0);

    Msg   = MsgBuf;
    MsgSz = 0;
    UtAssert_VOIDCALL(SBN_PackMsg(SBNBuf, MsgSz, MsgType, ProcessorID, SpacecraftID, Msg));
    UtAssert_STUB_COUNT(SBN_Pack_Data, 0);

    Msg   = NULL;
    MsgSz = sizeof(MsgBuf);
    UtAssert_VOIDCALL(SBN_PackMsg(SBNBuf, MsgSz, MsgType, ProcessorID, SpacecraftID, Msg));
    UtAssert_STUB_COUNT(SBN_Pack_Data, 0);

    /* Finally the valid config */
    Msg   = MsgBuf;
    MsgSz = sizeof(MsgBuf);
    UtAssert_VOIDCALL(SBN_PackMsg(SBNBuf, MsgSz, MsgType, ProcessorID, SpacecraftID, Msg));
    UtAssert_STUB_COUNT(SBN_Pack_Data, 1);
}

void Test_SBN_UnpackMsg(void)
{
    START();

    /* Test Case for:
     * bool SBN_UnpackMsg(void *SBNBuf, SBN_MsgSz_t *MsgSzPtr, SBN_MsgType_t *MsgTypePtr, SBN_ProcessorID_t
     * *ProcessorIDPtr, SBN_SpacecraftID_t *SpacecraftIDPtr, void *Msg);
     */
    void              *SBNBuf = NULL;
    uint32             MsgSzBuf;
    SBN_MsgSz_t        MsgSz;
    SBN_MsgType_t      MsgType;
    SBN_ProcessorID_t  ProcessorID;
    SBN_SpacecraftID_t SpacecraftID;
    void              *Msg = NULL;
    uint8              MsgBuf[16];

    memset(MsgBuf, 0xAA, sizeof(MsgBuf));

    /* Nominal */
    UtAssert_BOOL_TRUE(SBN_UnpackMsg(SBNBuf, &MsgSz, &MsgType, &ProcessorID, &SpacecraftID, Msg));
    UtAssert_STUB_COUNT(SBN_Unpack_Data, 0);

    /* msg too large */
    Msg      = MsgBuf;
    MsgSzBuf = CFE_MISSION_SB_MAX_SB_MSG_SIZE + 1;
    UT_SetDataBuffer(UT_KEY(SBN_Unpack_UInt32), &MsgSzBuf, sizeof(MsgSzBuf), false);
    UtAssert_BOOL_FALSE(SBN_UnpackMsg(SBNBuf, &MsgSz, &MsgType, &ProcessorID, &SpacecraftID, Msg));
    UtAssert_STUB_COUNT(SBN_Unpack_Data, 0);

    /* Finally the valid config */
    Msg      = MsgBuf;
    MsgSzBuf = sizeof(MsgBuf);
    UT_SetDataBuffer(UT_KEY(SBN_Unpack_UInt32), &MsgSzBuf, sizeof(MsgSzBuf), false);
    UtAssert_BOOL_TRUE(SBN_UnpackMsg(SBNBuf, &MsgSz, &MsgType, &ProcessorID, &SpacecraftID, Msg));
    UtAssert_STUB_COUNT(SBN_Unpack_Data, 1);
}

void Test_SBN_SendNetMsg(void)
{
    START();

    /* Test Case for:
     * SBN_Status_t SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer);
     */
    SBN_MsgType_t        MsgType = SBN_NO_MSG;
    SBN_MsgSz_t          MsgSz   = 0;
    void                *Msg     = NULL;
    SBN_PeerInterface_t *Peer    = &SBN_AppData.PeerRecords[0].Intf;

    UtAssert_INT32_EQ(SBN_SendNetMsg(MsgType, MsgSz, Msg, Peer), SBN_SUCCESS);
}

void Test_SBN_WaitForWakeup(void)
{
    START();

    /* Test Case for:
     * SBN_Status_t SBN_WaitForWakeup(int32 iTimeOut)
     */
    CFE_SB_Buffer_t  SbBuf;
    CFE_SB_Buffer_t *BufPtr;

    memset(&SbBuf, 0, sizeof(SbBuf));
    BufPtr = &SbBuf;

    /* Nominal */
    UT_SetDataBuffer(UT_KEY(CFE_SB_ReceiveBuffer), &BufPtr, sizeof(BufPtr), false);
    UtAssert_INT32_EQ(SBN_WaitForWakeup(1), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_HandleCommand, 1);

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_NO_MESSAGE);
    UtAssert_INT32_EQ(SBN_WaitForWakeup(1), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_HandleCommand, 1);

    /* No activity from Subs_PollCycle */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_TIME_OUT);
    UT_SetDeferredRetcode(UT_KEY(SBN_Subs_PollCycle), 1, SBN_IF_EMPTY);
    UtAssert_INT32_EQ(SBN_WaitForWakeup(1), SBN_SUCCESS);
    UtAssert_STUB_COUNT(SBN_HandleCommand, 1);

    /* failure in Subs_PollCycle (this has an event) */
    UT_CheckEvent_Setup(SBN_PEER_EID, "SBN_Subs_PollCycle failed");
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_NO_MESSAGE);
    UT_SetDeferredRetcode(UT_KEY(SBN_Subs_PollCycle), 1, SBN_ERROR);
    UtAssert_INT32_EQ(SBN_WaitForWakeup(1), SBN_SUCCESS);
    EVENT_CNT(1);

    /* This is the only thing that returns non success */
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_ReceiveBuffer), 1, CFE_SB_BAD_ARGUMENT);
    UtAssert_INT32_EQ(SBN_WaitForWakeup(1), SBN_ERROR);
}

SBN_Status_t UnloadNet_Err(SBN_NetInterface_t *Net)
{
    return SBN_ERROR;
} /* end UnloadNet_Nominal() */

void Test_SBN_Cleanup_NetErr(void)
{
    START();

    /* Test Case for:
     * SBN_Status_t SBN_Cleanup(void);
     */

    UT_CheckEvent_Setup(SBN_TBL_EID, "unable to unload network");

    IfOpsPtr->UnloadNet = UnloadNet_Err;
    UtAssert_INT32_EQ(SBN_Cleanup(), SBN_SUCCESS);
    EVENT_CNT(1);

    IfOpsPtr->UnloadNet = NULL;
    UtAssert_INT32_EQ(SBN_Cleanup(), SBN_SUCCESS);
    EVENT_CNT(1);

    IfOpsPtr->UnloadNet = UnloadNet_Nominal;
}

void Test_SBN_Cleanup_PeerErr(void)
{
    START();

    /* Test Case for:
     * SBN_Status_t SBN_Cleanup(void);
     */
    UT_SetHandlerFunction(UT_KEY(SBN_LocateNetRecordByID), SBN_UT_LocateNetRecordByID_Handler, NULL);
    UT_SetHandlerFunction(UT_KEY(SBN_LocatePeerRecordByID), SBN_UT_LocatePeerRecordByID_Handler, NULL);

    IfOpsPtr->UnloadPeer = NULL;
    UtAssert_INT32_EQ(SBN_Cleanup(), SBN_SUCCESS);

    IfOpsPtr->UnloadPeer = UnloadPeer_Nominal;
}

void UT_Setup(void)
{
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_AppMain_EVSRegisterErr);
    ADD_TEST(SBN_AppMain_AppIdErr);
    ADD_TEST(SBN_AppMain_TaskInfoErr);
    ADD_TEST(SBN_LoadConfTbl_RegisterErr);
    ADD_TEST(SBN_LoadConfTbl_LoadErr);
    ADD_TEST(SBN_LoadConfTbl_NotifyErr);
    ADD_TEST(SBN_LoadConf_GetAddrErr);
#ifdef jphfix
    ADD_TEST(SBN_LoadConf_ProtoInitErr);
    ADD_TEST(SBN_LoadConf_FilterInitErr);
    ADD_TEST(SBN_LoadConf_ProtoNameErr);
    ADD_TEST(SBN_LoadConf_FiltNameErr);
#endif
    ADD_TEST(SBN_LoadConf_ReleaseAddrErr);
    ADD_TEST(SBN_FindActiveProtocol);
    ADD_TEST(SBN_FindActiveFilter);
    ADD_TEST(SBN_InitInterfaces);
    ADD_TEST(SBN_ReloadConfTbl_NetConfErr1);
    ADD_TEST(SBN_ReloadConfTbl_NetConfErr2);
    ADD_TEST(SBN_ReloadConfTbl_NetConfErr3);
    ADD_TEST(SBN_ReloadConfTbl_InitProtoErr1);
    ADD_TEST(SBN_ReloadConfTbl_InitProtoErr2);
    ADD_TEST(SBN_ReloadConfTbl_InitFiltErr1);
    ADD_TEST(SBN_ReloadConfTbl_InitFiltErr2);
    ADD_TEST(SBN_ReloadConfTbl_InitErr);
    ADD_TEST(SBN_ReloadConfTbl_UnloadErr);
    ADD_TEST(SBN_AppMain_SubPipeCrErr);
    ADD_TEST(SBN_AppMain_SubPipeAllSubErr);
    ADD_TEST(SBN_AppMain_SubPipeOneSubErr);
    ADD_TEST(SBN_AppMain_CmdPipeCrErr);
    ADD_TEST(SBN_AppMain_CmdPipeSubErr);
    ADD_TEST(SBN_AppMain_DeletePipeErr);
    ADD_TEST(SBN_AppMain_SetupWorkerErr);
    ADD_TEST(SBN_InitBigEndian);
    ADD_TEST(SBN_InitLittleEndian);
    ADD_TEST(SBN_AppMain_RunLoop);
    ADD_TEST(SBN_AppMain_CleanupErr);
    ADD_TEST(SBN_GetPeerRecord);
    ADD_TEST(SBN_ReloadConfTbl);
    ADD_TEST(SBN_InitializePeerCounters);
    ADD_TEST(SBN_InitializeCounters);
    ADD_TEST(SBN_ValidateConfTbl);
    ADD_TEST(SBN_GetPeer);
    ADD_TEST(SBN_PackMsg);
    ADD_TEST(SBN_UnpackMsg);
    ADD_TEST(SBN_SendNetMsg);
    ADD_TEST(SBN_WaitForWakeup);
    ADD_TEST(SBN_Cleanup_NetErr);
    ADD_TEST(SBN_Cleanup_PeerErr);
}
