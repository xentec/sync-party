cmake_minimum_required(VERSION 2.8)
find_package( OpenCV 4 REQUIRED )

MESSAGE(STATUS "${OpenCV_INCLUDE_DIRS}")
MESSAGE(STATUS "${OpenCV_LIBS}")
MESSAGE(STATUS "${OpenCV_VERSION}")


project(camera_opencv)
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable(${PROJECT_NAME} "main.cpp" "camera_opencv.cpp" "camera_opencv.h")
target_link_libraries( camera_opencv zbar pthread ${OpenCV_LIBS} )

