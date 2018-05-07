cmake_minimum_required(VERSION 3.2)

project(sp-cortex CXX)

# include sources
set(SOURCES
	asio.hpp
	controller.hpp
	controller.cpp
	driver.hpp
	driver.cpp
	main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	INTERPROCEDURAL_OPTIMIZATION $<IF:$<CONFIG:Debug>,OFF,ON>
)
target_compile_features(${PROJECT_NAME} PUBLIC "cxx_std_17") # C++17
target_compile_options(${PROJECT_NAME} PUBLIC "-Wall")       # include all warnings

target_compile_definitions(${PROJECT_NAME} PUBLIC "BOOST_ASIO_NO_DEPRECATED=1")

# Libraries
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

add_subdirectory(extern/fmtlib EXCLUDE_FROM_ALL)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
	# static
	# dynamic
		${CMAKE_THREAD_LIBS_INIT}
		${Boost_LIBRARIES}
		fmt
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
