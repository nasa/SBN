/*
** File: sbn_coveragetest_common.h
**
** Purpose:
** Common definitions for all sbn coverage tests
*/


#ifndef _sbn_coveragetest_common_h_
#define _sbn_coveragetest_common_h_

/*
 * Includes
 */

#include <utassert.h>
#include <uttest.h>
#include <utstubs.h>

#include <cfe.h>

#include "sbn_app.h"
#include "sbn_types.h"
#include "sbn_interfaces.h"

/*
 * Macro to call a function and check its int32 return code
 */
#define UT_TEST_FUNCTION_RC(func,exp)           \
{                                               \
    int32 rcexp = exp;                          \
    int32 rcact = func;                         \
    UtAssert_True(rcact == rcexp, "%s (%ld) == %s (%ld)",   \
        #func, (long)rcact, #exp, (long)rcexp);             \
}

/*
 * Macro to add a test case to the list of tests to execute
 */
#define ADD_TEST(test) UtTest_Add((Test_ ## test),UT_Setup,UT_TearDown, #test)

/*
 * Setup function prior to every test
 */
void UT_Setup(void);

/*
 * Teardown function after every test
 */
void UT_TearDown(void);

#define EVENT_CNT(EVTCNT) UtAssert_True(EventTest.MatchCount == (EVTCNT), "EID generated (%d)", EventTest.MatchCount)

typedef struct
{
    uint16 ExpectedEvent;
    int MatchCount;
    const char *ExpectedText;
} UT_CheckEvent_t;

/*
 * A hook function to check for a specific event. If the passed event ID and text (if ExpectedText
 * is defined) match those configured, the MatchCount is incremented.
 */
int32 UT_CheckEvent_Hook(void *UserObj, int32 StubRetcode,
        uint32 CallCount, const UT_StubContext_t *Context, va_list va);

extern UT_CheckEvent_t EventTest;

/*
 * Helper function to set up for checking the events sent to CFE_EVS_SendEvent, incrementing
 * the MatchCount every time the event ID matches and the text matches. If ExpectedText is
 * NULL, only the IDs of events are checked.
 */
void UT_CheckEvent_Setup(uint16 ExpectedEvent, const char *ExpectedText);

CFE_Status_t ProtoInitModule_Nominal(int ProtoVersion, CFE_EVS_EventID_t BaseEID);
SBN_Status_t InitNet_Nominal(SBN_NetInterface_t *Net);
SBN_Status_t LoadNet_Nominal(SBN_NetInterface_t *Net, const char *Address);
SBN_Status_t InitPeer_Nominal(SBN_PeerInterface_t *Peer);
SBN_Status_t RecvFromNet_Nominal(SBN_NetInterface_t *Net, SBN_MsgType_t *MsgTypePtr, SBN_MsgSz_t *MsgSzPtr,
    CFE_ProcessorID_t *ProcessorIDPtr, void *PayloadBuffer);
SBN_Status_t LoadPeer_Nominal(SBN_PeerInterface_t *Peer, const char *Address);
SBN_Status_t UnloadNet_Nominal(SBN_NetInterface_t *Net);
SBN_Status_t UnloadPeer_Nominal(SBN_PeerInterface_t *Net);
SBN_Status_t PollPeer_Nominal(SBN_PeerInterface_t *Peer);
SBN_Status_t Send_Nominal(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload);
SBN_Status_t Send_Err(SBN_PeerInterface_t *Peer, SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Payload);
CFE_Status_t FilterInitModule_Nominal(int FilterVersion, CFE_EVS_EventID_t BaseEID);

extern SBN_IfOps_t *IfOpsPtr;
extern SBN_FilterInterface_t *FilterInterfacePtr;
extern SBN_ConfTbl_t *NominalTblPtr;
extern CFE_ProcessorID_t ProcessorID;
extern CFE_SpacecraftID_t SpacecraftID;
extern SBN_NetInterface_t *NetPtr;
extern SBN_PeerInterface_t *PeerPtr;

#define START() START_fn(__func__, __LINE__)
void START_fn(const char *func, int line);

#endif /* _sbn_coveragetest_common_h_ */
