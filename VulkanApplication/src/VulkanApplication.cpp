#include "VulkanApplication.h"

#undef max
#undef min

#include <set>
#include <cmath>
#include <limits>
#include <string>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <functional>

/////////////////////////////// private function ///////////////////////////////

template<typename Collection, typename FunTypeBefore, typename FunType, typename FunTypeAfter>
void debug_each(Collection& collection, FunTypeBefore before_func, FunType each_func, FunTypeAfter after_func)
{
#ifdef _DEBUG
	before_func(collection);
	for (auto& e : collection)
		each_func(e);
	after_func(collection);
#endif
}

void VulkanApplication::init_extensions()
{
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	_instance_extensions.resize(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
		_instance_extensions.data());

	debug_each(_instance_extensions,
		[](const decltype(_instance_extensions)& extensions) {
		std::cout << "Available instance extensions: " << std::endl;
	},
		[](const typename decltype(_instance_extensions)::value_type& extension) {
		std::cout << "\t" << extension.extensionName << std::endl;
	},
		[](const decltype(_instance_extensions)& extensions) {
		std::cout << std::endl;
	});

	unsigned int glfw_extensions_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

	for (decltype(glfw_extensions_count) i = 0; i < glfw_extensions_count; i++)
		_instance_extension_names.push_back(glfw_extensions[i]);

	debug_each(_instance_extension_names,
		[](const decltype(_instance_extension_names)& extension_names) {
		std::cout << "Required instance extensions: " << std::endl;
	},
		[](const decltype(_instance_extension_names)::value_type& extension_name) {
		std::cout << "\t" << extension_name << std::endl;
	},
		[](const decltype(_instance_extension_names)& extension_names) {
		std::cout << std::endl;
	});
}

void VulkanApplication::init_instance()
{
	VkApplicationInfo app_info = {};
	{
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext = nullptr;
		app_info.pApplicationName = "VulkanApplication";
		app_info.applicationVersion = 1;
		app_info.pEngineName = "VulkanApplication";
		app_info.engineVersion = 1;
		app_info.apiVersion = VK_API_VERSION_1_0;
	}

	VkInstanceCreateInfo create_info = {};
	{
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		create_info.pNext = nullptr;
		create_info.flags = 0;
		create_info.enabledExtensionCount = static_cast<uint32_t>(_instance_extension_names.size());
		create_info.ppEnabledExtensionNames = _instance_extension_names.data();
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = nullptr;
	}

	VkResult res = vkCreateInstance(&create_info, nullptr, &_vkinstance);
	assert(res == VK_SUCCESS);
}

void VulkanApplication::init_surface()
{
	VkWin32SurfaceCreateInfoKHR create_Info;
	{
		create_Info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		create_Info.hwnd = _hwnd;
		create_Info.hinstance = _hinstance;
	}

	VkResult res = vkCreateWin32SurfaceKHR(_vkinstance, &create_Info, nullptr, &_vksurface);
	assert(res == VK_SUCCESS);
}

void VulkanApplication::init_physical_device()
{
	auto physical_device_score = [&](const decltype(_vkphysical_device)& device)
	{
		VkPhysicalDeviceFeatures device_features;
		VkPhysicalDeviceProperties device_properties;

		vkGetPhysicalDeviceFeatures(device, &device_features);
		vkGetPhysicalDeviceProperties(device, &device_properties);

		int score = device_properties.limits.maxImageDimension2D;
		if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;
		if (device_features.geometryShader)
			score += 1000;
		else
			score = 0;
		return score;
	};

	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(_vkinstance, &device_count, nullptr);
	_physical_devices.resize(device_count);
	vkEnumeratePhysicalDevices(_vkinstance, &device_count, _physical_devices.data());

	{
		int max_device_score = -1;
		for (auto& device : _physical_devices)
		{
			int score = physical_device_score(device);
			if (score > max_device_score)
			{
				max_device_score = score;
				_vkphysical_device = device;
			}
		}
		assert(_vkphysical_device != VK_NULL_HANDLE);

		debug_each(_physical_devices,
			[](const decltype(_physical_devices)& devices) {
			std::cout << "Available physical devices num: " << devices.size() << std::endl;
		},
			[](const decltype(_physical_devices)::value_type& device) {},
			[](const decltype(_physical_devices)& devices) {
			std::cout << std::endl;
		});
	}
}

