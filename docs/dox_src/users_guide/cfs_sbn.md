Core Flight Software (cFS) Software Bus Networking (SBN)
========================================================

Version History
---------------
This document details the design and use of the SBN application. This is current
as of SBN version 1.6.

This document is maintained by [Chris Knight, NASA Ames Research Center]
(Christopher.D.Knight@nasa.gov).

Overview
--------
SBN is a cFS application that connects the local software bus to one or more
other cFS nodes (who are also running SBN) such that all messages sent by an
application on one bus will be received by an application on another bus. SBN
has a modular *network* architecture (TCP, UDP, Serial, SpaceWire, etc.) to
connect *peers* and supports multiple peer networks with a local *host*
connection affiliated with each. SBN also remaps and filters messages (cFS
table-configured.)

SBN Build and Configuration
---------------------------

SBN is built like any other cFS application, either via specifying it in 
the the `TGT#_APPLISTS` parameter in the targets.cmake (for the CMake build)
or `THE_APPS` in the Makefile (for the "classic" build). Protocol modules
(`sbn_udp`, `sbn_tcp`, `sbn_dtn`, ...) should also be specified as an
application in the build process (and the module should be linked or copied
to the apps source directory). SBN must be defined as an application
in the `cfe_es_startup.scr` script but Modules should *not* be defined there.

SBN uses two runtime text configuration files (in the same model as
`cfe_es_startup.scr`, with one line per configuration and multiple columns,
separated by commas.) `SbnModuleData.dat` specifies the protocol modules
SBN should load at startup. `SbnPeerData.dat` specifies which protocol
and which protocol options (such as IP address, port number, DTN endpoint, etc.)
is required to communicate with that peer. Note that the `SbnPeerData.dat`
file should be the same for all peers, and that the file should include
at least one entry for the local system (sometimes this is referred to as
the "host" entry.)

### SBN Platform Configuration

The `sbn_platform_cfg.h` file contains a number of definitions that control
how SBN allocates and limits resources as well as controlling the behavior
of SBN. Most "max" definitions relate to in-memory static arrays, so increasing
the value increases the memory footprint of SBN (in some cases, non-linearly.)

Name|Default|Description
---|---|---
`SBN_MAX_NETS`|16|Maximum number of networks allowed.
`SBN_MAX_PEERS_PER_NET`|32|Maximum number of peers per network allowed.
`SBN_MAX_SUBS_PER_PEER`|256|Maximum number of subscriptions allowed per peer allowed.
`SBN_MAX_PEERNAME_LENGTH`|32|Maximum length of the name field in the `SbnPeerData.dat` file.
`SBN_MAX_NET_NAME_LENGTH`|16|Maximum length of the network field in the `SbnPeerData.dat` file.
`SBN_MAX_MSG_PER_WAKEUP`|32|In the polling configuration, the maximum number of messages we'll process from a peer. (To prevent starvation by a babbling peer.)
`SBN_MAIN_LOOP_DELAY`|200|In the polling configuration, how long (in milliseconds) to wait for a SCH wakeup message before SBN times out and processes. (Note, should really be significantly longer than the expected time between SCH wakeup messages.)
`SBN_POLL_TIME`|5|If I haven't sent a message to a peer in this amount of time (in seconds), call the poll function of the API in case it needs to perform some connection maintenance.
`SBN_PEER_PIPE_DEPTH`|64|For each peer, a pipe is created to receive messages that the peer has subscribed to. The pipe should be deep enough to handle all messages that will queue between wakeups.
`SBN_DEFAULT_MSG_LIM`|8|The maximum number of messages that will be queued for a particular message ID for a particular peer.
`SBN_SUB_PIPE_DEPTH`|256|The maximum number of subscription messages that will be queued between wakeups.
`SBN_MAX_ONESUB_PKTS_ON_PIPE`|256|The maximum number of subscription messages for a single message ID that will be queued between wakeups. (These are received when updates occur after SBN starts up.)
`SBN_MAX_ALLSUBS_PKTS_ON_PIPE`|256|The maximum number of subscription messages for all message IDs that will be queued between wakeups. (These are received on SBN startup.)
`SBN_VOL_PEER_FILENAME`|"/ram/SbnPeerData.dat"|The volatile memory location for the peer data file.
`SBN_NONVOL_PEER_FILENAME`|"/cf/SbnPeerData.dat"|The non-volatile memory location for the peer data file.
`SBN_PEER_FILE_LINE_SIZE`|128|The maximum length of a line of configuration data in the peer data file.
`SBN_VOL_MODULE_FILENAME`|"/ram/SbnModuleData.dat"|The volatile memory location for the module data file.
`SBN_NONVOL_MODULE_FILENAME`|"/cf/SbnModuleData.dat"|The non-volatile memory location for the module data file.
`SBN_MODULE_FILE_LINE_SIZE`|128|The maximum length of a line of configuration data in the module data file.
`SBN_MAX_INTERFACE_TYPES`|8|Maximum number of protocol modules.
`SBN_MOD_STATUS_MSG_SIZE`|128|SBN modules can provide status messages for housekeeping requests, this is the maximum length those messages can be.
`SBN_TASK_SEND`|(undef)|If defined, a task is created for each peer and that task pends on the data pipe for that peer, sending messages to the peer as soon as they are received on the pipe. Otherwise, peer pipes are read when SCH wakes up SBN.
`SBN_TASK_RECV`|(undef)|If defined, a task is created for each peer and that task pends on the network connection for that peer, receiving messages from that peer and publishing on the local bus as soon as they are received. Otherwise, peer network connections are read when SCH wakes up SBN.

### SBN Remapping Table

