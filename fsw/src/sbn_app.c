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

/*
 ** Include Files
 */
#include <string.h>

#include "sbn_pack.h"
#include "sbn_app.h"
#include "cfe_es.h" /* PerfLog */
#include "cfe_psp_endian.h"
#include "cfe_platform_cfg.h"
#include "cfe_msgids.h"
#include "sbn_error.h"
#include "sbn_tbl.h"
#include "sbn_dispatch.h"
#include "sbn_module.h"
#include "sbn_filter.h"
#include "sbn_outbound.h"
#include "sbn_worker.h"
#include "sbn_inbound.h"
#include "sbn_connection.h"
#include "sbn_version.h"

/** \brief SBN global application data, indexed by AppID. */
SBN_AppData_t SBN_AppData;

static SBN_Status_t SBN_UnloadNets(void);

/**
 * Packs a CCSDS message with an SBN message header.
 *
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in network (big-endian) byte order.
 * \param[out] SBNBuf The buffer to pack into.
 * \param[in] MsgSz The size of the payload.
 * \param[in] MsgType The SBN message type.
 * \param[in] ProcessorID The ProcessorID of the sender (should be CFE_CPU_ID)
 * \param[in] SpacecraftID The SpacecraftID of the sender
 * \param[in] Msg The payload (CCSDS message or SBN sub/unsub.)
 */
void SBN_PackMsg(void             *SBNBuf,
                 SBN_MsgSz_t       MsgSz,
                 SBN_MsgType_t     MsgType,
                 SBN_ProcessorID_t ProcessorID,
                 SBN_ProcessorID_t SpacecraftID,
                 void             *Msg)
{
    SBN_PackState_t Pack;
    SBN_Pack_Init(&Pack, SBNBuf, MsgSz + SBN_PACKED_HDR_SZ, false);

    SBN_Pack_UInt32(&Pack, MsgSz);
    SBN_Pack_UInt8(&Pack, MsgType);
    SBN_Pack_UInt32(&Pack, ProcessorID);
    SBN_Pack_UInt32(&Pack, SpacecraftID);

    if (!Msg || !MsgSz)
    {
        /* valid to have a NULL pointer/empty size payload */
        return;
    } /* end if */

    SBN_Pack_Data(&Pack, Msg, MsgSz);
}

/**
 * Unpacks a CCSDS message with an SBN message header.
 *
 * \param[in] SBNBuf The buffer to unpack.
 * \param[out] MsgSzPtr The payload size.
 * \param[out] MsgTypePtr The SBN message type.
 * \param[out] ProcessorIDPtr The ProcessorID of the sender.
 * \param[out] SpacecraftIDPtr The SpacecraftID of the sender.
 * \param[out] Msg The payload (a CCSDS message, or SBN sub/unsub).
 * \return true if we were able to unpack the message.
 *
 * \note Ensures the SBN fields (CPU ID, MsgSz) and CCSDS message headers
 *       are in platform byte order.
 * \todo Use a type for SBNBuf.
 */
bool SBN_UnpackMsg(void               *SBNBuf,
                   SBN_MsgSz_t        *MsgSzPtr,
                   SBN_MsgType_t      *MsgTypePtr,
                   SBN_ProcessorID_t  *ProcessorIDPtr,
                   SBN_SpacecraftID_t *SpacecraftIDPtr,
                   void               *Msg)
{
    uint8             t;
    SBN_UnpackState_t Pack;

    SBN_Unpack_Init(&Pack, SBNBuf, SBN_MAX_PACKED_MSG_SZ);

    *MsgSzPtr = 0;
    SBN_Unpack_UInt32(&Pack, MsgSzPtr);

    t = 0;
    SBN_Unpack_UInt8(&Pack, &t);
    *MsgTypePtr = t;

    SBN_Unpack_UInt32(&Pack, ProcessorIDPtr);
    SBN_Unpack_UInt32(&Pack, SpacecraftIDPtr);

    if (!*MsgSzPtr)
    {
        return true;
    } /* end if */

    /* MsgSz is unsigned, no need to check for <0 */
    if (*MsgSzPtr > CFE_MISSION_SB_MAX_SB_MSG_SIZE)
    {
        return false;
    } /* end if */

    SBN_Unpack_Data(&Pack, Msg, *MsgSzPtr);

    return true;
}

/**
 * Sends a message to a peer using the module's send API.
 *
 * @param MsgType SBN type of the message
 * @param MsgSz Size of the message
 * @param Msg Message to send
 * @param Peer The peer to send the message to.
 * @return Number of characters sent on success, -1 on error.
 *
 */
SBN_Status_t SBN_SendNetMsg(SBN_MsgType_t MsgType, SBN_MsgSz_t MsgSz, void *Msg, SBN_PeerInterface_t *Peer)
{
    SBN_BufferDesc_t TempDesc;

    memset(&TempDesc, 0, sizeof(TempDesc));

    TempDesc.MsgType      = MsgType;
    TempDesc.ActualDataSz = MsgSz;
    TempDesc.MaxDataSz    = MsgSz;
    TempDesc.DataPtr      = Msg;

    return SBN_Outbound_SendToPeer(&TempDesc, Peer);
} /* end SBN_SendNetMsg */

/**
 * Loops through all hosts and peers, initializing all.
 *
 * @return SBN_SUCCESS if interface is initialized successfully
 *         SBN_ERROR otherwise
 */
