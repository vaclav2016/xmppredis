cmake_minimum_required (VERSION 2.8.12)
PROJECT(xmppredis C)

find_package (Threads REQUIRED)
find_package (Hiredis REQUIRED)

INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE(strophe.h HAVE_STROPHE)

include_directories ("${PROJECT_SOURCE_DIR}/ini")

add_subdirectory(ini)
add_subdirectory(xmppredis)
