cmake_minimum_required (VERSION 2.8.12)

PROJECT(vkredis C)

find_package (Threads REQUIRED)
find_package (Hiredis REQUIRED)
find_package (CURL REQUIRED)
find_package (cjson REQUIRED)
find_package (Ini REQUIRED)

ADD_EXECUTABLE(vkredis vkredis.c curl-client.c curl-client.h vk.c vk.h)
TARGET_LINK_LIBRARIES(vkredis m cjson ini curl hiredis ${CMAKE_THREAD_LIBS_INIT})