SBN_Status_t SBN_InitInterfaces(void)
{
    SBN_NetIdx_t         NetIdx;
    SBN_NetRecord_t     *NetRec;
    SBN_NetInterface_t  *Net;
    SBN_PeerIdx_t        PeerIdx;
    SBN_PeerRecord_t    *PeerRec;
    SBN_PeerInterface_t *Peer;
    SBN_NetIdx_t         NetCnt;
    SBN_Status_t         Status;
    uint32               ErrCnt;

    NetCnt = 0;
    ErrCnt = 0;
    for (NetIdx = 0; NetIdx < SBN_AppData.NetCnt; NetIdx++)
    {
        NetRec = SBN_LocateNetRecordByExtIdx(NetIdx);
        Net    = SBN_Net_GetIntfData(NetRec);
        if (Net == NULL || !Net->Configured)
        {
            EVSSendErr(SBN_INIT_EID, "network #%u not configured (skipping)", (unsigned int)NetIdx);
        }
        else
        {
            EVSSendInfo(SBN_INIT_EID, "initializing net: %u", (unsigned int)NetIdx);

            if (Net->IfOps->InitNet)
            {
                Status = Net->IfOps->InitNet(Net);
            }
            else
            {
                Status = SBN_SUCCESS;
            }

            if (Status == SBN_SUCCESS)
            {
                ++NetCnt;
            }
            else
            {
                EVSSendErr(SBN_INIT_EID, "network #%u failed to init: %d", (unsigned int)NetIdx, (int)Status);
                ++ErrCnt;
            }

            EVSSendInfo(SBN_INIT_EID, "Net %d has %d peers", NetIdx, Net->PeerCnt);
            for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
            {
                PeerRec = SBN_LocatePeerRecordInNet(NetRec, PeerIdx);
                Peer    = SBN_Peer_GetIntfData(PeerRec);
                if (Peer == NULL)
                {
                    continue;
                }

                EVSSendInfo(SBN_PEER_EID,
                            "initializing net: %d peer: %d: sc: %d cpu: %d",
                            (int)NetIdx,
                            (int)PeerIdx,
                            Peer->SpacecraftID,
                            Peer->ProcessorID);

                if (Net->IfOps->InitPeer)
                {
                    Status = Net->IfOps->InitPeer(Peer);
                }
                else
                {
                    Status = SBN_SUCCESS;
                }

                if (Status != SBN_SUCCESS)
                {
                    EVSSendErr(SBN_INIT_EID,
                               "peer #%u on net #%u failed to init: %d",
                               (unsigned int)PeerIdx,
                               (unsigned int)NetIdx,
                               (int)Status);
                    ++ErrCnt;
                }
            }
        }
    } /* end for */

    EVSSendInfo(SBN_INIT_EID, "configured, %d nets", (int)NetCnt);

    if (ErrCnt != 0)
    {
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
} /* end SBN_InitInterfaces */

/**
 * This function waits for the scheduler (SCH) to wake this code up, so that
 * nothing transpires until the cFE is fully operational.
 *
 * @param[in] iTimeOut The time to wait for the scheduler to notify this code.
 * @return CFE_SUCCESS on success, otherwise an error value.
 */
SBN_Status_t SBN_WaitForWakeup(int32 iTimeOut)
{
    CFE_Status_t           CFE_Status;
    SBN_Status_t           SBN_Status;
    const CFE_SB_Buffer_t *BufPtr;

    /* Wait for WakeUp messages from scheduler */
    CFE_Status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&BufPtr, SBN_AppData.CmdPipe, iTimeOut);

    switch (CFE_Status)
    {
        case CFE_SB_NO_MESSAGE:
        case CFE_SB_TIME_OUT:
            break;
        case CFE_SUCCESS:
            SBN_HandleCommand(BufPtr);
            break;
        default:
            return SBN_ERROR;
    } /* end switch */

    /* For sbn, we still want to perform cyclic processing
    ** if the SBN_WaitForWakeup time out
    ** cyclic processing at timeout rate
    */
    CFE_ES_PerfLogEntry(SBN_PERF_RECV_ID);

    SBN_Inbound_PollCycle();

    SBN_Status = SBN_Subs_PollCycle();
    switch (SBN_Status)
    {
        case SBN_IF_EMPTY:
            break;
        case SBN_ERROR:
            EVSSendErr(SBN_PEER_EID, "SBN_Subs_PollCycle failed.");
            break;
        default:
            /* no error */
            break;
    };

    SBN_Subs_SynchronizeAllPeers();

    SBN_Outbound_PollCycle();
    SBN_Connection_PollCycle();

    CFE_ES_PerfLogExit(SBN_PERF_RECV_ID);

    return SBN_SUCCESS;
} /* end SBN_WaitForWakeup */

