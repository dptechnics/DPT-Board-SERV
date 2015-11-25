# Copyright (c) 2014, Daan Pape
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 
#     1. Redistributions of source code must retain the above copyright 
#        notice, this list of conditions and the following disclaimer.
#
#     2. Redistributions in binary form must reproduce the above copyright 
#        notice, this list of conditions and the following disclaimer in the 
#        documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 2.6)

PROJECT(dpt-breakout-server C)

INCLUDE (CheckFunctionExists)
INCLUDE(FindPkgConfig)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
ADD_DEFINITIONS(-Os -Wall -Werror -Wmissing-declarations --std=gnu99)

SET(SOURCES 
    main.c 
    listen.c 
    client.c 
    config.c
    utils.c 
    file.c
    api.c 
    logger.c
    filedownload.c 
    helper.c
    longrunner.c

    firmware/firmware.c 
    firmware/firmware_dao.c 
    firmware/firmware_json_api.c 

    tempsensor/tempsensor.c 
    tempsensor/tempsensor_json_api.c 

    gpio/gpio.c
    gpio/gpio_dao.c
    gpio/gpio_json_api.c

    combus/spi.c

    display/screen_ssd1306.c

    kunio/kunio.c
    kunio/kunio_json_api.c

    network/network.c

    wifi/nl.c
    wifi/wifi.c
    wifi/wifi_json_api.c
    wifi/wifi_longrunner.c

    system/system.c
    system/system_json_api.c

    bluecherry/bluecherry.c
    bluecherry/bluecherry_json_api.c

    database/database.c
    database/db_keyvalue.c

    pwm/pwm.c
)

CHECK_FUNCTION_EXISTS(getspnam HAVE_SHADOW)
IF(HAVE_SHADOW)
    ADD_DEFINITIONS(-DHAVE_SHADOW)
ENDIF()

MESSAGE(STATUS "Configuring libnl ...")

PKG_SEARCH_MODULE(LIBNL-TINY libnl-tiny)
IF(LIBNL-TINY_FOUND)
	INCLUDE_DIRECTORIES(${LIBNL-TINY_INCLUDE_DIRS})
	LINK_DIRECTORIES(${LIBNL-TINY_LIBRARY_DIRS})
	SET(LIBNL_LIBRARIES ${LIBNL-TINY_LIBRARIES})
ELSE(LIBNL-TINY_FOUND)
	PKG_SEARCH_MODULE(LIBNL libnl>=2.0 libnl-2.0 libnl-2)
	IF(LIBNL_FOUND)
		INCLUDE_DIRECTORIES(${LIBNL_INCLUDE_DIRS})
		LINK_DIRECTORIES(${LIBNL_LIB_DIRS})
		LIST(APPEND LIBNL_LIBRARIES
			nl-genl
		)
	ELSE(LIBNL_FOUND)
		PKG_SEARCH_MODULE(LIBNL3 libnl-3 libnl-3.0 libnl-3.1)
		IF(LIBNL3_FOUND)
			INCLUDE_DIRECTORIES(${LIBNL3_INCLUDE_DIRS})
			LINK_DIRECTORIES(${LIBNL3_LIB_DIRS})
			SET(LIBNL_LIBRARIES ${LIBNL3_LIBRARIES})
			MESSAGE(STATUS "")
			MESSAGE(STATUS "Configuring libnl-genl ...")

			PKG_SEARCH_MODULE(LIBNL_GENL libnl-genl>=2.0 libnl-genl-3 libnl-genl-3.0 libnl-genl-3.1)
			IF(LIBNL_GENL_FOUND)
				INCLUDE_DIRECTORIES(${LIBNL_GENL_INCLUDE_DIRS})
				LINK_DIRECTORIES(${LIBNL_GENL_LIB_DIRS})
			ENDIF(LIBNL_GENL_FOUND)
		ELSE(LIBNL3_FOUND)
			MESSAGE(FATAL_ERROR "Error: libnl and libnl-genl not found")
		ENDIF(LIBNL3_FOUND)
	ENDIF(LIBNL_FOUND)
ENDIF(LIBNL-TINY_FOUND)

ADD_EXECUTABLE(dpt-breakout-server ${SOURCES})
FIND_LIBRARY(libjson NAMES json-c json)
FIND_LIBRARY(iwinfo NAMES libiwinfo iwinfo)
FIND_LIBRARY(libsqlite3 NAMES sqlite3 libsqlite3)
FIND_LIBRARY(uci NAMES uci libuci)
FIND_LIBRARY(libubus NAMES ubus libubus)
FIND_LIBRARY(libblobmsg_json NAMES libblobmsg-json blobmsg_json)
FIND_LIBRARY(libcurl NAMES curl libcurl)
FIND_LIBRARY(libpthread NAMES pthread libpthread)
FIND_LIBRARY(libnl-tiny NAMES nl-tiny libnl-tiny)
TARGET_LINK_LIBRARIES(dpt-breakout-server ubox dl ${libjson} ${libsqlite3} ${iwinfo} ${uci} ${libubus} ${libblobmsg_json} ${libcurl} ${libpthread} ${libnl-tiny} ${LIBS})

INSTALL(TARGETS dpt-breakout-server ${PLUGINS}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)