The SBN remapping table is a standard cFS table defining, on a peer-by-peer
basis, which message ID's should be remapped to other ID's, or which
message ID's should be filtered (where the "To" field is 0).

### SbnModuleData.dat File Format

The `SbnModuleData.dat` file is comprised of a number of rows, one per protocol
module, with four columns:

- Protocol ID - The ID used to in the SbnPeerData.dat file to identify which
  module to use to communicate to that peer.
- Module Name - The name of the module (used in debug/error CFE events.)
- Module Shared Library Filename - The path/file name used to load the
  shared library of the module.
- Module interface symbol - The name of a global symbol that is of the type
  `SBN_IfOps_t`.

Fields are separated by commas (`,`) and lines are terminated with a semicolon
(`;`). The first line starting with an exclamation mark (`!`) is considered the
end of the file and all further text in the file is ignored (use with caution!)

For example:

    1, UDP, /cf/sbn_udp.so, SBN_UDP_Ops;
    2, TCP, /cf/sbn_tcp.so, SBN_TCP_Ops;
    8, DTN, /cf/sbn_dtn.so, SBN_DTN_Ops;
    ! Module ID, Module Name, Shared Lib, Interface Symbol

### SbnPeerData.dat File Format

Once SBN has loaded modules via the `SbnModuleData.dat` file, it then loads the
peer configuration file. Peers are grouped into "networks", all peers in the
same network share messages equally (there is no routing or bridging between
networks, but there can be filtering and remapping of message ID's between
peers.) Networks are particularly important to identify the interface specifics
of the local system (the "host" interface), particularly in connection-less
(UDP and DTN) protocols.

The fields in the peer data file are:
- CPU Name - for debug/information event message text.
- CPU ID - must match the return value of `CFE_PSP_GetProcessorId()` for the
  system (normally `CFE_CPU_ID`).
- Protocol ID - to determine which module to use to communicate with this
  peer.
- Spacecraft ID - Must match the spacecraft ID for the peer/host.
- QoS - The QoS bitfield (currently unused.)
- Network Name - For grouping peers and host interfaces.
- *network-specific fields* - All remaining fields are passed to the protocol
  module, specifying how to address the host/peer.

For example:

    CPU1, 1, 8, 42, 0, SBN0, ipn:1.1;
    CPU2, 2, 8, 42, 0, SBN0, ipn:1.2;
    CPU3, 3, 8, 42, 0, SBN0, ipn:1.3;

    CPU4, 1, 1, 0xCC, 0, SBN1, 127.0.0.1, 15820;
    CPU5, 2, 1, 0xCC, 0, SBN1, 127.0.0.1, 15821;
    CPU6, 3, 1, 0xCC, 0, SBN1, 127.0.0.1, 15822;

SBN Interactions With Other Apps and the Software Bus (SB)
----------------------------------------------------------

SBN treats all nodes as peers and (by default) all subscriptions of local
applications should receive messages sent by publishers on other peers, and
all messages published on the local bus should be transmitted to any peers
who have applications subscribed to that message ID.

The Software Bus (SB), when an application subscribes to a message ID or
unsubscribes from a message ID, sends a message that SBN receives. Upon
receipt of these messages, SBN updates its internal state tables and sends
a message to the peers with the information on the update.

SBN Scheduling and Tasks
------------------------

SBN has two modes of operation (configured at compile time):

- A traditional scheduler (SCH)-driven mode where SCH sends a wakeup message
  periodically and SBN polls pipes and network modules. SBN has a built-in
  timeout period so that if SCH is somehow not functioning properly, or is
  mis-configured, SBN will continue to function. This mode is preferable
  in environments where resources are constrained, where network traffic
  load is well understood, and deterministic behavior is expected.

- A per-peer task model where the local SBN instance creates two tasks for
  every peer--one task blocks on reading the local pipe (waiting for messages
  to send to the respective peer) and the other task blocks on the network
  module's "receive" function, waiting for messages from the peer to send
  to the local bus. This model is preferred in environments where network
  traffic load may vary significantly (posing a risk of overloading pipes),
  and where determinism is not expected and resources are not particularly
  constrained.

Technically, the choice of task or SCH-driven processing is set for
each direction ("sending" local bus messages to the peer and "receiving"
messages from the peer to put on the local bus.) However, it's generally
best to stick with either SCH-driven processing or task-driven processing.

SBN Protocol Modules
--------------------

SBN requires the use of protocol modules--shared libraries that provide a
defined set of functions to send and receive encapsulated software bus messages
and subscription updates. Protocol modules may use connection-less (UDP)
or connection-based (TCP) network technologies and network reliability and
connection maintenance is expected to be provided by the module or the
network technology it is using. SBN does benefit from knowing when a peer
is "connected" so that the local subscriptions can be sent (in bulk) to
that peer; otherwise SBN does not need to know the state of the network
the module is communicating with.

SBN modules are built as separate cFS "applications" but are not loaded via the
Executive Service (ES) interface, instead the module is loaded by the SBN
application (as defined by a start-time configuration file.)

Currently SBN provides the following modules:
- UDP - Utilizing the UDP/IP connectionless protocol, the UDP module uses
  "announce" and "heartbeat" internal messages to determine when a peer has
  connected to the network (and that the subscriptions need to be sent.)
  Otherwise no network reliability is provided by the UDP module, packets
  may be lost or jumbled without the knowledge of SBN.

- TCP - The TCP module utilizes the Internet-standard, high reliability TCP
  protocol, which provides for error correction and connection management.

- DTN - Integrating the ION-DTN 3.6.0 libraries, the DTN module provides
  high reliability, multi-path transmission, and queueing. Effectively,
  DTN peers are always connected.
