#/**********************************************************\ 
# Auto-generated Mac project definition file for the
# DepthJS Plugin project
#\**********************************************************/

# Mac template platform definition CMake file
# Included from ../CMakeLists.txt

# remember that the current source dir is the project root; this file is in Mac/
file (GLOB PLATFORM RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    Mac/[^.]*.cpp
    Mac/[^.]*.h
    Mac/[^.]*.cmake
	Mac/[^.]*.mm
    )

# use this to add preprocessor definitions
add_definitions(
    
)

find_library(OPENGL_FRAMEWORK OpenGL)
find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)
find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)

include_directories(${CORE_FOUNDATION_FRAMEWORK})
 
set(OPENNI_XML_FILE "Sample-Tracking.xml")
set_source_files_properties(
	${OPENNI_XML_FILE}
	PROPERTIES
	MACOSX_PACKAGE_LOCATION "Resources"
)

SOURCE_GROUP(Mac FILES ${PLATFORM})

set (SOURCES
    ${SOURCES}
    ${PLATFORM}
	${OPENNI_XML_FILE}
    )

set(PLIST "Mac/bundle_template/Info.plist")
set(STRINGS "Mac/bundle_template/InfoPlist.strings")
set(LOCALIZED "Mac/bundle_template/Localized.r")

add_mac_plugin(${PROJECT_NAME} ${PLIST} ${STRINGS} ${LOCALIZED} SOURCES)

add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND cp ../chrome-extension/manifest.json.MACOSX ../chrome-extension/manifest.json
	COMMAND cp -R ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/depthjsplugin.plugin ../chrome-extension/plugin/
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)

# add library dependencies here; leave ${PLUGIN_INTERNAL_DEPS} there unless you know what you're doing!
target_link_libraries(${PROJECT_NAME}
    ${PLUGIN_INTERNAL_DEPS}
	${OPENGL_FRAMEWORK}
	${QUARTZ_CORE_FRAMEWORK}
	${CORE_FOUNDATION_FRAMEWORK}
	${OpenNI_LIBS}
	${NITE_LIBS}
    )
