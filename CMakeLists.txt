cmake_minimum_required(VERSION 2.8.7)
project(Amdgpu-Fan)

if (NOT CMAKE_BUILD_TYPE)
	message("Setting build type to 'RelWithDebInfo'")
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

OPTION(BUILD_DBUS_SUPPORT "Build DBUS listener." OFF)
OPTION(BUILD_CLIENT "Build CLI." OFF)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	SET(CMAKE_INSTALL_PREFIX
		"${CMAKE_CURRENT_SOURCE_DIR}/dist" CACHE PATH "Installation prefix, default 'dist'" FORCE
		)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

unset(DBUS_AVAILABLE CACHE)

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#include_directories(.)
#include_directories(src)

add_executable(amdgpufand src/amdgpufand.c)

IF(BUILD_DBUS_SUPPORT)
	message( FATAL_ERROR "DBUS is not implemented yet" )
	find_package(DBus REQUIRED)
	add_definitions(-DDBUS_FOUND)
	include_directories(${DBUS_INCLUDE_DIRS})
	add_library(libamdgpufan)
	target_link_libraries(libamdgpufan ${DBUS_LIBRARIES})
	target_link_libraries(amdgpufand ${DBUS_LIBRARIES})
	IF(BUILD_CLIENT)
		add_executable(amdgpufan src/amdgpufan.c)
		target_link_libraries(amdgpufan libamdgpufan)
	ENDIF(BUILD_CLIENT)
ELSE(BUILD_DBUS_SUPPORT)
	IF(BUILD_CLIENT)
		message( FATAL_ERROR "DBUS is required for client" )
	ENDIF(BUILD_CLIENT)
ENDIF(BUILD_DBUS_SUPPORT)

IF(BUILD_DBUS_SUPPORT)
	INSTALL(DIRECTORY src/ DESTINATION include/amdgpufan FILES_MATCHING PATTERN *.h)
ENDIF(BUILD_DBUS_SUPPORT)

#INSTALL(TARGETS Amdgpu-Fan amdgpufan
#	RUNTIME DESTINATION bin
#	ARCHIVE DESTINATION lib
#	LIBRARY DESTINATION lib)

#INSTALL(TARGETS amdgpufand
#	RUNTIME DESTINATION sbin)





