set(TARGET_NAME sp-cortex)

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

find_package(OpenCV REQUIRED)

target_link_libraries(${TARGET_NAME}
	PUBLIC
	# static
	# dynamic
	    sp-common
		stdc++fs
		zbar
		pthread
		${OpenCV_LIBS}
)

target_include_directories(${TARGET_NAME}
	PRIVATE
	    ${PROJECT_SOURCE_DIR}/../driver
		${OpenCV_INCLUDE_DIRS}
)

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION bin)