SBN_Status_t SBN_ValidateConfTbl(void *TableData)
{
    const SBN_ConfTbl_t    *PendingTbl = TableData;
    const SBN_Peer_Entry_t *PeerConf;
    SBN_ModuleIdx_t         ModuleIdx;
    SBN_PeerIdx_t           PeerIdx;
    SBN_PeerIdx_t           PeerCounts[SBN_PLATFORM_MAX_NETS];
    const char             *NetProtoName[SBN_PLATFORM_MAX_NETS];
    uint32                  NumErrs;

    if (PendingTbl->ProtocolCnt > SBN_MISSION_MAX_MOD_CNT || PendingTbl->FilterCnt > SBN_MISSION_MAX_MOD_CNT
        || PendingTbl->PeerCnt > SBN_MISSION_MAX_PEER_CNT)
    {
        /* Invalid count */
        return SBN_ERROR;
    }

    memset(PeerCounts, 0, sizeof(PeerCounts));
    memset(NetProtoName, 0, sizeof(NetProtoName));
    NumErrs = 0;

    for (PeerIdx = 0; PeerIdx < PendingTbl->PeerCnt; ++PeerIdx)
    {
        PeerConf = &PendingTbl->Peers[PeerIdx];

        /* Make sure the peer refers to a valid protocol in the protocol section */
        for (ModuleIdx = 0; ModuleIdx < PendingTbl->ProtocolCnt; ++ModuleIdx)
        {
            if (strncmp(PendingTbl->ProtocolModules[ModuleIdx].Name,
                        PeerConf->ProtocolName,
                        sizeof(PeerConf->ProtocolName))
                == 0)
            {
                break;
            }
        }

        if (ModuleIdx == PendingTbl->ProtocolCnt)
        {
            /* do not print the string since we do not know if its null terminated */
            EVSSendErr(SBN_TBL_EID, "invalid module name on peer %u", (unsigned int)PeerIdx);
            ++NumErrs;
        }

        if (PeerConf->NetNum >= SBN_PLATFORM_MAX_NETS)
        {
            EVSSendErr(SBN_TBL_EID, "network index too large (%d>%d)", PeerConf->NetNum, SBN_PLATFORM_MAX_NETS);
            ++NumErrs;
        } /* end if */
        else
        {
            ++PeerCounts[PeerConf->NetNum];

            if (NetProtoName[PeerConf->NetNum] == NULL)
            {
                NetProtoName[PeerConf->NetNum] = PeerConf->ProtocolName;
            }
            else if (strncmp(NetProtoName[PeerConf->NetNum], PeerConf->ProtocolName, sizeof(PeerConf->ProtocolName))
                     != 0)
            {
                /* do not print the string since we do not know if its null terminated */
                EVSSendErr(SBN_TBL_EID,
                           "protocol mismatch on peer %u (net %u)",
                           (unsigned int)PeerIdx,
                           (unsigned int)PeerConf->NetNum);
                ++NumErrs;
            }
        }
    }

    if (NumErrs > 0)
    {
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

static void SBN_InitProtoModules(const SBN_ConfTbl_t *ConfTbl)
{
    SBN_Status_t              Status;
    const SBN_Module_Entry_t *ModEntry;
    SBN_ModuleIdx_t           ModuleIdx;
    const SBN_IfOps_t        *ProtoOps;
    SBN_ProtocolModInfo_t    *LocalInfo;
    char                      SymbolName[CFE_MISSION_MAX_API_LEN];
    SBN_ProtocolOutlet_t      Outlet = { .PackMsg      = SBN_PackMsg,
                                         .UnpackMsg    = SBN_UnpackMsg,
                                         .Connected    = SBN_Connection_HandleConnect,
                                         .Disconnected = SBN_Connection_HandleDisconnect,
                                         .SendNetMsg   = SBN_SendNetMsg,
                                         .GetPeer      = SBN_GetPeer };

    /* load protocol modules */
    EVSSendDbg(SBN_TBL_EID, "Loading protocol modules...");
    ModEntry = ConfTbl->ProtocolModules;
    for (ModuleIdx = 0; ModuleIdx < SBN_MISSION_MAX_MOD_CNT && ModuleIdx < ConfTbl->ProtocolCnt; ++ModuleIdx)
    {
        LocalInfo = &SBN_AppData.ProtocolModInfo[ModuleIdx];
        CFE_SB_MessageStringGet(LocalInfo->ProtocolName,
                                ModEntry->Name,
                                "",
                                sizeof(LocalInfo->ProtocolName),
                                sizeof(ModEntry->Name));
        CFE_SB_MessageStringGet(SymbolName, ModEntry->LibSymbol, "", sizeof(SymbolName), sizeof(ModEntry->LibSymbol));

        Status = SBN_Module_GetProtocolIf(SymbolName, &ProtoOps);
        if (Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID,
                       "Failed to map protocol \'%s\' via symbol \'%s\' (at index %u)",
                       LocalInfo->ProtocolName,
                       SymbolName,
                       (unsigned int)ModuleIdx);
        }
        else
        {
            EVSSendInfo(SBN_TBL_EID, "initializing protocol module %s", LocalInfo->ProtocolName);
            Status = ProtoOps->InitModule(SBN_PROTOCOL_VERSION, ModEntry->BaseEID, &Outlet);
            if (Status != SBN_SUCCESS)
            {
                EVSSendErr(SBN_TBL_EID, "error in protocol init");
            }
            else
            {
                EVSSendInfo(SBN_TBL_EID, "protocol module %s initialized", LocalInfo->ProtocolName);
                LocalInfo->Intf = ProtoOps;
            }
        }
        ++ModEntry;

    } /* end for */
}

static void SBN_InitFilterModules(const SBN_ConfTbl_t *ConfTbl)
{
    SBN_Status_t                 Status;
    const SBN_Module_Entry_t    *ModEntry;
    SBN_ModuleIdx_t              ModuleIdx;
    const SBN_FilterInterface_t *FiltOps;
    SBN_FilterModInfo_t         *LocalInfo;
    char                         SymbolName[CFE_MISSION_MAX_API_LEN];

    /* load filter modules */
    EVSSendDbg(SBN_TBL_EID, "Loading filter modules...");
    ModEntry = ConfTbl->FilterModules;
    for (ModuleIdx = 0; ModuleIdx < SBN_MISSION_MAX_MOD_CNT && ModuleIdx < ConfTbl->FilterCnt; ++ModuleIdx)
    {
        LocalInfo = &SBN_AppData.FilterModInfo[ModuleIdx];
        CFE_SB_MessageStringGet(LocalInfo->FilterName,
                                ModEntry->Name,
                                "",
                                sizeof(LocalInfo->FilterName),
                                sizeof(ModEntry->Name));
        CFE_SB_MessageStringGet(SymbolName, ModEntry->LibSymbol, "", sizeof(SymbolName), sizeof(ModEntry->LibSymbol));

        Status = SBN_Module_GetFilterIf(SymbolName, &FiltOps);
        if (Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_TBL_EID,
                       "Failed to map filter \'%s\' via symbol \'%s\' (at index %u)",
                       LocalInfo->FilterName,
                       SymbolName,
                       (unsigned int)ModuleIdx);
        }
        else
        {
            EVSSendInfo(SBN_TBL_EID, "initializing filter module %s", LocalInfo->FilterName);
            Status = FiltOps->InitModule(SBN_FILTER_VERSION, ModEntry->BaseEID);
            if (Status != SBN_SUCCESS)
            {
                EVSSendErr(SBN_TBL_EID, "error in filter init");
            }
            else
            {
                EVSSendInfo(SBN_TBL_EID, "filter module %s initialized", LocalInfo->FilterName);
                LocalInfo->Intf = FiltOps;
            }
        }
        ++ModEntry;
    }
}