void VulkanApplication::init_logical_device()
{
	auto get_queue_families = [](const decltype(_vkphysical_device)& device)
	{
		uint32_t queue_family_count = 0;
		bool queue_family_found = false;
		std::vector<VkQueueFamilyProperties> queue_families;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		queue_families.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
		assert(queue_family_found == true);
		return queue_families;
	};

	auto find_queue_families_index = [](const decltype(_vkphysical_device)& device,
		const decltype(_vksurface)& surface, const decltype(_queue_families)& queue_families)
		->std::pair<uint32_t, uint32_t>
	{
		VkBool32 support;
		uint32_t queue_family_count = 0;
		std::vector<VkBool32> present_supports;
		{
			vkGetPhysicalDeviceQueueFamilyProperties(device,
				&queue_family_count, nullptr);
			for (uint32_t i = 0; i < queue_family_count; i++)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &support);
				present_supports.push_back(support);
			}
		}

		uint32_t graphics_queue_family_index = std::numeric_limits<uint32_t>::max();
		uint32_t present_queue_family_index = std::numeric_limits<uint32_t>::max();
		{
			for (uint32_t i = 0; i < queue_family_count; i++)
			{
				auto& queue_family = queue_families[i];
				if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					graphics_queue_family_index = i;
				if (present_supports[i] == VK_TRUE)
					present_queue_family_index = i;
				if (graphics_queue_family_index == present_queue_family_index)
					break;
			}
			assert(graphics_queue_family_index != std::numeric_limits<uint32_t>::max());
			assert(present_queue_family_index != std::numeric_limits<uint32_t>::max());
		}

		return std::make_pair(graphics_queue_family_index,
			present_queue_family_index);
	};

	_logical_device_extension_names = { "VK_KHR_swapchain" };
	{
		uint32_t physical_device_extension_count;
		std::vector<VkExtensionProperties> extensions;
		vkEnumerateDeviceExtensionProperties(_vkphysical_device, nullptr,
			&physical_device_extension_count, nullptr);
		extensions.resize(physical_device_extension_count);
		vkEnumerateDeviceExtensionProperties(_vkphysical_device, nullptr,
			&physical_device_extension_count, extensions.data());

		bool is_support_swapchain = false;
		std::set<std::string> required_extensions(_logical_device_extension_names.begin(),
			_logical_device_extension_names.end());
		for (auto& extension : extensions)
			required_extensions.erase(extension.extensionName);
		is_support_swapchain = required_extensions.empty();
		assert(is_support_swapchain == true);

		debug_each(extensions,
			[](const decltype(extensions)& extensions) {
			std::cout << "Available physical device extensions: " << std::endl;
		},
			[](const decltype(extensions)::value_type& extension) {
			std::cout << "\t" << extension.extensionName << std::endl;
		},
			[](const decltype(extensions)& extensions) {
			std::cout << std::endl;
		});
	}

	float queue_priorities[1] = { 1.0f };
	std::vector<VkDeviceQueueCreateInfo> queue_infos;
	{
		auto queue_familiy_indice =
			find_queue_families_index(_vkphysical_device, _vksurface, _queue_families);

		_graphics_queue_family_index = queue_familiy_indice.first;
		_present_queue_family_index = queue_familiy_indice.second;
		_queue_families = get_queue_families(_vkphysical_device);

		std::set<uint32_t> unique_queue_families_indice = {
			_graphics_queue_family_index,
			_present_queue_family_index
		};

		for (auto& queue_family_index : unique_queue_families_indice)
		{
			VkDeviceQueueCreateInfo queue_info = {};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.pNext = nullptr;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = queue_priorities;
			queue_info.queueFamilyIndex = queue_family_index;
			queue_infos.push_back(queue_info);
		}
	}

	VkDeviceCreateInfo device_info = {};
	{
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.pNext = nullptr;
		device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
		device_info.pQueueCreateInfos = queue_infos.data();
		device_info.enabledExtensionCount = static_cast<uint32_t>(
			_logical_device_extension_names.size());
		device_info.ppEnabledExtensionNames = _logical_device_extension_names.data();
		device_info.enabledLayerCount = 0;
		device_info.ppEnabledLayerNames = nullptr;
		device_info.pEnabledFeatures = nullptr;
	}

	VkResult res = vkCreateDevice(_vkphysical_device, &device_info, nullptr, &_vkdevice);
	assert(res == VK_SUCCESS);
}

