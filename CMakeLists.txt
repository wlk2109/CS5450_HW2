cmake_minimum_required(VERSION 3.12.3)  # CMake version check
project(p2pApp)               # Create project "simple_example"
set(CMAKE_CXX_STANDARD 14)            # Enable c++14 standard

# Add main.c file of project root directory as source file
set(SOURCE_FILES process_old.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(process ${SOURCE_FILES})
ADD_LIBRARY(p2pApp SHARED p2pApp.c)
SET_TARGET_PROPERTIES(p2pApp PROPERTIES RULE_LAUNCH_LINK
        "${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} <OBJECTS> --")