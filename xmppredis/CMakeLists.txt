cmake_minimum_required (VERSION 2.8.12)

PROJECT(xmppredis C)

set(GCC_CXX_FLAGS ${GCC_CXX_FLAGS} "-static-libgcc -static")

find_package (Threads REQUIRED)
find_package (Hiredis REQUIRED)
# find_package (Ini REQUIRED)

INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE(strophe.h HAVE_STROPHE)

ADD_EXECUTABLE(xmppredis xmppredis.c ini.c ini.h logger.h logger.c)
TARGET_LINK_LIBRARIES(xmppredis strophe hiredis ${CMAKE_THREAD_LIBS_INIT})

install (TARGETS xmppredis DESTINATION bin)
install (FILES xmppredis.sample.conf DESTINATION /etc )
install (FILES xmppredis.sample.conf DESTINATION /usr/share/doc/xmppredis)
install (FILES LICENSE DESTINATION /usr/share/doc/xmppredis)

IF(WIN32)
	set(CPACK_GENERATOR "NSIS;ZIP")
else(WIN32)
	set(CPACK_GENERATOR "DEB")
endif(WIN32)

SET(CPACK_DEB_COMPONENT_INSTALL 1)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_NAME "xmppredis")
set(CPACK_PACKAGE_VENDOR "vaclav2016")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XMPP to REDIS bridge")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)

set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/vaclav2016/xmppredis/")
SET(CPACK_PACKAGE_CONTACT "jabber id is vaclav2016@jabber.cz")
set(CPACK_DEBIAN_PACKAGE_SECTION "Communication")

include(CPack)

