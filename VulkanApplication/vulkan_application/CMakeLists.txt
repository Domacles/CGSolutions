set(HEAD_FILES 
    ${HEAD_DIR}/VulkanApplication.h
)
set(SRC_FILES
    ${SRC_DIR}/VulkanApplication.cpp
    ${SRC_DIR}/main.cpp
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/header files")

link_libraries(glfw3.lib vulkan-1.lib)
add_executable(VulkanApplication ${HEAD_FILES} ${SRC_FILES})
