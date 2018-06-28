set(TARGET_NAME sp-controller)

add_executable(${TARGET_NAME}
	main.cpp
	controller.hpp
	controller.cpp
)
set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	INTERPROCEDURAL_OPTIMIZATION $<IF:$<CONFIG:Debug>,OFF,ON>
)
target_compile_options(${TARGET_NAME} PUBLIC "-Wall")       # include all warnings

#target_compile_definitions(${TARGET_NAME} PUBLIC "BOOST_ASIO_NO_DEPRECATED=1")

target_link_libraries(${TARGET_NAME}
	PUBLIC
	# static
	# dynamic
	    sp-common
)

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION bin)