const SBN_ProtocolModInfo_t *SBN_FindActiveProtocol(const char *ProtocolName)
{
    const SBN_ProtocolModInfo_t *ProtoEntry;
    SBN_ModuleIdx_t              ModuleIdx;

    ProtoEntry = SBN_AppData.ProtocolModInfo;
    ModuleIdx  = 0;

    while (true)
    {
        if (ModuleIdx >= SBN_MISSION_MAX_MOD_CNT)
        {
            ProtoEntry = NULL;
            break;
        }
        if (strcmp(ProtoEntry->ProtocolName, ProtocolName) == 0)
        {
            break;
        }
        ++ModuleIdx;
        ++ProtoEntry;
    }

    /* only return it if it was successfully set up */
    if (ProtoEntry != NULL && ProtoEntry->Intf == NULL)
    {
        ProtoEntry = NULL;
    }

    return ProtoEntry;
}

const SBN_FilterModInfo_t *SBN_FindActiveFilter(const char *FilterName)
{
    const SBN_FilterModInfo_t *FilterEntry;
    SBN_ModuleIdx_t            ModuleIdx;

    FilterEntry = SBN_AppData.FilterModInfo;
    ModuleIdx   = 0;

    while (true)
    {
        if (ModuleIdx >= SBN_MISSION_MAX_MOD_CNT)
        {
            FilterEntry = NULL;
            break;
        }
        if (strcmp(FilterEntry->FilterName, FilterName) == 0)
        {
            break;
        }
        ++ModuleIdx;
        ++FilterEntry;
    }

    /* only return it if it was successfully set up */
    if (FilterEntry != NULL && FilterEntry->Intf == NULL)
    {
        FilterEntry = NULL;
    }

    return FilterEntry;
}

