#ifndef VULKANAPPLICATION_H
#define VULKANAPPLICATION_H

#define GLFW_EXPOSE_NATIVE_WIN32
#define VK_USE_PLATFORM_WIN32_KHR

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>

#include <vector>
#include <memory>
#include <cstdlib>

class VulkanApplication
{
public:
	// enum
	enum class ExecutionStatus { EXEC_SUCCESS, EXEC_FAULT };

	// constructor
	VulkanApplication() :
		_window_width(800), _window_height(600),
		_physical_device(VK_NULL_HANDLE)
	{};
	~VulkanApplication() {};

	// run
	void run();

protected:
	// window property
	int _window_width;
	int _window_height;

	// vulkan object
	VkDevice _vkdevice;
	VkInstance _vkinstance;
	VkSurfaceKHR _vksurface;
	VkCommandPool _vkcommand_pool;
	VkPhysicalDevice _physical_device;

	// glfw object or pointer
	HWND _hwnd;
	HINSTANCE _hinstance;
	std::shared_ptr<GLFWwindow> _window_ptr;

	// vulkan vectors
	std::vector<VkPhysicalDevice> _physical_devices;
	std::vector<const char*> _instance_extension_names;
	std::vector<VkExtensionProperties> _instance_extensions;
	std::vector<const char*> _logical_device_extension_names;
	std::vector<VkQueueFamilyProperties> _queue_family_props;

	// Vulkan
	void init_surface();
	void init_instance();
	void init_extensions();
	void init_logical_device();
	void init_physical_device();
	void init_swapchain_extension();

	// main function
	ExecutionStatus destroy();
	ExecutionStatus main_loop();
	ExecutionStatus init_vulkan();
	ExecutionStatus init_window();
};

#endif // VULKANAPPLICATION_H
