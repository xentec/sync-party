cmake_minimum_required(VERSION 3.2)

project(sp-cortex CXX)

# include sources
set(SOURCES
	main.cpp

	adjust.hpp
	adjust.cpp
	driver.hpp
	driver.cpp
	pwm.hpp
	pwm.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	INTERPROCEDURAL_OPTIMIZATION $<IF:$<CONFIG:Debug>,OFF,ON>
)
target_compile_features(${PROJECT_NAME} PUBLIC "cxx_std_17") # C++17
target_compile_options(${PROJECT_NAME} PUBLIC "-Wall")       # include all warnings

add_subdirectory(${PROJECT_SOURCE_DIR}/common)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
	# static
	# dynamic
		sp-common
		stdc++fs
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
