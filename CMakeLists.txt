cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(sync-party)

# Options
#nothing to see here yet
option(SP_BUILD_DOCS "Build LATEX documents" OFF)

# includes
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Subprojects
add_subdirectory(src)
if(${SP_BUILD_DOCS})
	add_subdirectory(docs)
endif()
