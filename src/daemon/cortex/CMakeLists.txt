set(TARGET_NAME sp-cortex)

message("CMAKE_CXX_COMPILE_FEATURES = ${CMAKE_CXX_COMPILE_FEATURES}")

find_package( OpenCV 4 REQUIRED )
find_package( PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK2 REQUIRED gtk+-2.0)

add_executable(${TARGET_NAME}
	main.cpp

	adjust.hpp
	adjust.cpp
	driver.hpp
	driver.cpp
	pwm.hpp
	pwm.cpp
	camera_opencv.cpp
	camera_opencv.h
)

set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
	INTERPROCEDURAL_OPTIMIZATION $<IF:$<CONFIG:Debug>,OFF,ON>
)
target_compile_options(${TARGET_NAME} PUBLIC "-Wall")       # include all warnings

target_link_libraries(${TARGET_NAME}
	PUBLIC
	# static
	# dynamic
		sp-common
		stdc++fs
		zbar
		pthread
		${OpenCV_LIBS} 
		${GTK3_LIBRARIES}
)

target_include_directories(${TARGET_NAME}
	PRIVATE
<<<<<<< HEAD
	    ${PROJECT_SOURCE_DIR}/../driver
	    ${GTK3_INCLUDE_DIRS} 
	    ${OpenCV_INCLUDE_DIRS} 
=======
		${PROJECT_SOURCE_DIR}/../driver
>>>>>>> master
)

ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION bin)
