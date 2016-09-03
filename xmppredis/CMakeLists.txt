cmake_minimum_required (VERSION 2.8.12)

PROJECT(xmppredis C)

set(GCC_CXX_FLAGS ${GCC_CXX_FLAGS} "-static-libgcc -static")

find_package (Threads REQUIRED)
find_package (Hiredis REQUIRED)
find_package (Ini REQUIRED)

INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE(strophe.h HAVE_STROPHE)

ADD_EXECUTABLE(xmppredis xmppredis.c)
TARGET_LINK_LIBRARIES(xmppredis strophe hiredis ini ${CMAKE_THREAD_LIBS_INIT})

install (TARGETS xmppredis DESTINATION bin)
