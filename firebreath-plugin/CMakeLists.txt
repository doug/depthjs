#/**********************************************************\ 
# 
# Auto-generated CMakeLists.txt for the DepthJS Plugin project
#
#\**********************************************************/

# Written to work with cmake 2.6
cmake_minimum_required (VERSION 2.6)
set (CMAKE_BACKWARDS_COMPATIBILITY 2.6)

Project(${PLUGIN_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

file (GLOB GENERAL RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    [^.]*.cpp
    [^.]*.h
	[^.]*.hpp
    [^.]*.cmake
	${CMAKE_MODULE_PATH}/*
    )

include_directories(${PLUGIN_INCLUDE_DIRS})

# Generated files are stored in ${GENERATED} by the project configuration
SET_SOURCE_FILES_PROPERTIES(
    ${GENERATED}
    PROPERTIES
        GENERATED 1
    )

SOURCE_GROUP(Generated FILES
    ${GENERATED}
    )

SET( SOURCES
    ${GENERAL}
    ${GENERATED}
    )

find_package(OpenNI)
if(NOT OPENNI_FOUND)
	message(STATUS "Can't find OpenNI!")
else()
	message(STATUS "OpenNI found in ${OpenNI_INCLUDE_DIRS}")
endif()
include_directories(${OpenNI_INCLUDE_DIRS})


find_package(NITE)
if(NOT NITE_FOUND)
	message(STATUS "Can't find NITE!")
else()
	message(STATUS "NITE found in ${NITE_INCLUDE_DIRS}")
endif()
include_directories(${NITE_INCLUDE_DIRS})


# This will include Win/projectDef.cmake, X11/projectDef.cmake, Mac/projectDef 
# depending on the platform
include_platform()
