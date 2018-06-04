cmake_minimum_required(VERSION 2.8)
find_package( OpenCV 3 REQUIRED )
find_package( PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

MESSAGE(STATUS "${OpenCV_INCLUDE_DIRS}")
MESSAGE(STATUS "${OpenCV_LIBS}")
MESSAGE(STATUS "${OpenCV_VERSION}")


project(camera_opencv)
include_directories(${GTK3_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} )
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})
add_executable(${PROJECT_NAME} "main.cpp" "camera_opencv.cpp" "camera_opencv.h")
target_link_libraries( camera_opencv zbar pthread ${OpenCV_LIBS} ${GTK3_LIBRARIES})
