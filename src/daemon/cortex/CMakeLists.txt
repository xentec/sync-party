set(TARGET_NAME sp-cortex)

add_executable(${TARGET_NAME}
	main.cpp

	driver.hpp
	driver.cpp
	pwm.hpp
	pwm.cpp
)

set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	INTERPROCEDURAL_OPTIMIZATION $<IF:$<CONFIG:Debug>,OFF,ON>
)
target_compile_options(${TARGET_NAME} PUBLIC "-Wall")       # include all warnings

target_link_libraries(${TARGET_NAME}
	PUBLIC
	# static
	# dynamic
	    sp-common
		stdc++fs
)

target_include_directories(${TARGET_NAME}
	PRIVATE
	    ${PROJECT_SOURCE_DIR}/../driver
)

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION bin)
