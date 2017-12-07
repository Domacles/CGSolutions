SET(CMAKE_PREFIX_PATH "C:\\dev\\Qt5.9.0\\5.9\\msvc2015_64\\")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)
find_package(Qt5Widgets)

file(GLOB MOC "./header files/*window.h")
qt5_wrap_cpp(HEADERS_MOC ${MOC})

file(GLOB FORMS "./ui files/*.ui")
qt5_wrap_ui(FORMS_UIC ${FORMS})

file(GLOB RES "./qrc files/*.qrc")
qt5_add_resources(RES_RCC ${RES})

set(HEAD_FILES
    ${HEAD_DIR}/Vulkan.h
    ${HEAD_DIR}/vulkanwindow.h  
)
set(SRC_FILES
    ${SRC_DIR}/qmain.cpp
    ${SRC_DIR}/Vulkan.cpp
    ${SRC_DIR}/vulkanwindow.cpp
)

link_libraries(vulkan-1.lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/header files")

source_group("Generated Files" FILES ${HEADERS_MOC} ${FORMS_UIC} "gui_automoc.cpp")

add_executable(VulkanApplication_Qt ${HEAD_FILES} ${SRC_FILES} ${FORMS_UIC})
target_link_libraries(VulkanApplication_Qt Qt5::Widgets)