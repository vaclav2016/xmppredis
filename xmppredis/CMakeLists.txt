cmake_minimum_required (VERSION 2.8.12)

ADD_EXECUTABLE(xmppredis xmppredis.c)
TARGET_LINK_LIBRARIES(xmppredis ini strophe hiredis ${CMAKE_THREAD_LIBS_INIT})
