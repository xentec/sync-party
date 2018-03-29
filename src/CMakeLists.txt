

# include sources
set(SOURCES main.cpp)

add_executable(sync-drive ${SOURCES})

set_target_properties(sync-drive PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	INTERPROCEDURAL_OPTIMIZATION $<IF:$<CONFIG:Debug>,OFF,ON>
)
target_compile_features(sync-drive PUBLIC "cxx_std_17") # C++17
target_compile_options(sync-drive PUBLIC "-Wall")       # include all warnings

# Libraries
find_package(Threads REQUIRED)

target_link_libraries(sync-drive
	PUBLIC
	# static
	# dynamic
		${CMAKE_THREAD_LIBS_INIT}
		mosquitto
)

