#include "VulkanApplication.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <memory>
#include <iostream>

/////////////////////////////// private function ///////////////////////////////

void VulkanApplication::init_instance()
{
	const char** glfw_extensions;
	unsigned int glfw_extensions_count = 0;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

	VkApplicationInfo app_info = {};
	VkInstanceCreateInfo create_info = {};

	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = "VulkanApplication";
	app_info.applicationVersion = 1;
	app_info.pEngineName = "VulkanApplication";
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.enabledExtensionCount = glfw_extensions_count;
	create_info.ppEnabledExtensionNames = glfw_extensions;
	create_info.enabledLayerCount = 0;
	create_info.ppEnabledLayerNames = NULL;

	VkResult res = vkCreateInstance(&create_info, NULL, &_vkinstance);
}

void VulkanApplication::init_extensions()
{
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	_extensions.resize(extension_count);
	auto extensions = _extensions.data();
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions);
#ifdef _DEBUG
	std::cout << "available extensions:" << std::endl;
	for (const auto& extension : _extensions)
		std::cout << "\t" << extension.extensionName << std::endl;
#endif // DEBUG
}

void VulkanApplication::setup_debug_callback()
{}

void VulkanApplication::pick_physical_device()
{}

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

	return STATUS::EXEC_SUCCESS;
}

VulkanApplication::ExecutionStatus VulkanApplication::init_vulkan()
{
	using STATUS = VulkanApplication::ExecutionStatus;
	if (glfwVulkanSupported() == GLFW_TRUE)
	{
		// vkInstance
		init_instance();

		// Checking for extension support
		init_extensions();

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

VulkanApplication::~VulkanApplication()
{}