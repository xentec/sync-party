cmake_minimum_required(VERSION 3.2)

project(sp-common CXX)

# include sources
set(SOURCES
	asio.hpp
	asio.cpp
	echo.hpp
	echo.cpp
	logger.hpp
	types.hpp
	util.hpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_compile_features(${PROJECT_NAME} PUBLIC "cxx_std_17") # C++17
target_compile_options(${PROJECT_NAME} PUBLIC "-Wall")       # include all warnings

# Libraries
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

add_subdirectory(extern/fmtlib EXCLUDE_FROM_ALL)
add_subdirectory(extern/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(extern/mqtt_cpp EXCLUDE_FROM_ALL)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		${CMAKE_THREAD_LIBS_INIT}
		${Boost_LIBRARIES}
		fmt
		spdlog
		mqtt_cpp
)
