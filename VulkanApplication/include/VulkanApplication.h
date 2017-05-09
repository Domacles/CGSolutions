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
		_window_width(800), _window_height(600)
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
	VkExtent2D _swapchain_extent;
	VkFormat _swapchain_image_format;
	uint32_t _present_queue_family_index;
	uint32_t _graphics_queue_family_index;
	VkSurfaceCapabilitiesKHR _vksurface_capabilities;
	VkPipelineShaderStageCreateInfo _shader_stages[2];

	// vulkan object pointer
	std::shared_ptr<VkDevice_T> _vkdevice_ptr;
	std::shared_ptr<VkInstance_T> _vkinstance_ptr;
	std::shared_ptr<VkSurfaceKHR_T> _vksurface_ptr;
	std::shared_ptr<VkSwapchainKHR_T> _vkswapchain_ptr;
	std::shared_ptr<VkCommandPool_T> _vkcommand_pool_ptr;
	std::shared_ptr<VkShaderModule_T> _vert_shader_module_ptr;
	std::shared_ptr<VkShaderModule_T> _frag_shader_module_ptr;
	std::shared_ptr<VkPhysicalDevice_T> _vkphysical_device_ptr;

	// vulkan vectors for structs
	std::vector<VkSurfaceFormatKHR> _formats;
	std::vector<VkPresentModeKHR> _present_modes;
	std::vector<VkQueueFamilyProperties> _queue_families;
	std::vector<VkExtensionProperties> _instance_extensions;

	// vulkan vectors for pointers
	std::vector<char> _vshader_spir_v_bytes;
	std::vector<char> _fshader_spir_v_bytes;
	std::vector<const char*> _instance_extension_names;
	std::vector<const char*> _logical_device_extension_names;
	std::vector<std::shared_ptr<VkImageView_T>> _image_views;
	std::vector<std::shared_ptr<VkImage_T>> _swapchain_images;
	std::vector<std::shared_ptr<VkPhysicalDevice_T>> _physical_devices;

	// Vulkan
	void init_surface();
	void init_instance();
	void init_extensions();
	void init_image_views();
	void init_logical_device();
	void init_physical_device();
	void init_swapchain_extension();

	void create_image_views();
	void create_graphics_pipeline();

	// main function
	ExecutionStatus destroy();
	ExecutionStatus main_loop();
	ExecutionStatus init_vulkan();
	ExecutionStatus init_window();
};

#endif // VULKANAPPLICATION_H
