VUlkanApplication
===

本工程用于练习glfw与Vulkan创建三维图形显示程序，该工程依赖glfw和Vulkan，使用的开发环境为Microsoft Visual Studio 2015 Community，目标为win64程序。

CMake需要手动修改`## other include and libs`下的两项：
+ 第一项为`include`目录，存放着GLFW和Vulkan的头文件，以及glm的头文件；
+ 第二项为`libs`目录，存放着GLFW和Vulkan的`lib`文件。

注意，编译Debug版时会提示`MSVCRT.lib`与其他库冲突的警告，即LNK4098，这是因为GLFW用的是`MD`版的lib运行库，我选择的是忽略`MSVCRT.lib`。
