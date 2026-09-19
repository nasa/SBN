# Software Bus Network
NASA Core Flight System (cFS) Software Bus Network (SBN) Application

## Description
The SBN is a cFS application that is a plug in to the Core Flight Executive (cFE) component of the cFS.

The cFS is a platform and project independent reusable software framework and set of reusable applications developed by NASA Goddard Space Flight Center. This framework is used as the basis for the flight software for satellite data systems and instruments, but can be used on other embedded systems. More information on the cFS can be found at [http://cfs.gsfc.nasa.gov](http://cfs.gsfc.nasa.gov)

The SBN application connects the cFE Software Bus (SB) to other buses, bridging the publish/subscribe messaging service to separate cFS instances in separate partitions, processes, processors, and/or networks.

## License
This software is licensed under the [Apache 2.0 License](LICENSE).

## Building the default configuration

Selecting `sbn` in the cFE target application list also builds and installs the UDP
and remap modules required by the bundled configuration table, including the remap
table. Modules explicitly listed by the mission are still built through their normal
cFE targets. Missions supplying a different SBN configuration can set
`SBN_BUILD_DEFAULT_MODULES=OFF` and select their required modules explicitly.
