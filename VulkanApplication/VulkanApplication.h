#ifndef VULKANAPPLICATION_H
#define VULKANAPPLICATION_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>

class VulkanApplication
{
public:
	// enum
	enum class ExecutionStatus { EXEC_SUCCESS, EXEC_FAULT };
	// function
	VulkanApplication() :
		_window_ptr(nullptr), _window_width(800), _window_height(600), _vkinstance()
	{};
	~VulkanApplication();
	// run
	void run();

protected:
	int _window_width;
	int _window_height;
	VkInstance _vkinstance;
	std::shared_ptr<GLFWwindow> _window_ptr;
	std::vector<VkExtensionProperties> _extensions;
	// Vulkan
	void init_instance();
	void init_extensions();
	void setup_debug_callback();
	void pick_physical_device();
	// main function
	ExecutionStatus init_window();
	ExecutionStatus init_vulkan();
	ExecutionStatus main_loop();
	ExecutionStatus destroy();
};

#endif // VULKANAPPLICATION_H
