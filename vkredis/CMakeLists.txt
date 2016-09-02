cmake_minimum_required (VERSION 2.8.12)

PROJECT(vkredis C)

find_package (Threads REQUIRED)
find_package (Hiredis REQUIRED)
find_package (CURL REQUIRED)

ADD_EXECUTABLE(vkredis vkredis.c ini.c ini.h json.c json.h curl-client.c curl-client.h)
TARGET_LINK_LIBRARIES(vkredis m curl hiredis ${CMAKE_THREAD_LIBS_INIT})
