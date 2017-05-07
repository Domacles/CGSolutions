cmake_minimum_required(VERSION 3.5)

project(VulkanApplication)

## project set

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

## other include and libs

include_directories("D:/ProgramSDK/includes")
link_directories("D:/ProgramSDK/libs")

## main include and src

include_directories(${CMAKE_SOURCE_DIR}/include)

set(HEAD_DIR "include")
set(SRC_DIR "src")

set(HEAD_FILES 
    ${HEAD_DIR}/VulkanApplication.h
)
set(SRC_FILES
    ${SRC_DIR}/VulkanApplication.cpp
    ${SRC_DIR}/main.cpp    
)

## link and executable

link_libraries(glfw3.lib vulkan-1.lib)
add_executable(VulkanApplication ${HEAD_FILES} ${SRC_FILES})

