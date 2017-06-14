Core Flight Software (cFS) Software Bus Networking (SBN)
========================================================

Version History
---------------
This document details the design and use of the SBN application. This is current
as of SBN version 1.6.

This document is maintained by Christopher.D.Knight@nasa.gov .

Overview
--------
SBN is a cFS application that connects the local software bus to one or more
other cFS nodes (who are also running SBN) such that all messages sent by an
application on one bus will be received by an application on another bus. SBN
has a modular network architecture (TCP, UDP, Serial, SpaceWire, etc.) to
connect peers and supports mixed-mode peer networks. SBN also remaps and
filters messages (cFS table-configured.)

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