void  VulkanApplication::init_swapchain_extension()
{
	auto choose_swapchain_surface_format = [](const decltype(_formats)& formats)
		-> VkSurfaceFormatKHR
	{
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
			return VkSurfaceFormatKHR{
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};

		for (auto& element : formats)
		{
			if (element.format == VK_FORMAT_B8G8R8A8_UNORM &&
				element.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return element;
		}

		return formats[0];
	};

	auto choose_swapchain_present_mode = [](const decltype(_present_modes)& present_modes)
		-> VkPresentModeKHR
	{
		for (auto& mode : present_modes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
			if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	};

	auto choose_swapchain_extent = [](const decltype(_vksurface_capabilities)& capabilities, int window_width, int window_height)
		->VkExtent2D
	{
		if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
		{
			VkExtent2D now_extent{
				static_cast<uint32_t>(window_width),
				static_cast<uint32_t>(window_height)
			};
			now_extent.width = std::max(capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, now_extent.width));
			now_extent.height = std::max(capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, now_extent.height));
			return now_extent;
		}
		else
			return capabilities.currentExtent;
	};

	auto choose_swapchain_images_count = [](const decltype(_vksurface_capabilities)& capabilities)
		->uint32_t
	{
		uint32_t images_count = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && images_count > capabilities.maxImageCount)
			images_count = capabilities.maxImageCount;
		return images_count;
	};

	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_vkphysical_device, _vksurface,
			&_vksurface_capabilities);
	}

	{
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(_vkphysical_device, _vksurface,
			&format_count, nullptr);
		_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_vkphysical_device, _vksurface,
			&format_count, _formats.data());
		assert(format_count != 0);
	}

	{
		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(_vkphysical_device, _vksurface,
			&present_mode_count, nullptr);
		_present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(_vkphysical_device, _vksurface,
			&present_mode_count, _present_modes.data());
		assert(present_mode_count != 0);
	}

	VkSwapchainCreateInfoKHR create_info = {};
	{
		std::vector<uint32_t> queue_families_indice = {
			_graphics_queue_family_index,
			_present_queue_family_index
		};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.pNext = NULL;
		create_info.surface = _vksurface;
		create_info.preTransform = _vksurface_capabilities.currentTransform;
		create_info.presentMode = choose_swapchain_present_mode(_present_modes);
		create_info.minImageCount = choose_swapchain_images_count(_vksurface_capabilities);
		create_info.imageFormat = choose_swapchain_surface_format(_formats).format;
		create_info.imageColorSpace = choose_swapchain_surface_format(_formats).colorSpace;
		create_info.imageExtent = choose_swapchain_extent(_vksurface_capabilities,
			_window_width, _window_height);

		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.oldSwapchain = VK_NULL_HANDLE;
		create_info.clipped = VK_TRUE;

		if (_graphics_queue_family_index == _present_queue_family_index)
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = NULL;
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_families_indice.data();
		}

		VkResult res = vkCreateSwapchainKHR(_vkdevice, &create_info, nullptr, &_vkswapchain);
		assert(res == VK_SUCCESS);
	}

	{
		uint32_t swapchain_images_count = 0;
		vkGetSwapchainImagesKHR(_vkdevice, _vkswapchain,
			&swapchain_images_count, NULL);
		_swapchain_images.resize(swapchain_images_count);
		vkGetSwapchainImagesKHR(_vkdevice, _vkswapchain,
			&swapchain_images_count, _swapchain_images.data());
		assert(swapchain_images_count != 0);
	}
}

VulkanApplication::ExecutionStatus VulkanApplication::init_window()
{
	using STATUS = VulkanApplication::ExecutionStatus;

	// glfw init
	{
		auto glfw_init_res = glfwInit();
		if (glfw_init_res != GLFW_TRUE)
		{
			std::cout << "glfw init error, and return " <<
				glfw_init_res << "." << std::endl;
			return STATUS::EXEC_FAULT;
		}
	}

	// glfw create window
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window_ptr = std::shared_ptr<GLFWwindow>(
			glfwCreateWindow(_window_width, _window_height, "VulkanApplication", nullptr, nullptr),
			[](GLFWwindow* window) {});
		if (_window_ptr == nullptr)
		{
			std::cout << "Create window Error!!!" << std::endl;
			return STATUS::EXEC_FAULT;
		}
	}

	// glfw hwnd hinstance
	{
		_hwnd = glfwGetWin32Window(_window_ptr.get());
		_hinstance = GetModuleHandle(nullptr);
	}

	return STATUS::EXEC_SUCCESS;
}

VulkanApplication::ExecutionStatus VulkanApplication::init_vulkan()
{
	using STATUS = VulkanApplication::ExecutionStatus;
	if (glfwVulkanSupported() == GLFW_TRUE)
	{
		// checking for extension support
		init_extensions();

		// vkInstance
		init_instance();

		// use WSI
		init_surface();

		// pick physical device
		init_physical_device();

		//init logical device
		init_logical_device();

		return STATUS::EXEC_SUCCESS;
	}
	else
		std::cout << "Don't support Vulkan." << std::endl;
	return STATUS::EXEC_FAULT;
}

VulkanApplication::ExecutionStatus VulkanApplication::main_loop()
{
	using STATUS = VulkanApplication::ExecutionStatus;

	while (!glfwWindowShouldClose(_window_ptr.get()))
		glfwPollEvents();
	glfwTerminate();

	return STATUS::EXEC_SUCCESS;
}

VulkanApplication::ExecutionStatus VulkanApplication::destroy()
{
	using STATUS = VulkanApplication::ExecutionStatus;

	{
		glfwDestroyWindow(_window_ptr.get());
		_window_ptr.reset();
	}

	return STATUS::EXEC_SUCCESS;
}

////////////////////////////// public functions ////////////////////////////////

void VulkanApplication::run()
{
	init_window();
	init_vulkan();
	main_loop();
	destroy();
}