static void SBN_InitPeersFromConfig(const SBN_ConfTbl_t *ConfTbl)
{
    const SBN_Peer_Entry_t       *PeerConf;
    const SBN_ProtocolModInfo_t  *ProtoEntry;
    const SBN_FilterModInfo_t    *FilterEntry;
    const SBN_FilterInterface_t **FilterList;
    char                          FilterNameTemp[CFE_MISSION_MAX_API_LEN];
    SBN_PeerRecord_t             *PeerRec;
    SBN_NetRecord_t              *NetRec;
    SBN_PeerInterface_t          *Peer;
    SBN_NetInterface_t           *Net;
    SBN_NetIdx_t                  MaxNet;
    SBN_PeerIdx_t                 PeerIdx;
    uint8                         FilterIdx;

    union
    {
        char Name[CFE_MISSION_MAX_API_LEN];
        char Address[SBN_MISSION_ADDR_SZ];
    } LocalBuf;

    /* load nets and peers */
    for (PeerIdx = 0; PeerIdx < SBN_MISSION_MAX_PEER_CNT && PeerIdx < ConfTbl->PeerCnt; PeerIdx++)
    {
        PeerConf   = &ConfTbl->Peers[PeerIdx];
        FilterList = NULL;
        PeerRec    = NULL;
        NetRec     = NULL;
        Peer       = NULL;
        Net        = NULL;

        EVSSendInfo(SBN_TBL_EID, "configuring peer (SC=%d, CPU=%d)...", PeerConf->SpacecraftID, PeerConf->ProcessorID);

        CFE_SB_MessageStringGet(LocalBuf.Name,
                                PeerConf->ProtocolName,
                                "",
                                sizeof(LocalBuf),
                                sizeof(PeerConf->ProtocolName));
        ProtoEntry = SBN_FindActiveProtocol(LocalBuf.Name);

        if (ProtoEntry == NULL)
        {
            EVSSendCrit(SBN_TBL_EID,
                        "protocol %s is not available (skipping peer %u)",
                        LocalBuf.Name,
                        (unsigned int)PeerIdx);
        }
        else
        {
            /* note: the "NetNum" in the table was validated already by the validation routine */
            if (!CFE_RESOURCEID_TEST_DEFINED(SBN_AppData.NetIds[PeerConf->NetNum]))
            {
                SBN_RegisterNet(&SBN_AppData.NetIds[PeerConf->NetNum]);
            }
            NetRec = SBN_LocateNetRecordByID(SBN_AppData.NetIds[PeerConf->NetNum]);
            if (!SBN_NetRecordIsMatch(NetRec, SBN_AppData.NetIds[PeerConf->NetNum]))
            {
                EVSSendCrit(SBN_TBL_EID, "no net record found (skipping peer %u)", (unsigned int)PeerIdx);
                continue;
            }

            Net = SBN_Net_GetIntfData(NetRec);

            CFE_SB_MessageStringGet(LocalBuf.Address,
                                    (const char *)PeerConf->Address,
                                    "",
                                    sizeof(LocalBuf),
                                    sizeof(PeerConf->Address));

            /* Reset peer count since we're initializing the net */
            if (PeerConf->ProcessorID == CFE_PSP_GetProcessorId()
                && PeerConf->SpacecraftID == CFE_PSP_GetSpacecraftId())
            {
                EVSSendInfo(SBN_TBL_EID, "peer is this processor: loading net %u", (unsigned int)PeerConf->NetNum);

                Net->ProtocolIdx = (ProtoEntry - SBN_AppData.ProtocolModInfo);
                Net->IfOps       = ProtoEntry->Intf;
                Net->TaskFlags   = PeerConf->TaskFlags;
                Net->Configured  = true;

                ProtoEntry->Intf->LoadNet(Net, LocalBuf.Address);

                FilterList = Net->Filters;

                /* If this RecvFromNet is provided then inbound traffic is scheduled at the net level */
                NetRec->Sched.InboundSchedulingEnabled = (ProtoEntry->Intf->RecvFromNet != NULL);
            }
            else
            {
                EVSSendInfo(SBN_TBL_EID,
                            "peer is other processor: loading peer onto net %u",
                            (unsigned int)PeerConf->NetNum);

                /* Note that it was already validated that this cannot exceed the max peer count */
                SBN_RegisterPeer(&Net->PeerIds[Net->PeerCnt]);
                PeerRec = SBN_LocatePeerRecordByID(Net->PeerIds[Net->PeerCnt]);
                if (!SBN_PeerRecordIsMatch(PeerRec, Net->PeerIds[Net->PeerCnt]))
                {
                    EVSSendCrit(SBN_TBL_EID, "no peer record found (skipping peer %u)", (unsigned int)PeerIdx);
                    continue;
                }

                PeerRec->NetId = NetRec->NetId;

                Peer = SBN_Peer_GetIntfData(PeerRec);

                Peer->Net          = Net;
                Peer->ProcessorID  = PeerConf->ProcessorID;
                Peer->SpacecraftID = PeerConf->SpacecraftID;
                Peer->TaskFlags    = PeerConf->TaskFlags;
                ++Net->PeerCnt;

                ProtoEntry->Intf->LoadPeer(Peer, LocalBuf.Address);

                FilterList = Peer->Filters;

                /* If this RecvFromPeer is provided then inbound traffic is scheduled at the peer level */
                PeerRec->Sched.InboundSchedulingEnabled = (ProtoEntry->Intf->RecvFromPeer != NULL);
            }
        }

        /* Set up the filters for this peer */
        if (FilterList != NULL)
        {
            for (FilterIdx = 0; FilterIdx < SBN_MISSION_MAX_FILTERS_PER_PEER; ++FilterIdx)
            {
                CFE_SB_MessageStringGet(FilterNameTemp,
                                        PeerConf->Filters[FilterIdx],
                                        "",
                                        sizeof(FilterNameTemp),
                                        sizeof(PeerConf->Filters[FilterIdx]));
                if (FilterNameTemp[0] != 0)
                {
                    FilterEntry = SBN_FindActiveFilter(FilterNameTemp);
                    if (FilterEntry == NULL)
                    {
                        EVSSendInfo(SBN_TBL_EID,
                                    "cannot map filter %s at index %u peer %u",
                                    FilterNameTemp,
                                    (unsigned int)FilterIdx,
                                    (unsigned int)PeerIdx);
                    }
                    else
                    {
                        *FilterList = FilterEntry->Intf;
                        ++FilterList;
                    }
                }
            }
        }
    }

    /* Now find the MAX configured net number.  This is mainly for efficiency, to limit the looping at runtime. */
    MaxNet = SBN_PLATFORM_MAX_NETS;
    while (MaxNet != 0)
    {
        if (CFE_RESOURCEID_TEST_DEFINED(SBN_AppData.NetIds[MaxNet - 1]))
        {
            break;
        }
        --MaxNet;
    }

    SBN_AppData.NetCnt = MaxNet;
}

static SBN_Status_t SBN_LoadConf(void)
{
    void        *TableAddr;
    CFE_Status_t Status;

    /* wipe the runtime info structs as we are re-loading them */
    memset(SBN_AppData.ProtocolModInfo, 0, sizeof(SBN_AppData.ProtocolModInfo));
    memset(SBN_AppData.FilterModInfo, 0, sizeof(SBN_AppData.FilterModInfo));
    memset(SBN_AppData.NetIds, 0, sizeof(SBN_AppData.NetIds));
    memset(SBN_AppData.PeerRecords, 0, sizeof(SBN_AppData.PeerRecords));
    memset(SBN_AppData.NetRecords, 0, sizeof(SBN_AppData.NetRecords));

    SBN_AppData.NetCnt = 0;

    TableAddr = NULL;
    Status    = CFE_TBL_GetAddress(&TableAddr, SBN_AppData.ConfTblHandle);

    if (Status < CFE_SUCCESS || TableAddr == NULL)
    {
        EVSSendErr(SBN_TBL_EID, "unable to get conf table address");
        return SBN_ERROR;
    } /* end if */

    SBN_InitProtoModules(TableAddr);
    SBN_InitFilterModules(TableAddr);
    SBN_InitPeersFromConfig(TableAddr);

    /* address only needed at load time, release */
    if (CFE_TBL_ReleaseAddress(SBN_AppData.ConfTblHandle) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to release address of conf tbl");
        return SBN_ERROR;
    } /* end if */

    /* ...but we keep the handle so we can be notified of updates */
    return SBN_SUCCESS;
} /* end SBN_LoadConf() */

static SBN_Status_t SBN_UnloadConf(void)
{
    SBN_Status_t Status;

    EVSSendInfo(SBN_TBL_EID, "unloading configuration");

    Status = SBN_UnloadNets();
    if (Status != SBN_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to unload nets");
    }

    return Status;
} /* end SBN_UnloadConf() */

