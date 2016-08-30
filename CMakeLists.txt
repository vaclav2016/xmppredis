cmake_minimum_required (VERSION 2.8.12)
PROJECT(xmppredis C)

find_package (Threads REQUIRED)
find_package (Hiredis REQUIRED)

INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE(strophe.h HAVE_STROPHE)

ADD_EXECUTABLE(xmppredis xmppredis.c ini.c)
TARGET_LINK_LIBRARIES(xmppredis strophe hiredis ${CMAKE_THREAD_LIBS_INIT})
