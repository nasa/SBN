#include "sbn_tbl.h"
#include "cfe_tbl_filedef.h"

SBN_ConfTbl_t SBN_ConfTbl =
{
    .ProtocolModules =
    {
        { /* [0] */
            .Name = "UDP",
            .LibFileName = "/cf/sbn_udp.so",
            .LibSymbol = "SBN_UDP_Ops"
        },
        { /* [1] */
            .Name = "TCP",
            .LibFileName = "/cf/sbn_tcp.so",
            .LibSymbol = "SBN_TCP_Ops"
        }
    },
    .ProtocolCnt = 2,
    .FilterModules =
    {
        { /* [0] */
            .Name = "Filter_Test",
            .LibFileName = "/cf/sbn_filt_test.so",
            .LibSymbol = "SBN_Filter_Test"
        }
    },
    .FilterCnt = 1,
    .Peers = {
        { /* [0] */
            .ProcessorID = 1,
            .SpacecraftID = 42,
            .NetNum = 0,
            .ProtocolName = "UDP",
            .InFilters = {{
            }},
            .OutFilters = {{
                /* "Filter_Test" */
            }},
            .Address = "127.0.0.1:2234",
            .TaskFlags = SBN_TASK_POLL
        },
        { /* [1] */
            .ProcessorID = 2,
            .SpacecraftID = 42,
            .NetNum = 0,
            .ProtocolName = "UDP",
            .InFilters = {{
            }},
            .OutFilters = {{
                /* "Filter_Test" */
            }},
            .Address = "127.0.0.1:2235",
            .TaskFlags = SBN_TASK_POLL
        },
        { /* [2] */
            .ProcessorID = 3,
            .SpacecraftID = 42,
            .NetNum = 0,
            .ProtocolName = "UDP",
            .InFilters = {{
            }},
            .OutFilters = {{
                /* "Filter_Test" */
            }},
            .Address = "127.0.0.1:2236",
            .TaskFlags = SBN_TASK_POLL
        },
    },
    .PeerCnt = 3
};

CFE_TBL_FILEDEF(SBN_ConfTbl, SBN.SBN_ConfTbl, SBN Configuration Table, sbn_conf_tbl.tbl)
