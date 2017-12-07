cmake_minimum_required(VERSION 3.5)

project(VulkanApplication)

## project set
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(HEAD_DIR "./header files")
set(SRC_DIR "./source files")

## other subdirectory
include_directories("D:/ProgramSDK/includes")
link_directories("D:/ProgramSDK/libs")

## program include directories
add_subdirectory(vulkan_application)
add_subdirectory(vulkan_application_qt)