static SBN_Status_t SBN_UnloadPeer(SBN_PeerRecord_t *PeerRec)
{
    SBN_PeerInterface_t *Peer = SBN_Peer_GetIntfData(PeerRec);
    SBN_NetInterface_t  *Net  = Peer->Net;

    SBN_Connection_HandleDisconnect(Peer);
    SBN_Subs_UpdateDisconnectedPeerLocalSubs(PeerRec);

    if (Net->IfOps->UnloadPeer)
    {
        Net->IfOps->UnloadPeer(Peer);
    }

    // Reset peer description
    Peer->ProcessorID  = 0;
    Peer->SpacecraftID = 0;
    Peer->Net          = NULL;
    Peer->TaskFlags    = 0;
    Peer->Pipe         = CFE_SB_INVALID_PIPE;
    Peer->FilterCnt    = 0;

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_DoUnloadNet(SBN_NetRecord_t *NetRec, void *Arg)
{
    uint32             *Errs = Arg;
    SBN_NetInterface_t *Net;
    SBN_Status_t        Status;
    SBN_PeerIdx_t       PeerIdx;
    SBN_PeerRecord_t   *PeerRec;

    Net = SBN_Net_GetIntfData(NetRec);

    Net->Configured = false;

    for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        PeerRec = SBN_LocatePeerRecordByID(Net->PeerIds[PeerIdx]);
        if (SBN_PeerRecordIsMatch(PeerRec, Net->PeerIds[PeerIdx]))
        {
            SBN_UnloadPeer(PeerRec);

            SBN_PeerRecordSetFree(PeerRec);
        }

        Net->PeerIds[PeerIdx] = SBN_PEERID_UNDEFINED;
    }

    // Peers were cleared, reset the count
    Net->PeerCnt = 0;

    /* Note -- if the net was not ever correctly loaded, the Ops pointer can be NULL */
    if (Net->IfOps && Net->IfOps->UnloadNet)
    {
        Status = Net->IfOps->UnloadNet(Net);
    }
    else
    {
        Status = SBN_SUCCESS;
    }
    if (Status != SBN_SUCCESS)
    {
        EVSSendCrit(SBN_TBL_EID, "unable to unload network %d", SBN_NetId_AsInt(NetRec->NetId));
        ++(*Errs);
    }
    else
    {
        EVSSendInfo(SBN_TBL_EID, "Terminated net: %d", SBN_NetId_AsInt(NetRec->NetId));
    }

    SBN_NetRecordSetFree(NetRec);

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_UnloadNets(void)
{
    uint32 Errs;

    Errs = 0;

    SBN_ForEachConfiguredNet(SBN_DoUnloadNet, &Errs);

    if (Errs != 0)
    {
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_LoadConfTbl(void)
{
    CFE_Status_t Status;

    Status = CFE_TBL_Register(&SBN_AppData.ConfTblHandle,
                              "ConfTbl",
                              sizeof(SBN_ConfTbl_t),
                              CFE_TBL_OPT_DEFAULT,
                              SBN_ValidateConfTbl);
    if (Status < CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to register conf tbl handle");
        return SBN_ERROR;
    } /* end if */

    Status = CFE_TBL_NotifyByMessage(SBN_AppData.ConfTblHandle, CFE_SB_ValueToMsgId(SBN_CMD_MID), SBN_TBL_CC, 0);
    if (Status < CFE_SUCCESS)
    {
        EVSSendErr(SBN_TBL_EID, "unable to set notifybymessage for conf tbl");
        return SBN_ERROR;
    } /* end if */

    Status = CFE_TBL_Load(SBN_AppData.ConfTblHandle, CFE_TBL_SRC_FILE, SBN_PLATFORM_CONF_TBL_FILENAME);
    if (Status < CFE_SUCCESS)
    {
        /* This is not a fatal error, the table can be loaded later */
        EVSSendErr(SBN_TBL_EID, "unable to load conf tbl %s", SBN_PLATFORM_CONF_TBL_FILENAME);
    }

    return SBN_SUCCESS;
} /* end SBN_LoadConfTbl() */

static SBN_Status_t SBN_TeardownSubPipe(void)
{
    CFE_Status_t Status;

    if (CFE_RESOURCEID_TEST_DEFINED(SBN_AppData.SubPipe))
    {
        /* Delete pipe for subscribes and unsubscribes from SB */
        Status = CFE_SB_DeletePipe(SBN_AppData.SubPipe);
        if (Status != CFE_SUCCESS)
        {
            EVSSendErr(SBN_INIT_EID, "failed to delete subscription pipe (Status=%d)", (int)Status);
            return SBN_ERROR;
        }

        SBN_AppData.SubPipe = CFE_SB_INVALID_PIPE;
    }

    return SBN_SUCCESS;
}

static SBN_Status_t SBN_SetupSubPipe(void)
{
    CFE_Status_t Status;

    /* Create pipe for subscribes and unsubscribes from SB */
    Status = CFE_SB_CreatePipe(&SBN_AppData.SubPipe, SBN_PLATFORM_SUB_PIPE_DEPTH, "SBNSubPipe");
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to create subscription pipe (Status=%d)", (int)Status);
        SBN_AppData.SubPipe = CFE_SB_INVALID_PIPE;
        return SBN_ERROR;
    } /* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID),
                                   SBN_AppData.SubPipe,
                                   SBN_PLATFORM_MAX_ALLSUBS_PKTS_ON_PIPE);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to allsubs (Status=%d)", (int)Status);
        return SBN_ERROR;
    } /* end if */

    Status = CFE_SB_SubscribeLocal(CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID),
                                   SBN_AppData.SubPipe,
                                   SBN_PLATFORM_MAX_ONESUB_PKTS_ON_PIPE);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "failed to subscribe to sub (Status=%d)", (int)Status);
        return SBN_ERROR;
    }

    return SBN_SUCCESS;
}

