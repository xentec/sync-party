cmake_minimum_required(VERSION 3.2)

project(sync-party LANGUAGES CXX)

add_subdirectory(extern/argh EXCLUDE_FROM_ALL)
add_subdirectory(extern/fmtlib EXCLUDE_FROM_ALL)
add_subdirectory(extern/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(extern/mqtt_cpp EXCLUDE_FROM_ALL)

add_subdirectory(common)
add_subdirectory(cortex)
add_subdirectory(controller)
