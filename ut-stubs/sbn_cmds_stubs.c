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

/**
 * @file
 *
 * Auto-Generated stub implementations for functions defined in sbn_cmds header
 */

#include "sbn_cmds.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_NoopCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_NoopCmd(const SBN_NoopCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_NoopCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_NoopCmd, const SBN_NoopCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_NoopCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_NoopCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_ReloadTblCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_ReloadTblCmd(const SBN_ReloadTblCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_ReloadTblCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_ReloadTblCmd, const SBN_ReloadTblCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_ReloadTblCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_ReloadTblCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_ResetCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_ResetCmd(const SBN_ResetCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_ResetCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_ResetCmd, const SBN_ResetCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_ResetCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_ResetCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_ResetPeerCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_ResetPeerCmd(const SBN_ResetPeerCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_ResetPeerCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_ResetPeerCmd, const SBN_ResetPeerCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_ResetPeerCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_ResetPeerCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_SendHkCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_SendHkCmd(const SBN_SendHkCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_SendHkCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_SendHkCmd, const SBN_SendHkCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_SendHkCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_SendHkCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_SendHkMySubsCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_SendHkMySubsCmd(const SBN_SendHkMySubsCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_SendHkMySubsCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_SendHkMySubsCmd, const SBN_SendHkMySubsCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_SendHkMySubsCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_SendHkMySubsCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_SendHkNetCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_SendHkNetCmd(const SBN_SendHkNetCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_SendHkNetCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_SendHkNetCmd, const SBN_SendHkNetCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_SendHkNetCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_SendHkNetCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_SendHkPeerCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_SendHkPeerCmd(const SBN_SendHkPeerCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_SendHkPeerCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_SendHkPeerCmd, const SBN_SendHkPeerCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_SendHkPeerCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_SendHkPeerCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_SendHkPeerSubsCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_SendHkPeerSubsCmd(const SBN_SendHkPeerSubsCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_SendHkPeerSubsCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_SendHkPeerSubsCmd, const SBN_SendHkPeerSubsCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_SendHkPeerSubsCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_SendHkPeerSubsCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SBN_WakeupCmd()
 * ----------------------------------------------------
 */
CFE_Status_t SBN_WakeupCmd(const SBN_WakeupCmd_t *MsgPtr)
{
    UT_GenStub_SetupReturnBuffer(SBN_WakeupCmd, CFE_Status_t);

    UT_GenStub_AddParam(SBN_WakeupCmd, const SBN_WakeupCmd_t *, MsgPtr);

    UT_GenStub_Execute(SBN_WakeupCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(SBN_WakeupCmd, CFE_Status_t);
}
