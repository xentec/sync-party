set(TARGET_NAME sp-common)

add_library(${TARGET_NAME} STATIC
	def.hpp

	asio.hpp
	asio.cpp
	echo.hpp
	echo.cpp
	opts.hpp
	opts.cpp
	logger.hpp
	timer.hpp
	timer.cpp
	types.hpp
	util.hpp
)

set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_compile_options(${TARGET_NAME} PUBLIC "-Wall")       # include all warnings

# Libraries
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${TARGET_NAME}
	PUBLIC
	    ${CMAKE_THREAD_LIBS_INIT}
		${Boost_LIBRARIES}
		argh
		fmt
		spdlog
		mqtt_cpp
)
