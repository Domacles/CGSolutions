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
		_vkphysical_device(VK_NULL_HANDLE)
	{};
	~VulkanApplication() {};

	// run
	void run();

protected:
	// window property
	int _window_width;
	int _window_height;

	// glfw object or pointer
	HWND _hwnd;
	HINSTANCE _hinstance;
	std::shared_ptr<GLFWwindow> _window_ptr;

	// vulkan object
	VkDevice _vkdevice;
	VkInstance _vkinstance;
	VkSurfaceKHR _vksurface;
	VkSwapchainKHR _vkswapchain;
	VkCommandPool _vkcommand_pool;
	VkPhysicalDevice _vkphysical_device;
	uint32_t _present_queue_family_index;
	uint32_t _graphics_queue_family_index;
	VkSurfaceCapabilitiesKHR _vksurface_capabilities;

	// vulkan vectors
	std::vector<VkImage> _swapchain_images;
	std::vector<VkSurfaceFormatKHR> _formats;
	std::vector<VkPresentModeKHR> _present_modes;
	std::vector<VkPhysicalDevice> _physical_devices;
	std::vector<const char*> _instance_extension_names;
	std::vector<VkQueueFamilyProperties> _queue_families;
	std::vector<VkExtensionProperties> _instance_extensions;
	std::vector<const char*> _logical_device_extension_names;

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
