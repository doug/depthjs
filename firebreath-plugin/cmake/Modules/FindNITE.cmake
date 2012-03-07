# - Try to find NITE
# Once done, this will define
#
#  NITE_FOUND - system has NITE
#  NITE_INCLUDE_DIRS - the NITE include directories
#  NITE_LIBRARIES - link these to use NITE 

#include(LibFindMacros)

# Dependencies

# Use pkg-config to get hints about paths
find_package(PkgConfig)
pkg_check_modules(NITE_PKGCONF NITE)

SET(NITE_INCLUDE_SEARCH_PATHS ${NITE_PKGCONF_INCLUDE_DIRS})
SET(NITE_LIB_SEARCH_PATHS ${NITE_PKGCONF_LIBRARY_DIRS})

IF(WIN32) 
	# this is where NITE msi installers usually put the files
	SET(NITE_INCLUDE_SEARCH_PATHS "C:/Program Files/NITE/Include/" "D:/Program Files/NITE/Include/")
	SET(NITE_LIB_SEARCH_PATHS "C:/Program Files/NITE/Lib/" "D:/Program Files/NITE/Lib/")
ELSE(WIN32)
	SET(NITE_INCLUDE_SEARCH_PATHS ${NITE_INCLUDE_SEARCH_PATHS} "/usr/local/include" "/usr/include")
	set(NITE_LIB_SEARCH_PATHS "${NITE_LIB_SEARCH_PATHS}" "/usr/local/lib" "/usr/lib")
ENDIF(WIN32)

MESSAGE(STATUS "Try to look here: ${NITE_INCLUDE_SEARCH_PATHS}")

# Include dir
find_path(NITE_INCLUDE_DIR
  NAMES XnVNite.h
  HINTS ${NITE_INCLUDE_SEARCH_PATHS} 
  PATH_SUFFIXES "nite"
)

if(NITE_INCLUDE_DIR STREQUAL "NITE_INCLUDE_DIR-NOTFOUND")
	message(STATUS "Looking for NITE in default dirs")
	find_path(NITE_INCLUDE_DIR NAMES XnVNite.h 
	  PATH_SUFFIXES "nite"
	)
endif()

# Finally the library itself
find_library(NITE_LIBRARY
  NAMES XnVNITE
  PATHS ${NITE_LIB_SEARCH_PATHS}
)

if(NITE_LIBRARY STREQUAL "NITE_LIBRARY-NOTFOUND")
	message(STATUS "can't find NITE library, looking more aggressively")
	foreach(NITE_LOOKUP_PATH ${NITE_LIB_SEARCH_PATHS})
		file(GLOB NITE_LIBRARY "${NITE_LOOKUP_PATH}/*XnV*")
		message(STATUS "looking in ${NITE_LOOKUP_PATH} resulted in ${NITE_LIBRARY}")
		if(NITE_LIBRARY)
#			#found NITE library
#			#but there may be many files found..
			if(WIN32)
				foreach(NITE_LIBRARY_FOUND ${NITE_LIBRARY})
					message(STATUS "checking ${NITE_LIBRARY_FOUND}")
					string(REGEX MATCH "lib" FOUND_LIB_IN_FILE ${NITE_LIBRARY_FOUND})
					if(FOUND_LIB_IN_FILE)
						set(NITE_LIBRARY_TMP ${NITE_LIBRARY_TMP} ${NITE_LIBRARY_FOUND})
					endif()
				endforeach()
			set(NITE_LIBRARY ${NITE_LIBRARY_TMP})
			endif(WIN32)
			break()
		endif()
	endforeach()
endif()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
#set(NITE_PROCESS_INCLUDES NITE_INCLUDE_DIR NITE_INCLUDE_DIRS)
#set(NITE_PROCESS_LIBS NITE_LIBRARY NITE_LIBRARIES)
#libfind_process(NITE)

set(NITE_LIBRARIES ${NITE_LIBRARY} )
set(NITE_LIBS ${NITE_LIBRARY} )
set(NITE_INCLUDE_DIRS ${NITE_INCLUDE_DIR} )
set(NITE_DIR "${NITE_INCLUDE_DIR}/../" )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ???_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(NITE  DEFAULT_MSG
                                  NITE_LIBRARY NITE_INCLUDE_DIR)

mark_as_advanced(NITE_INCLUDE_DIR NITE_LIBRARY )