/**
 * @brief Initializes the housekeeping counters for a peer.
 *
 * @param[in] Peer The peer interface for which to reset housekeeping.
 */
static SBN_Status_t SBN_DoInitializePeerCounters(SBN_PeerRecord_t *PeerRec, void *Arg)
{
    SBN_PeerInterface_t *Peer = SBN_Peer_GetIntfData(PeerRec);

    Peer->LastSend = OS_TIME_ZERO;
    Peer->LastRecv = OS_TIME_ZERO;

    /* to be more thread safe, instead of zeroing these counts, take a snapshot */
    PeerRec->Priv.LastZero.RecvCnt    = Peer->Counts.RecvCnt;
    PeerRec->Priv.LastZero.RecvErrCnt = Peer->Counts.RecvErrCnt;
    PeerRec->Priv.LastZero.SendCnt    = Peer->Counts.SendCnt;
    PeerRec->Priv.LastZero.SendErrCnt = Peer->Counts.SendErrCnt;

    return SBN_SUCCESS;
}

/**
 * @brief Initializes the housekeeping counters both at the app level
 *        and for each peer.
 */
void SBN_InitializeCounters(void)
{
    SBN_AppData.CmdCnt    = 0;
    SBN_AppData.CmdErrCnt = 0;

    /* Wipe the counters on every peer */
    SBN_ForEachConfiguredPeer(SBN_DoInitializePeerCounters, NULL);
}

/**
 * @brief Initializes the housekeeping counters for a peer.
 *
 * @param[in] Peer The peer interface for which to reset housekeeping.
 */
void SBN_InitializePeerCounters(SBN_PeerRecord_t *PeerRec)
{
    /* Wipe the counters on only this peer */
    SBN_DoInitializePeerCounters(PeerRec, NULL);
}

static SBN_Status_t SBN_Init(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not initialize SBN:";

    SBN_Status_t Status;

    /*
     * NOTE: Do not over-react if these fail. We do not need to abort
     * and kill the SBN app because of a config error.  Rather, the bad config
     * will be reported, and SBN can fall back to a passive or reduced operation
     * mode.  The configuration can then be fixed and reloaded.
     */

    /* Load the configuration from the table */
    Status = SBN_LoadConf();
    if (Status == SBN_SUCCESS)
    {
        Status = SBN_InitInterfaces();
        if (Status != SBN_SUCCESS)
        {
            EVSSendErr(SBN_INIT_EID, "%s unable to initialize interfaces", FAIL_PREFIX);
        }
    }

    const char *bit_order = CFE_PSP_IsBigEndian() ? "big-endian" : "little-endian";

    EVSSendInfo(SBN_INIT_EID,
                "initialized (ProcessorID=%d SpacecraftId=%d %s "
                "SBN_AppData.AppID=%d...",
                (int)CFE_PSP_GetProcessorId(),
                (int)CFE_PSP_GetSpacecraftId(),
                bit_order,
                (int)CFE_RESOURCEID_TO_ULONG(SBN_AppData.AppID));

    EVSSendInfo(SBN_INIT_EID, "...SBN_IDENT=%s CMD_MID=0x%04X)", SBN_IDENT, SBN_CMD_MID);

    EVSSendInfo(SBN_INIT_EID, "Re-sending SB subscription requests...");
    SBN_SendSubsRequests();

    EVSSendInfo(SBN_INIT_EID, "SBN initialized.");

    return Status;
}

SBN_Status_t SBN_Cleanup(void)
{
    SBN_Status_t Status;

    Status = SBN_TeardownSubPipe();
    if (Status != SBN_SUCCESS)
    {
        /* A failure here does not mean we should everything else uncleaned */
        EVSSendErr(SBN_PEER_EID, "unable to tear down sub pipe");
    }

    Status = SBN_UnloadConf();
    if (Status != SBN_SUCCESS)
    {
        /* A failure here does not mean we should everything else uncleaned */
        EVSSendErr(SBN_PEER_EID, "unable to unload configuration");
    }

    // cleanup subs
    memset(&SBN_AppData.SubSet, 0, sizeof(SBN_AppData.SubSet));

    if (CFE_TBL_HANDLE_IS_VALID(SBN_AppData.ConfTblHandle))
    {
        /* "best effort" cleanup -- if this fails, nothing else to do */
        CFE_TBL_Unregister(SBN_AppData.ConfTblHandle);
        SBN_AppData.ConfTblHandle = CFE_TBL_BAD_TABLE_HANDLE;
    }

    return SBN_SUCCESS;
}

