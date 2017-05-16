cmake_minimum_required(VERSION 3.5)

project(OBBTree)

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
    ${HEAD_DIR}/OBBTree.h
)
set(SRC_FILES
    ${SRC_DIR}/OBBTree.cpp
    ${SRC_DIR}/main.cpp    
)

## link and executable

add_executable(OBBTree ${HEAD_FILES} ${SRC_FILES})