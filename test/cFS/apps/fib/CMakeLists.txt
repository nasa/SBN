cmake_minimum_required(VERSION 2.6.4)
project(CFE_FIB C)

include_directories(fsw/platform_inc)

aux_source_directory(fsw/src APP_SRC_FILES)

add_cfe_app(fib_app ${APP_SRC_FILES})
