#!/bin/bash
set -e

# The */ ensures it matches the folder name at the very end of the path
if [[ ! "$PWD" == */talker_listener ]]; then
    echo "Due to scoping issues, please only run this in the apps/sbn/talker_listener directory"
    exit 1
fi

cFS_root=$(realpath ../../..)

if [[ -d $cFS_root/build-native_std ]]
then
    build_dir=$cFS_root/build-native_std
elif [[ -d $cFS_root/build-native_eds ]]
then
    build_dir=$cFS_root/build-native_eds
else
    echo "You need to build the FSW first"
    exit 1
fi

echo "==============================================="
echo "Enabling the use of listener_app and talker_app"
echo "==============================================="
sleep 2

echo "> Copying the listener and talker applications into apps"
sleep 2

cd $cFS_root/apps/sbn/talker_listener/apps
cp -r listener_app $cFS_root/apps
cp -r talker_app   $cFS_root/apps

# ==================================================

echo "> Removing troublesome table files"
sleep 2

cd $cFS_root/sample_defs/tables
rm sample_app_*.c
rm to_lab_sub_*.c

# ==================================================

# Navigate to common directory for next tasks
cd $cFS_root/apps/sbn/talker_listener/sample_defs

# ==================================================

echo "> Forcing cpus to start specific applications"
sleep 2

cp cpu1/cfe_es_startup.scr $build_dir/exe/cpu1/cf/cfe_es_startup.scr
cp cpu2/cfe_es_startup.scr $build_dir/exe/cpu2/cf/cfe_es_startup.scr

# ==================================================

echo "> Copying modified tables to sample_defs"
sleep 2

cp tables/sch_lab_table.c $cFS_root/sample_defs/tables/sch_lab_table.c
cp tables/to_lab_sub.c    $cFS_root/sample_defs/tables/to_lab_sub.c

# ==================================================

echo "> Copying up custom install CMake files"
sleep 2

cp cpu1/install_custom.cmake $cFS_root/sample_defs/cpu1/install_custom.cmake
cp cpu2/install_custom.cmake $cFS_root/sample_defs/cpu2/install_custom.cmake

# ==================================================

echo "> Adjust target.cmake for listener app, talker app"
sleep 2

cp targets.cmake $cFS_root/sample_defs/targets.cmake

# ==================================================

echo  "> Update remap table to translate talker_app and listener_app messages across instances"
sleep 2

cp tables/sbn_remap_tbl.c $cFS_root/sample_defs/tables/sbn_remap_tbl.c