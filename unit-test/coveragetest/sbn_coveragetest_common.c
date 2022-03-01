#include "sbn_coveragetest_common.h"

int32 UT_CheckEvent_Hook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context,
                         va_list va)
{
    UT_CheckEvent_t *State = UserObj;
    char             TestText[CFE_MISSION_EVS_MAX_MESSAGE_LENGTH];
    uint16           EventId;
    const char *     Spec;

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
            }
        }
    }

    return 0;
}

UT_CheckEvent_t EventTest;

void UT_CheckEvent_Setup(uint16 ExpectedEvent, const char *ExpectedText)
{
    memset(&EventTest, 0, sizeof(EventTest));
    EventTest.ExpectedEvent = ExpectedEvent;
    EventTest.ExpectedText  = ExpectedText;
    UT_SetVaHookFunction(UT_KEY(CFE_EVS_SendEvent), UT_CheckEvent_Hook, &EventTest);
}

SBN_Status_t ProtoInitModule_Nominal(int ProtoVersion, CFE_EVS_EventID_t BaseEID, SBN_ProtocolOutlet_t *Outlet)
{
    return CFE_SUCCESS;
} /* end ProtoInitModule_Nominal() */

SBN_Status_t InitNet_Nominal(SBN_NetInterface_t *Net)
{
    Net->Configured = true;
    return SBN_SUCCESS;
} /* end InitNet_Nominal() */

SBN_Status_t LoadNet_Nominal(SBN_NetInterface_t *Net, const char *Address)
{
    return SBN_SUCCESS;
} /* end InitNet_Nominal() */

SBN_Status_t InitPeer_Nominal(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
} /* end InitPeer_Nominal() */

SBN_Status_t RecvFromNet_Nominal(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
                                 CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer)
{
    *ProcessorIDPtr = 1235;

    return SBN_SUCCESS;
} /* end RecvFromNet_Nominal() */

SBN_Status_t LoadPeer_Nominal(SBN_PeerInterface_t *Peer, const char *Address)
{
    return SBN_SUCCESS;
} /* end LoadPeer_Nominal() */

SBN_Status_t UnloadNet_Nominal(SBN_NetInterface_t *Net)
{
    return SBN_SUCCESS;
} /* end UnloadNet_Nominal() */

SBN_Status_t UnloadPeer_Nominal(SBN_PeerInterface_t *Net)
{
    return SBN_SUCCESS;
} /* end UnloadPeer_Nominal() */

SBN_Status_t PollPeer_Nominal(SBN_PeerInterface_t *Peer)
{
    return SBN_SUCCESS;
} /* end PollPeer_Nominal() */

SBN_Status_t Send_Nominal(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload)
{
    return SBN_SUCCESS;
} /* end Send_Nominal() */

SBN_Status_t Send_Err(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload)
{
    return SBN_ERROR;
} /* end Send_Err() */

SBN_IfOps_t IfOps = {.InitModule   = ProtoInitModule_Nominal,
                     .InitNet      = InitNet_Nominal,
                     .InitPeer     = InitPeer_Nominal,
                     .LoadNet      = LoadNet_Nominal,
                     .LoadPeer     = LoadPeer_Nominal,
                     .PollPeer     = PollPeer_Nominal,
                     .Send         = Send_Nominal,
                     .RecvFromPeer = NULL,
                     .RecvFromNet  = RecvFromNet_Nominal,
                     .UnloadNet    = UnloadNet_Nominal,
                     .UnloadPeer   = UnloadPeer_Nominal}; /* end IfOps */

SBN_IfOps_t *IfOpsPtr = &IfOps;

SBN_Status_t FilterInitModule_Nominal(int FilterVersion, CFE_EVS_EventID_t BaseEID)
{
    return SBN_SUCCESS;
} /* end InitFilterModule() */

SBN_FilterInterface_t FilterInterface = {.InitModule = FilterInitModule_Nominal,
                                         .FilterRecv = NULL,
                                         .FilterSend = NULL,
                                         .RemapMID   = NULL}; /* end FilterInterface */

SBN_FilterInterface_t *FilterInterfacePtr = &FilterInterface;

/********************************** table ************************************/

