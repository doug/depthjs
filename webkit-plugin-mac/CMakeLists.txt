cmake_minimum_required (VERSION 2.6)
set (CMAKE_BACKWARDS_COMPATIBILITY 2.6)

Project(depthjs-webkit-plugin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/" "${CMAKE_CURRENT_SOURCE_DIR}/../firebreath-plugin/cmake/Modules/")

find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
find_library(WEBKIT_FRAMEWORK WebKit REQUIRED)
find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
find_library(LIBZ libz REQUIRED)
find_library(ACCELERATE_FRAMEWORK Accelerate REQUIRED)
find_library(JAVASCRIPTCORE_FRAMEWORK JavaScriptCore REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)

find_package(OpenNI REQUIRED)
if(NOT OPENNI_FOUND)
	message(STATUS "Can't find OpenNI!")
else()
	message(STATUS "OpenNI found in ${OpenNI_INCLUDE_DIRS}")
endif()
include_directories(${OpenNI_INCLUDE_DIRS})


find_package(NITE REQUIRED)
if(NOT NITE_FOUND)
	message(STATUS "Can't find NITE!")
else()
	message(STATUS "NITE found in ${NITE_INCLUDE_DIRS}")
endif()
include_directories(${NITE_INCLUDE_DIRS})


file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	[^.]*.cpp 
	[^.]*.c 
	[^.]*.h 
	[^.]*.hpp
	[^.]*.m
	[^.]*.mm
	[^.]*.pch
)

add_library(depthjs-webkit-plugin MODULE 
	${SOURCES}
	)


set_target_properties(depthjs-webkit-plugin
	PROPERTIES 
	MACOSX_BUNDLE TRUE
	MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
	BUNDLE TRUE
	BUNDLE_EXTENSION "webplugin"
	)

target_link_libraries(depthjs-webkit-plugin
	${COCOA_FRAMEWORK}
	${WEBKIT_FRAMEWORK}
	${IOKIT_FRAMEWORK}
	${LIBZ_LIBS} ${LIBZ_LIBRARIES}
	${ACCELERATE_FRAMEWORK}
	${JAVASCRIPTCORE_FRAMEWORK}
	${FOUNDATION_FRAMEWORK}
	${OpenNI_LIBS}
	${NITE_LIBS}
	)
