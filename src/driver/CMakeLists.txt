cmake_minimum_required(VERSION 3.2)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

project(sp-driver C CXX ASM)

# Options
set(SP_PORT "/dev/ttyACM0" CACHE PATH "The path to serial port")

# Sources
set(${PROJECT_NAME}_SRCS  main.cpp proto-def.hpp)

# Board info
set(${PROJECT_NAME}_BOARD uno)
set(${PROJECT_NAME}_PORT  ${SP_PORT})

generate_arduino_firmware(${PROJECT_NAME})

# Includes for IDE
include_directories(
	${ARDUINO_PLATFORM_PATH}/cores/arduino
	${ARDUINO_PLATFORM_PATH}/variants/standard/
)

install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex DESTINATION arduino/)