SBN_ConfTbl_t NominalTbl = {.ProtocolModules = {{.Name        = "UDP",
                                                 .LibFileName = "/cf/sbn_udp.so",
                                                 .LibSymbol   = "SBN_UDP_Ops",
                                                 .BaseEID     = 0x0100}},
                            .ProtocolCnt     = 1,
                            .FilterModules   = {{.Name        = "CCSDS Endian",
                                               .LibFileName = "/cf/sbn_f_ccsds_end.so",
                                               .LibSymbol   = "SBN_F_CCSDS_End",
                                               .BaseEID     = 0x1000}},
                            .FilterCnt       = 1,

                            .Peers =
                                {
                                    {/* [0] */
                                     .ProcessorID  = 1234,
                                     .SpacecraftID = 5678,
                                     .NetNum       = 0,
                                     .ProtocolName = "UDP",
                                     .Filters      = {"CCSDS Endian"},
                                     .Address      = "127.0.0.1:2234",
                                     .TaskFlags    = SBN_TASK_POLL},
                                    {/* [1] */
                                     .ProcessorID  = 1235,
                                     .SpacecraftID = 5678,
                                     .NetNum       = 0,
                                     .ProtocolName = "UDP",
                                     .Filters      = {"CCSDS Endian"},
                                     .Address      = "127.0.0.1:2235",
                                     .TaskFlags    = SBN_TASK_POLL},
                                },
                            .PeerCnt = 2}; /* end NominalTbl */

SBN_ConfTbl_t *NominalTblPtr = &NominalTbl;

/********************************** globals ************************************/
CFE_ProcessorID_t    ProcessorID  = 1234;
CFE_SpacecraftID_t   SpacecraftID = 5678;
SBN_NetInterface_t * NetPtr       = NULL;
SBN_PeerInterface_t *PeerPtr      = NULL;

/* SBN tries to look up the symbol of the protocol or filter module using OS_SymbolLookup, if it
 * finds it (because ES loaded it) it does nothing; if it does not, it tries to load the symbol.
 * This hook forces the OS_SymbolLookup to fail the first time but succeed the second time and
 * load the symbol into the data buffer so that the "load" has been performed successfully.
 */
int32 SymLookHook(void *UserObj, int32 StubRetcode, uint32 CallCount, const UT_StubContext_t *Context)
{
    static char LastSeen[32] = {0};
    char *      SymbolName   = (char *)Context->ArgPtr[1];

    /* this forces the LoadConf_Module() function to call ModuleLoad */

    /* we've seen this symbol already, time to load */
    if (!strcmp(SymbolName, LastSeen))
    {
        if (!strcmp(SymbolName, "SBN_UDP_Ops"))
        {
            /* the stub will read this into the addr */
            UT_SetDataBuffer(UT_KEY(OS_SymbolLookup), &IfOpsPtr, sizeof(IfOpsPtr), false);
        }
        else if (!strcmp(SymbolName, "SBN_F_CCSDS_End"))
        {
            /* the stub will read this into the addr */
            UT_SetDataBuffer(UT_KEY(OS_SymbolLookup), &FilterInterfacePtr, sizeof(FilterInterfacePtr), false);
        } /* end if */

        return OS_SUCCESS;
    }
    else
    {
        strcpy(LastSeen, SymbolName); /* so that next call succeeds */

        return 1;
    } /* end if */
} /* end SymLookHook() */

void START_fn(const char *func, int line)
{
    UT_ResetState(0);
    printf("Start item %s (%d)\n", func, line);
    memset(&SBN, 0, sizeof(SBN));

    NetPtr                = &SBN.Nets[0];
    SBN.NetCnt            = 1;
    NetPtr->PeerCnt       = 1;
    NetPtr->Configured    = 1;
    PeerPtr               = &NetPtr->Peers[0];
    PeerPtr->ProcessorID  = ProcessorID;
    PeerPtr->SpacecraftID = SpacecraftID;
    PeerPtr->Net          = NetPtr;
    NetPtr->IfOps         = &IfOps;

    UT_SetHookFunction(UT_KEY(OS_SymbolLookup), SymLookHook, NULL);

    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &NominalTblPtr, sizeof(NominalTblPtr), false);
    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_GetAddress), 1, CFE_TBL_INFO_UPDATED);

    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetProcessorId), ProcessorID);

    UT_SetDefaultReturnValue(UT_KEY(CFE_PSP_GetSpacecraftId), SpacecraftID);
} /* end START_fn() */