/** \brief SBN Main Routine */
void SBN_AppMain(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not start SBN:";
    CFE_ES_TaskInfo_t TaskInfo      = { 0 };
    int32             Status;
    uint32            RunStatus;
    CFE_ES_AppId_t    AppID;
    CFE_ES_TaskId_t   TskId;

    memset(&SBN_AppData, 0, sizeof(SBN_AppData));

    RunStatus = CFE_ES_RunStatus_APP_RUN;
    AppID     = CFE_ES_APPID_UNDEFINED;

    if (CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER) != CFE_SUCCESS)
    {
        return;
    }

    if (CFE_ES_GetAppID(&AppID) != CFE_SUCCESS)
    {
        EVSSendCrit(SBN_INIT_EID, "%s unable to get AppID", FAIL_PREFIX);
        return;
    }

    SBN_AppData.AppID = AppID;

    /* load my TaskName so I can ignore messages I send out to SB */
    CFE_ES_GetTaskID(&TskId);
    Status = CFE_ES_GetTaskInfo(&TaskInfo, TskId);
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s SBN failed to get task info (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    strncpy(SBN_AppData.App_FullName, (const char *)TaskInfo.TaskName, OS_MAX_API_NAME - 1);
    SBN_AppData.App_FullName[OS_MAX_API_NAME - 1] = '\0';

    /* Create pipe for HK requests and gnd commands */
    /* TODO: make configurable depth */
    Status = CFE_SB_CreatePipe(&SBN_AppData.CmdPipe, 20, "SBNCmdPipe");
    if (Status != CFE_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s failed to create command pipe (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SBN_CMD_MID), SBN_AppData.CmdPipe);
    if (Status == CFE_SUCCESS)
    {
        EVSSendInfo(SBN_INIT_EID, "SBN subscribed to command pipe SBN_CMD_MID 0x%04X", SBN_CMD_MID);
    }
    else
    {
        EVSSendErr(SBN_INIT_EID, "%s failed to subscribe to command pipe (%d)", FAIL_PREFIX, (int)Status);
        return;
    } /* end if */

    /* Load the table */
    if (SBN_LoadConfTbl() != SBN_SUCCESS)
    {
        /* the SBN_LoadConfTbl() functions will generate events */
        EVSSendErr(SBN_INIT_EID, "%s failed to load configuration table", FAIL_PREFIX);
        return;
    }

    if (SBN_SetupSubPipe() != SBN_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s unable to set up subscription pipe", FAIL_PREFIX);
        return;
    }

    if (SBN_Worker_Setup() != SBN_SUCCESS)
    {
        EVSSendErr(SBN_INIT_EID, "%s unable to set up worker tasks", FAIL_PREFIX);
    }

    CFE_ES_WaitForStartupSync(10000);

    /* This sets up the nets and peers based on what is in the table, as a "best effort" */
    /* Notably - if this fails, then we will just proceed with watever nets/peers were valid,
     * or simply no nets or peers at all.  Thats OK.  SBN will not do anything useful without
     * any nets/peers, but it will sit idly until the table is reloaded with something valid.  */
    SBN_ReloadConfTbl();

    /* Loop Forever */
    while (CFE_ES_RunLoop(&RunStatus))
    {
        SBN_WaitForWakeup(SBN_PLATFORM_MAIN_LOOP_DELAY);
    } /* end while */

    SBN_Worker_Shutdown();

    /* Cleanup is best effort */
    SBN_Cleanup();

    CFE_ES_ExitApp(RunStatus);
} /* end SBN_AppMain */

SBN_PeerRecord_t *
SBN_GetPeerRecord(SBN_NetRecord_t *NetRec, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID)
{
    SBN_PeerIdx_t        PeerIdx;
    SBN_PeerRecord_t    *PeerRec;
    SBN_NetInterface_t  *Net;
    SBN_PeerInterface_t *Peer;

    Net = SBN_Net_GetIntfData(NetRec);

    for (PeerIdx = 0; PeerIdx < Net->PeerCnt; PeerIdx++)
    {
        PeerRec = SBN_LocatePeerRecordInNet(NetRec, PeerIdx);
        Peer    = SBN_Peer_GetIntfData(PeerRec);

        if (Peer != NULL && Peer->ProcessorID == ProcessorID && Peer->SpacecraftID == SpacecraftID)
        {
            return PeerRec;
        }
    }

    return NULL;
}

/**
 * Find the PeerIndex for a given ProcessorID and net.
 * @param[in] Net The network interface to search.
 * @param[in] ProcessorID The ProcessorID of the peer being sought.
 * @param[in] SpacecraftID The SpacecraftI of the peer being sought.
 * @return The Peer interface pointer, or NULL if not found.
 */
SBN_PeerInterface_t *
SBN_GetPeer(SBN_NetInterface_t *Net, SBN_ProcessorID_t ProcessorID, SBN_SpacecraftID_t SpacecraftID)
{
    /* Compatibility wrapper that works on intf level */
    return SBN_Peer_GetIntfData(SBN_GetPeerRecord(SBN_Net_GetPrivateData(Net), ProcessorID, SpacecraftID));
} /* end SBN_GetPeer */

/** \brief Reload configuration and re-initialize
 * This is only called after receiving a message from TBL service.
 */
SBN_Status_t SBN_ReloadConfTbl(void)
{
    static const char FAIL_PREFIX[] = "ERROR: could not reload SBN configuration:";

    SBN_Status_t Status;

    EVSSendInfo(SBN_TBL_EID, "re-initializing SBN with new configuration...");

    /* This will pause all workers.  This means they will empty the current queue,
     * and no new jobs can be scheduled.  This blocks until the queue is empty, so
     * When this returns, all workers are idle and SBN is effectively single threaded. */
    SBN_Worker_Pause();

    Status = SBN_UnloadConf();
    if (Status != SBN_SUCCESS)
    {
        /* A failure here does not mean we should everything else uncleaned */
        EVSSendErr(SBN_PEER_EID, "unable to unload configuration, status=%d", (int)Status);
    }

    /* This is intentional -- try to re-init regardless of how the cleanup went */
    Status = SBN_Init();
    if (Status != SBN_SUCCESS)
    {
        EVSSendErr(SBN_PEER_EID, "%s status=%d", FAIL_PREFIX, (int)Status);
    }
    else
    {
        EVSSendInfo(SBN_TBL_EID, "SBN re-initialized successfully.");
    }

    SBN_Worker_Resume();

    return Status;
} /* end SBN_ReloadConfTbl() */
