#include "Vulkan.h"

#include <map>
#include <set>
#include <cmath>
#include <thread>
#include <chrono>
#include <limits>
#include <string>
#include <cstdio>
#include <cassert>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <loader/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <loader/tiny_obj_loader.h>

#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#undef max
#undef min

#pragma warning(disable: 4100)

namespace std {
template<> struct hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((hash<glm::vec3>()(vertex.pos) ^
			(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
			(hash<glm::vec2>()(vertex.tex_coord) << 1);
	}
};
}

const std::string MODEL_PATH = "chalet.obj";
const std::string TEXTURE_PATH = "chalet.jpg";

/****************************************************************************/

bool Vertex::operator==(const Vertex& other) const
{
	return pos == other.pos && color == other.color &&
		tex_coord == other.tex_coord;
}

VkVertexInputBindingDescription Vertex::binding_description()
{
	VkVertexInputBindingDescription vk_binding_description = {};
	{
		vk_binding_description.binding = 0;
		vk_binding_description.stride = sizeof(Vertex);
		vk_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}
	return vk_binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::attribute_descriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> vk_attribute_descriptions = {};
	{
		vk_attribute_descriptions[0].binding = 0;
		vk_attribute_descriptions[0].location = 0;
		vk_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vk_attribute_descriptions[0].offset = offsetof(Vertex, pos);

		vk_attribute_descriptions[1].binding = 0;
		vk_attribute_descriptions[1].location = 1;
		vk_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vk_attribute_descriptions[1].offset = offsetof(Vertex, color);

		vk_attribute_descriptions[2].binding = 0;
		vk_attribute_descriptions[2].location = 2;
		vk_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		vk_attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);
	}
	return vk_attribute_descriptions;
}

/****************************************************************************/

uint32_t find_memory_type(VkPhysicalDevice physical_device,
	uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	uint32_t res = 0;
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
		if ((type_filter & (1 << i)) &&
			(memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	assert(false);
	return res;
};

VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool command_pool)
{
	VkCommandBufferAllocateInfo allocate_info = {};
	{
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandPool = command_pool;
		allocate_info.commandBufferCount = 1;
	}

	VkCommandBuffer command_buffer;
	VkResult res = vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);
	assert(res == VK_SUCCESS);

	VkCommandBufferBeginInfo begin_info = {};
	{
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	}

	vkBeginCommandBuffer(command_buffer, &begin_info);
	return command_buffer;
}

void end_single_time_commands(VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue,
	VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {};
	{
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;
	}
	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void create_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
	VkBufferCreateInfo buffer_info = {};
	{
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkResult res1 = vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
	assert(res1 == VK_SUCCESS);

	VkMemoryRequirements requirement;
	VkMemoryAllocateInfo alloc_info = {};
	{
		vkGetBufferMemoryRequirements(device, buffer, &requirement);

		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = requirement.size;
		alloc_info.memoryTypeIndex = find_memory_type(physical_device,
			requirement.memoryTypeBits, properties);
	}

	VkResult res2 = vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory);
	assert(res2 == VK_SUCCESS);

	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void create_image(VkPhysicalDevice physical_device, VkDevice device, uint32_t width, uint32_t height,
	VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory)
{
	VkImageCreateInfo image_info = {};
	{
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		image_info.usage = usage;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	VkResult res0 = vkCreateImage(device, &image_info, nullptr, &image);
	assert(res0 == VK_SUCCESS);

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(device, image, &mem_requirements);

	VkMemoryAllocateInfo allocInfo = {};
	{
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = mem_requirements.size;
		allocInfo.memoryTypeIndex = find_memory_type(physical_device,
			mem_requirements.memoryTypeBits, properties);
	}

	VkResult res1 = vkAllocateMemory(device, &allocInfo, nullptr, &image_memory);
	assert(res1 == VK_SUCCESS);

	vkBindImageMemory(device, image, image_memory, 0);
}

void copy_buffer(VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue,
	VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer command_buffer = begin_single_time_commands(device, command_pool);
	{
		VkBufferCopy copy_region = {};
		{
			copy_region.srcOffset = 0;
			copy_region.dstOffset = 0;
			copy_region.size = size;
		}
		vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
	}
	end_single_time_commands(device, command_pool, graphics_queue, command_buffer);
}

void copy_buffer_to_image(VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue,
	VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = begin_single_time_commands(device, command_pool);
	{
		VkBufferImageCopy region = {};
		{
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };
		}
		vkCmdCopyBufferToImage(command_buffer, buffer, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	end_single_time_commands(device, command_pool, graphics_queue, command_buffer);
}

void transition_image_layout(VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue,
	VkImage image, VkFormat format,
	VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = begin_single_time_commands(device, command_pool);
	{
		VkImageMemoryBarrier barrier = {};
		{
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = old_layout;
			barrier.newLayout = new_layout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;

			if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
					format == VK_FORMAT_D24_UNORM_S8_UINT)
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			if (old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
				new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			}
			else if (old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
				new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
				new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}
			else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
				new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}
			else
				assert(false);
		}

		vkCmdPipelineBarrier(command_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
	end_single_time_commands(device, command_pool, graphics_queue, command_buffer);
};

VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
	VkImageViewCreateInfo view_info = {};
	{
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = format;
		view_info.subresourceRange.aspectMask = aspect_flags;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;
	}

	VkImageView image_view;
	VkResult res = vkCreateImageView(device, &view_info, nullptr, &image_view);
	assert(res == VK_SUCCESS);
	return image_view;
}

std::pair<uint32_t, uint32_t> find_queue_families_index(
	VkPhysicalDevice device, VkSurfaceKHR surface,
	std::vector<VkQueueFamilyProperties>& queue_families)
{
	VkBool32 support;
	uint32_t queue_family_count = 0;
	std::vector<VkBool32> present_supports;
	{
		vkGetPhysicalDeviceQueueFamilyProperties(device,
			&queue_family_count, nullptr);
		present_supports.resize(queue_family_count);
		for (uint32_t i = 0; i < queue_family_count; i++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i,
				surface, &support);
			present_supports[i] = support;
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
}

std::vector<VkQueueFamilyProperties> query_queue_families(VkPhysicalDevice device)
{
	uint32_t queue_family_count = 0;
	std::vector<VkQueueFamilyProperties> queue_families;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	queue_families.resize(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
		queue_families.data());
	return queue_families;
};

bool check_device_extension_support(VkPhysicalDevice device)
{
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count,
		available_extensions.data());

	std::set<std::string> required_extensions(
		device_extensions.begin(), device_extensions.end());

	for (const auto& extension : available_extensions)
		required_extensions.erase(extension.extensionName);

	return required_extensions.empty();
}

std::tuple<VkSurfaceCapabilitiesKHR,
	std::vector<VkSurfaceFormatKHR>, std::vector<VkPresentModeKHR>>
	query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t format_count;
	uint32_t present_mode_count;
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
	formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data());

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
	present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
		present_modes.data());

	return make_tuple(capabilities, formats, present_modes);
}

VkFormat find_support_format(VkPhysicalDevice device, const std::vector<VkFormat>& candidates,
	VkImageTiling tiling, VkFormatFeatureFlags features)
{
	VkFormat res = {};
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR &&
			(props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & features) == features)
			return format;
	}
	assert(false);
	return res;
}

/////////////////////////////// private function ///////////////////////////////

void Vulkan::init_window(HWND hwnd, HINSTANCE hinstance, int width, int height)
{
	_hwnd = hwnd;
	_hinstance = hinstance;
	_window_width = width, _window_height = height;
}

void Vulkan::start_run()
{
	_stop_draw = false;
	std::thread show_thread([&]() {
		vkDeviceWaitIdle(_device.get());
		while (_stop_draw == false)
		{
			this->update_uniform_buffer();
			this->draw_frame();
		}
		vkDeviceWaitIdle(_device.get());
	});
	show_thread.detach();
}

void Vulkan::end_run()
{
	_stop_draw = true;
}

void Vulkan::init_vulkan()
{
	load_model();
	init_extensions();
	init_instance();
	init_surface();
	init_physical_device();
	init_logical_device();
	create_command_pool();
	create_swapchain();
	create_image_views();
	create_renderpass();
	create_descriptor_set_layout();
	create_graphics_pipeline();
	create_depth_image();
	create_depth_image_view();
	create_texture_image();
	create_texture_image_view();
	create_texture_sampler();
	create_frame_buffers();
	create_vertex_buffer();
	create_indice_buffer();
	create_uniform_buffer();
	create_descriptor_pool();
	create_descriptor_set();
	create_command_buffers();
	create_semaphores();
}

void Vulkan::clear_vulkan()
{
	clear_swapchain();
	_texture_sampler.reset();
	_texture_image_view.reset();
	_texture_image.reset();
	_texture_image_memory.reset();
	_descriptor_pool.reset();
	_descriptor_set_layout.reset();
	_uniform_buffer.reset();
	_uniform_buffer_memory.reset();
	_indices_buffer.reset();
	_indices_buffer_memory.reset();
	_vertex_buffer.reset();
	_vertex_buffer_memory.reset();
	_image_available_semaphore.reset();
	_render_finished_semaphore.reset();
	_command_pool.reset();
	_device.reset();
	_surface.reset();
	_instance.reset();
}

////////////////////////////////// init functions ///////////////////////////////////

void Vulkan::init_extensions()
{
	//	uint32_t extension_count = 0;
	// 	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	// 	_instance_extensions.resize(extension_count);
	// 	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
	// 		_instance_extensions.data());

	_instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	_instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

void Vulkan::init_instance()
{
	VkApplicationInfo app_info = {};
	{
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext = nullptr;
		app_info.pApplicationName = "Vulkan";
		app_info.applicationVersion = 1;
		app_info.pEngineName = "Vulkan";
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

	VkInstance instance;
	VkResult res = vkCreateInstance(&create_info, nullptr, &instance);
	assert(res == VK_SUCCESS);

	_instance = std::shared_ptr<VkInstance_T>(instance,
		[](VkInstance instance) {
		if (instance != VK_NULL_HANDLE)
			vkDestroyInstance(instance, nullptr);
	});
}

void Vulkan::init_surface()
{
	VkWin32SurfaceCreateInfoKHR create_Info;
	{
		create_Info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		create_Info.hwnd = _hwnd;
		create_Info.hinstance = _hinstance;
	}

	VkSurfaceKHR surface;
	VkResult res = vkCreateWin32SurfaceKHR(_instance.get(), &create_Info, nullptr,
		&surface);
	assert(res == VK_SUCCESS);

	_surface = std::shared_ptr<VkSurfaceKHR_T>(surface,
		[&](VkSurfaceKHR surface) {
		if (surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(_instance.get(), surface, nullptr);
	});
}

void Vulkan::init_physical_device()
{
	auto is_device_suitable = [](VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		auto queue_families = query_queue_families(device);
		auto indices = find_queue_families_index(device, surface, queue_families);

		bool swapChainAdequate = false;
		bool extensions_supported = check_device_extension_support(device);

		if (extensions_supported == true)
		{
			auto swapchain_support = query_swapchain_support(device, surface);
			swapChainAdequate =
				!std::get<1>(swapchain_support).empty() &&
				!std::get<2>(swapchain_support).empty();
		}

		VkPhysicalDeviceFeatures supported_features;
		vkGetPhysicalDeviceFeatures(device, &supported_features);

		return indices.first >= 0 && indices.second >= 0 &&
			extensions_supported && supported_features.samplerAnisotropy;
	};

	auto physical_device_score = [&](VkPhysicalDevice device)
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

	std::vector<VkPhysicalDevice> physical_devices;
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(_instance.get(), &device_count, nullptr);
		physical_devices.resize(device_count);
		vkEnumeratePhysicalDevices(_instance.get(), &device_count,
			physical_devices.data());
		_physical_devices.resize(device_count);
		for (size_t i = 0; i < physical_devices.size(); i++)
		{
			auto& device = physical_devices[i];
			if (is_device_suitable(device, _surface.get()) == true)
				_physical_devices[i] = std::shared_ptr<VkPhysicalDevice_T>(device,
					[](VkPhysicalDevice device) {});
		}
	}

	int max_device_score = -1;
	for (auto& device_ptr : _physical_devices)
	{
		int score = physical_device_score(device_ptr.get());
		if (score > max_device_score)
		{
			max_device_score = score;
			_physical_device = device_ptr;
		}
	}
	assert(_physical_device.get() != VK_NULL_HANDLE);
}

void Vulkan::init_logical_device()
{
	_logical_device_extension_names = { "VK_KHR_swapchain" };
	{
		uint32_t physical_device_extension_count;
		std::vector<VkExtensionProperties> extensions;
		vkEnumerateDeviceExtensionProperties(_physical_device.get(), nullptr,
			&physical_device_extension_count, nullptr);
		extensions.resize(physical_device_extension_count);
		vkEnumerateDeviceExtensionProperties(_physical_device.get(), nullptr,
			&physical_device_extension_count, extensions.data());

		bool is_support_swapchain = false;
		std::set<std::string> required_extensions(_logical_device_extension_names.begin(),
			_logical_device_extension_names.end());
		for (auto& extension : extensions)
			required_extensions.erase(extension.extensionName);
		is_support_swapchain = required_extensions.empty();
		assert(is_support_swapchain == true);
	}

	float queue_priorities[1] = { 1.0f };
	std::vector<VkDeviceQueueCreateInfo> queue_infos;
	{
		_queue_families = query_queue_families(_physical_device.get());
		auto queue_familiy_indice = find_queue_families_index(_physical_device.get(),
			_surface.get(), _queue_families);

		_graphics_queue_family_index = queue_familiy_indice.first;
		_present_queue_family_index = queue_familiy_indice.second;

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

	VkDevice device;
	VkResult res = vkCreateDevice(_physical_device.get(), &device_info, nullptr, &device);
	assert(res == VK_SUCCESS);

	VkQueue graphics_queue, present_queue;
	vkGetDeviceQueue(device, _graphics_queue_family_index, 0, &graphics_queue);
	vkGetDeviceQueue(device, _present_queue_family_index, 0, &present_queue);

	_device = std::shared_ptr<VkDevice_T>(device,
		[](VkDevice device) {
		if (device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(device);
			vkDestroyDevice(device, nullptr);
		}
	});

	_graphics_queue = std::shared_ptr<VkQueue_T>(graphics_queue, [](VkQueue queue) {});
	_present_queue = std::shared_ptr<VkQueue_T>(present_queue, [](VkQueue queue) {});
}

////////////////////////////////// create functions /////////////////////////////////

void  Vulkan::create_swapchain()
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

	auto choose_swapchain_extent = [&](const decltype(_vksurface_capabilities)& capabilities, int window_width, int window_height)
		->VkExtent2D
	{
		if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
		{
			int width, height;
			width = window_width, height = window_height;

			VkExtent2D now_extent{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
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
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device.get(),
			_surface.get(), &_vksurface_capabilities);
	}

	{
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device.get(),
			_surface.get(), &format_count, nullptr);
		_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device.get(),
			_surface.get(), &format_count, _formats.data());
		assert(format_count != 0);
	}

	{
		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device.get(),
			_surface.get(), &present_mode_count, nullptr);
		_present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device.get(),
			_surface.get(), &present_mode_count, _present_modes.data());
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
		create_info.surface = _surface.get();
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

		VkSwapchainKHR swapchain;
		VkResult res = vkCreateSwapchainKHR(_device.get(), &create_info,
			nullptr, &swapchain);
		assert(res == VK_SUCCESS);

		_swapchain = std::shared_ptr<VkSwapchainKHR_T>(swapchain,
			[&](VkSwapchainKHR swapchain) {
			if (swapchain != VK_NULL_HANDLE)
				vkDestroySwapchainKHR(_device.get(), swapchain, nullptr);
		});

		_swapchain_extent = create_info.imageExtent;
		_swapchain_image_format = create_info.imageFormat;
	}

	{
		uint32_t swapchain_images_count = 0;
		std::vector<VkImage> swapchain_images;
		vkGetSwapchainImagesKHR(_device.get(), _swapchain.get(),
			&swapchain_images_count, NULL);
		swapchain_images.resize(swapchain_images_count);
		vkGetSwapchainImagesKHR(_device.get(), _swapchain.get(),
			&swapchain_images_count, swapchain_images.data());
		assert(swapchain_images_count != 0);

		_swapchain_images.resize(swapchain_images.size());
		for (size_t i = 0; i < swapchain_images.size(); i++)
		{
			auto& image = swapchain_images[i];
			_swapchain_images[i] = std::shared_ptr<VkImage_T>(image,
				[](VkImage image) {});
		}
	}
}

void Vulkan::create_image_views()
{
	_image_views.resize(_swapchain_images.size());
	for (size_t i = 0; i < _swapchain_images.size(); i++)
	{
		VkImageView image_view = create_image_view(_device.get(),
			_swapchain_images[i].get(), _swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);

		_image_views[i] = std::shared_ptr<VkImageView_T>(image_view,
			[&](VkImageView image_view) {
			if (image_view != VK_NULL_HANDLE)
				vkDestroyImageView(_device.get(), image_view, nullptr);
		});
	}
}

void Vulkan::create_renderpass()
{
	const std::vector<VkFormat>& candidates = {
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
	};

	VkAttachmentDescription depth_attachment = {};
	{
		depth_attachment.format = find_support_format(_physical_device.get(),
			candidates,
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference depth_attachment_ref = {};
	{
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentDescription color_attachment = {};
	{
		color_attachment.format = _swapchain_image_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	VkAttachmentReference color_attachment_ref = {};
	{
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {};
	{
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;
	}

	VkSubpassDependency dependency = {};
	{
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	VkRenderPassCreateInfo renderpass_info = {};
	std::array<VkAttachmentDescription, 2> attachments = {
		color_attachment, depth_attachment
	};
	{
		renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderpass_info.pAttachments = attachments.data();
		renderpass_info.subpassCount = 1;
		renderpass_info.pSubpasses = &subpass;
		renderpass_info.dependencyCount = 1;
		renderpass_info.pDependencies = &dependency;
	}

	VkRenderPass renderpass;
	VkResult res = vkCreateRenderPass(_device.get(), &renderpass_info, nullptr, &renderpass);

	assert(res == VK_SUCCESS);

	_renderpass = std::shared_ptr<VkRenderPass_T>(renderpass,
		[&](VkRenderPass renderpass) {
		vkDestroyRenderPass(_device.get(), renderpass, nullptr);
	});
}

void Vulkan::create_graphics_pipeline()
{
	auto read_file = [](const std::string& file_name)
		->std::vector<char>
	{
		std::vector<char> buffer;
		std::ifstream file(file_name, std::ios::ate | std::ios::binary);
		if (file.is_open())
		{
			size_t file_size = (size_t)file.tellg();
			buffer.resize(file_size);
			file.seekg(0);
			file.read(buffer.data(), file_size);
			file.close();
		}
		else
			std::cout << "Not has this file." << std::endl;
		assert(buffer.size());
		return buffer;
	};

	auto create_shader_module = [&](const std::vector<char>& code)
		->std::shared_ptr<VkShaderModule_T>
	{
		VkShaderModule shader_module;
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<uint32_t*>(
			const_cast<char*>(code.data()));
		VkResult res = vkCreateShaderModule(_device.get(), &create_info,
			nullptr, &shader_module);
		assert(res == VK_SUCCESS);
		return std::shared_ptr<VkShaderModule_T>(shader_module,
			[&](VkShaderModule shader_module) {
			if (shader_module != VK_NULL_HANDLE)
				vkDestroyShaderModule(_device.get(), shader_module, nullptr);
		});
	};

	_vshader_spir_v_bytes = read_file("vert.spv");
	_fshader_spir_v_bytes = read_file("frag.spv");

	auto vert_shader_module_ptr = create_shader_module(_vshader_spir_v_bytes);
	auto frag_shader_module_ptr = create_shader_module(_fshader_spir_v_bytes);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	{
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_shader_module_ptr.get();
		vert_shader_stage_info.pName = "main";
	}
	_shader_stages[0] = vert_shader_stage_info;

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	{
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_shader_module_ptr.get();
		frag_shader_stage_info.pName = "main";
	}
	_shader_stages[1] = frag_shader_stage_info;

	// vertex input state
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	auto vex_binding_description = Vertex::binding_description();
	auto vex_attribute_descriptions = Vertex::attribute_descriptions();
	{
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &vex_binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(
			vex_attribute_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = vex_attribute_descriptions.data();
	}

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	{
		input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;
	}

	// viewport and scissors
	VkPipelineViewportStateCreateInfo viewport_state_info = {};
	VkViewport viewport = {};
	VkRect2D scissor = {};
	{
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(_swapchain_extent.width);
		viewport.height = static_cast<float>(_swapchain_extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		scissor.offset = { 0, 0 };
		scissor.extent = _swapchain_extent;

		viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state_info.viewportCount = 1;
		viewport_state_info.pViewports = &viewport;
		viewport_state_info.scissorCount = 1;
		viewport_state_info.pScissors = &scissor;
	}

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
	{
		rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer_info.depthClampEnable = VK_FALSE;
		rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
		rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer_info.lineWidth = 1.0f;
		rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer_info.depthBiasEnable = VK_FALSE;
	}

	// multisampling
	VkPipelineMultisampleStateCreateInfo multisampling_info = {};
	{
		multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling_info.sampleShadingEnable = VK_FALSE;
		multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	}

	// depth stencil
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
	{
		depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_info.depthTestEnable = VK_TRUE;
		depth_stencil_info.depthWriteEnable = VK_TRUE;
		depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_info.stencilTestEnable = VK_FALSE;
		depth_stencil_info.minDepthBounds = 0.0f; // Optional
		depth_stencil_info.maxDepthBounds = 1.0f; // Optional
		depth_stencil_info.front = {}; // Optional
		depth_stencil_info.back = {}; // Optional
	}

	// color blend
	VkPipelineColorBlendStateCreateInfo color_blending_info = {};
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	{
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;

		color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending_info.logicOpEnable = VK_FALSE;
		color_blending_info.logicOp = VK_LOGIC_OP_COPY;
		color_blending_info.attachmentCount = 1;
		color_blending_info.pAttachments = &color_blend_attachment;
		color_blending_info.blendConstants[0] = 0.0f;
		color_blending_info.blendConstants[1] = 0.0f;
		color_blending_info.blendConstants[2] = 0.0f;
		color_blending_info.blendConstants[3] = 0.0f;
	}

	// dynamic state
	{}

	// pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	VkDescriptorSetLayout layouts[] = { _descriptor_set_layout.get() };
	{
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = layouts;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = 0;
	}

	VkPipelineLayout pipeline_layout;
	VkResult res1 = vkCreatePipelineLayout(_device.get(), &pipeline_layout_info,
		nullptr, &pipeline_layout);

	assert(res1 == VK_SUCCESS);

	_pipeline_layout = std::shared_ptr<VkPipelineLayout_T>(pipeline_layout,
		[&](VkPipelineLayout pipeline_layout) {
		vkDestroyPipelineLayout(_device.get(), pipeline_layout, nullptr);
	});

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	{
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = _shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly_info;
		pipeline_info.pViewportState = &viewport_state_info;
		pipeline_info.pRasterizationState = &rasterizer_info;
		pipeline_info.pMultisampleState = &multisampling_info;
		pipeline_info.pColorBlendState = &color_blending_info;
		pipeline_info.pDepthStencilState = &depth_stencil_info;
		pipeline_info.layout = _pipeline_layout.get();
		pipeline_info.renderPass = _renderpass.get();
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	}

	VkPipeline graphics_pipeline;
	VkResult res2 = vkCreateGraphicsPipelines(_device.get(), VK_NULL_HANDLE, 1,
		&pipeline_info, nullptr, &graphics_pipeline);

	assert(res2 == VK_SUCCESS);

	_graphics_pipeline = std::shared_ptr<VkPipeline_T>(graphics_pipeline,
		[&](VkPipeline graphics_pipeline) {
		vkDestroyPipeline(_device.get(), graphics_pipeline, nullptr);
	});
}

void Vulkan::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	{
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;
	}

	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	{
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;
	}

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	std::array<VkDescriptorSetLayoutBinding, 2> bing_array = {
		ubo_layout_binding, sampler_layout_binding };
	{
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(bing_array.size());
		layout_info.pBindings = bing_array.data();
	}

	VkDescriptorSetLayout layout;
	VkResult res = vkCreateDescriptorSetLayout(_device.get(), &layout_info,
		nullptr, &layout);
	assert(res == VK_SUCCESS);
	_descriptor_set_layout = std::shared_ptr<VkDescriptorSetLayout_T>(layout,
		[&](VkDescriptorSetLayout layout) {
		vkDestroyDescriptorSetLayout(_device.get(), layout, nullptr);
	});
}

void Vulkan::create_frame_buffers()
{
	_swapchain_framebuffers.resize(_image_views.size());
	for (size_t i = 0; i < _image_views.size(); i++)
	{
		VkFramebufferCreateInfo framebuffer_info = {};
		std::array<VkImageView, 2> attachments = {
			_image_views[i].get(), _depth_image_view.get()
		};
		{
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = _renderpass.get();
			framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = _swapchain_extent.width;
			framebuffer_info.height = _swapchain_extent.height;
			framebuffer_info.layers = 1;
		}
		VkFramebuffer framebuffer;
		VkResult res = vkCreateFramebuffer(_device.get(), &framebuffer_info,
			nullptr, &framebuffer);

		assert(res == VK_SUCCESS);

		_swapchain_framebuffers[i] = std::shared_ptr<VkFramebuffer_T>(framebuffer,
			[&](VkFramebuffer framebuffer) {
			vkDestroyFramebuffer(_device.get(), framebuffer, nullptr);
		});
	}
}

void Vulkan::create_depth_image()
{
	const std::vector<VkFormat>& candidates = {
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
	};

	VkFormat depth_format = find_support_format(
		_physical_device.get(), candidates,
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	{
		create_image(_physical_device.get(), _device.get(),
			_swapchain_extent.width, _swapchain_extent.height,
			depth_format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depth_image, depth_image_memory);
	}

	transition_image_layout(_device.get(), _command_pool.get(), _graphics_queue.get(),
		depth_image, depth_format,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	_depth_image = std::shared_ptr<VkImage_T>(depth_image,
		[&](VkImage image) {
		vkDestroyImage(_device.get(), image, nullptr);
	});

	_depth_image_memory = std::shared_ptr<VkDeviceMemory_T>(depth_image_memory,
		[&](VkDeviceMemory memory) {
		vkFreeMemory(_device.get(), memory, nullptr);
	});
}

void Vulkan::create_depth_image_view()
{
	const std::vector<VkFormat>& candidates = {
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
	};

	VkFormat depth_format = find_support_format(
		_physical_device.get(), candidates,
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	VkImageView depth_image_view = create_image_view(_device.get(),
		_depth_image.get(), depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

	_depth_image_view = std::shared_ptr<VkImageView_T>(depth_image_view,
		[&](VkImageView view) {
		vkDestroyImageView(_device.get(), view, nullptr);
	});
}

void Vulkan::create_texture_image()
{
	int tex_w, tex_h, tex_ch;
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &tex_w, &tex_h,
		&tex_ch, STBI_rgb_alpha);
	VkDeviceSize image_size = tex_w * tex_h * 4;

	VkBuffer buffer;
	VkDeviceMemory memory;
	{
		create_buffer(_physical_device.get(), _device.get(), image_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer, memory);

		void* data;
		vkMapMemory(_device.get(), memory, 0, image_size, 0, &data);
		{
			memcpy(data, pixels, static_cast<size_t>(image_size));
		}
		vkUnmapMemory(_device.get(), memory);
	}

	VkImage texture_image;
	VkDeviceMemory texture_image_memory;
	{
		create_image(_physical_device.get(), _device.get(),
			tex_w, tex_h,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			texture_image, texture_image_memory);

		transition_image_layout(_device.get(), _command_pool.get(), _graphics_queue.get(),
			texture_image, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		{
			copy_buffer_to_image(_device.get(), _command_pool.get(), _graphics_queue.get(),
				buffer, texture_image,
				static_cast<uint32_t>(tex_w), static_cast<uint32_t>(tex_h));
		}
		transition_image_layout(_device.get(), _command_pool.get(), _graphics_queue.get(),
			texture_image, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	_texture_image = std::shared_ptr<VkImage_T>(texture_image,
		[&](VkImage image) {
		vkDestroyImage(_device.get(), image, nullptr);
	});

	_texture_image_memory = std::shared_ptr<VkDeviceMemory_T>(texture_image_memory,
		[&](VkDeviceMemory memory) {
		vkFreeMemory(_device.get(), memory, nullptr);
	});

	vkDestroyBuffer(_device.get(), buffer, nullptr);
	vkFreeMemory(_device.get(), memory, nullptr);
	stbi_image_free(pixels);
}

void Vulkan::create_vertex_buffer()
{
	void* data;
	VkBuffer staging_buffer, buffer;
	VkDeviceMemory staging_buffer_memory, buffer_memory;
	VkDeviceSize buffer_size = sizeof(_vertices[0]) * _vertices.size();
	{
		create_buffer(_physical_device.get(), _device.get(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer, staging_buffer_memory);

		vkMapMemory(_device.get(), staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, _vertices.data(), static_cast<size_t>(buffer_size));
		vkUnmapMemory(_device.get(), staging_buffer_memory);

		create_buffer(_physical_device.get(), _device.get(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			buffer, buffer_memory);

		copy_buffer(_device.get(), _command_pool.get(), _graphics_queue.get(),
			staging_buffer, buffer, buffer_size);
	}

	_vertex_buffer = std::shared_ptr<VkBuffer_T>(buffer, [&](VkBuffer buffer) {
		vkDestroyBuffer(_device.get(), buffer, nullptr);
	});

	_vertex_buffer_memory = std::shared_ptr<VkDeviceMemory_T>(buffer_memory,
		[&](VkDeviceMemory memory) {
		vkFreeMemory(_device.get(), memory, nullptr);
	});

	vkDestroyBuffer(_device.get(), staging_buffer, nullptr);
	vkFreeMemory(_device.get(), staging_buffer_memory, nullptr);
}

void Vulkan::create_indice_buffer()
{
	void* data;
	VkBuffer staging_buffer, buffer;
	VkDeviceMemory staging_buffer_memory, buffer_memory;
	VkDeviceSize buffer_size = sizeof(_indices[0]) * _indices.size();
	{
		create_buffer(_physical_device.get(), _device.get(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer, staging_buffer_memory);

		vkMapMemory(_device.get(), staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, _indices.data(), static_cast<size_t>(buffer_size));
		vkUnmapMemory(_device.get(), staging_buffer_memory);

		create_buffer(_physical_device.get(), _device.get(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			buffer, buffer_memory);

		copy_buffer(_device.get(), _command_pool.get(), _graphics_queue.get(),
			staging_buffer, buffer, buffer_size);
	}

	_indices_buffer = std::shared_ptr<VkBuffer_T>(buffer, [&](VkBuffer buffer) {
		vkDestroyBuffer(_device.get(), buffer, nullptr);
	});

	_indices_buffer_memory = std::shared_ptr<VkDeviceMemory_T>(buffer_memory,
		[&](VkDeviceMemory memory) {
		vkFreeMemory(_device.get(), memory, nullptr);
	});

	vkDestroyBuffer(_device.get(), staging_buffer, nullptr);
	vkFreeMemory(_device.get(), staging_buffer_memory, nullptr);
}

void Vulkan::create_uniform_buffer()
{
	VkBuffer buffer;
	VkDeviceMemory buffer_memory;
	VkDeviceSize buffer_size = sizeof(UniformObject);
	{
		create_buffer(_physical_device.get(), _device.get(), buffer_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer, buffer_memory);
	}

	_uniform_buffer = std::shared_ptr<VkBuffer_T>(buffer,
		[&](VkBuffer buffer) {
		vkDestroyBuffer(_device.get(), buffer, nullptr);
	});

	_uniform_buffer_memory = std::shared_ptr<VkDeviceMemory_T>(buffer_memory,
		[&](VkDeviceMemory memory) {
		vkFreeMemory(_device.get(), memory, nullptr);
	});
}

void Vulkan::create_texture_image_view()
{
	VkImageView image_view = create_image_view(_device.get(),
		_texture_image.get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	_texture_image_view = std::shared_ptr<VkImageView_T>(image_view,
		[&](VkImageView view) {
		vkDestroyImageView(_device.get(), view, nullptr);
	});
}

void Vulkan::create_texture_sampler()
{
	VkSamplerCreateInfo sampler_info = {};
	{
		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = 16;
		sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	VkSampler texture_sampler;
	VkResult res = vkCreateSampler(_device.get(), &sampler_info, nullptr, &texture_sampler);
	assert(res == VK_SUCCESS);

	_texture_sampler = std::shared_ptr<VkSampler_T>(texture_sampler,
		[&](VkSampler sampler) {
		vkDestroySampler(_device.get(), sampler, nullptr);
	});
}

void Vulkan::create_descriptor_pool()
{
	std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
	{
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1;
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = 1;
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	{
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = 1;
	}

	VkDescriptorPool pool;
	VkResult res = vkCreateDescriptorPool(_device.get(),
		&pool_info, nullptr, &pool);
	assert(res == VK_SUCCESS);

	_descriptor_pool = std::shared_ptr<VkDescriptorPool_T>(pool,
		[&](VkDescriptorPool pool) {
		vkDestroyDescriptorPool(_device.get(), pool, nullptr);
	});
}

void Vulkan::create_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = {};
	VkDescriptorSetLayout layouts[] = { _descriptor_set_layout.get() };
	{
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = _descriptor_pool.get();
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = layouts;
	}
	VkDescriptorSet descriptor_set;
	VkResult res = vkAllocateDescriptorSets(_device.get(), &alloc_info, &descriptor_set);
	assert(res == VK_SUCCESS);

	VkDescriptorBufferInfo buffer_info = {};
	{
		buffer_info.buffer = _uniform_buffer.get();
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformObject);
	}

	VkDescriptorImageInfo image_info = {};
	{
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = _texture_image_view.get();
		image_info.sampler = _texture_sampler.get();
	}

	std::array<VkWriteDescriptorSet, 2> descriptor_writers = {};
	{
		descriptor_writers[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writers[0].dstSet = descriptor_set;
		descriptor_writers[0].dstBinding = 0;
		descriptor_writers[0].dstArrayElement = 0;
		descriptor_writers[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writers[0].descriptorCount = 1;
		descriptor_writers[0].pBufferInfo = &buffer_info;

		descriptor_writers[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writers[1].dstSet = descriptor_set;
		descriptor_writers[1].dstBinding = 1;
		descriptor_writers[1].dstArrayElement = 0;
		descriptor_writers[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writers[1].descriptorCount = 1;
		descriptor_writers[1].pImageInfo = &image_info;
	}
	vkUpdateDescriptorSets(_device.get(), 2, descriptor_writers.data(), 0, nullptr);

	_descriptor_set = std::shared_ptr<VkDescriptorSet_T>(descriptor_set, [](VkDescriptorSet set) {
	});
}

void Vulkan::create_command_buffers()
{
	VkCommandBufferAllocateInfo allocate_info = {};
	{
		_command_buffers.resize(_swapchain_framebuffers.size());
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandPool = _command_pool.get();
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandBufferCount = static_cast<uint32_t>(_command_buffers.size());
	}
	VkResult res = vkAllocateCommandBuffers(_device.get(), &allocate_info, _command_buffers.data());
	assert(res == VK_SUCCESS);

	for (size_t i = 0; i < _command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		begin_info.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(_command_buffers[i], &begin_info);
		{
			VkRenderPassBeginInfo renderpass_info = {};
			std::array<VkClearValue, 2> clear_colors = {};
			clear_colors[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clear_colors[1].depthStencil = { 1.0f, 0 };
			{
				renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderpass_info.renderPass = _renderpass.get();
				renderpass_info.framebuffer = _swapchain_framebuffers[i].get();
				renderpass_info.renderArea.offset = { 0, 0 };
				renderpass_info.renderArea.extent = _swapchain_extent;
				renderpass_info.clearValueCount = static_cast<uint32_t>(clear_colors.size());
				renderpass_info.pClearValues = clear_colors.data();
			}
			vkCmdBeginRenderPass(_command_buffers[i], &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
			{
				vkCmdBindPipeline(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
					_graphics_pipeline.get());

				VkDeviceSize offsets[] = { 0 };
				VkBuffer vertex_buffers[] = { _vertex_buffer.get() };
				VkDescriptorSet descriptor_set = _descriptor_set.get();
				vkCmdBindVertexBuffers(_command_buffers[i], 0, 1, vertex_buffers, offsets);
				vkCmdBindIndexBuffer(_command_buffers[i], _indices_buffer.get(), 0,
					VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
					_pipeline_layout.get(), 0, 1, &descriptor_set, 0, nullptr);

				vkCmdDrawIndexed(_command_buffers[i],
					static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
			}
			vkCmdEndRenderPass(_command_buffers[i]);
		}
		vkEndCommandBuffer(_command_buffers[i]);
	}
}

void Vulkan::create_semaphores()
{
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore1, semaphore2;
	VkResult res1 = vkCreateSemaphore(_device.get(), &semaphore_info, nullptr, &semaphore1);
	VkResult res2 = vkCreateSemaphore(_device.get(), &semaphore_info, nullptr, &semaphore2);

	assert(res1 == VK_SUCCESS && res2 == VK_SUCCESS);

	_image_available_semaphore = std::shared_ptr<VkSemaphore_T>(semaphore1, [&](VkSemaphore s) {
		vkDestroySemaphore(_device.get(), s, nullptr);
	});
	_render_finished_semaphore = std::shared_ptr<VkSemaphore_T>(semaphore2, [&](VkSemaphore s) {
		vkDestroySemaphore(_device.get(), s, nullptr);
	});
}

void Vulkan::create_command_pool()
{
	VkCommandPoolCreateInfo pool_info = {};
	{
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = _graphics_queue_family_index;
		pool_info.flags = 0;
	}

	VkCommandPool commandpool;
	VkResult res = vkCreateCommandPool(_device.get(), &pool_info, nullptr, &commandpool);

	assert(res == VK_SUCCESS);

	_command_pool = std::shared_ptr<VkCommandPool_T>(commandpool,
		[&](VkCommandPool commandpool) {
		vkDestroyCommandPool(_device.get(), commandpool, nullptr);
	});
}

//////////////////////////////////// update functions //////////////////////////////////

void Vulkan::recreate_swapchain()
{
	vkDeviceWaitIdle(_device.get());
	clear_swapchain();
	create_swapchain();
	create_image_views();
	create_renderpass();
	create_graphics_pipeline();
	create_depth_image();
	create_depth_image_view();
	create_frame_buffers();
	create_command_buffers();
}

void Vulkan::update_uniform_buffer()
{
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>
		(current_time - start_time).count() / 1000.0f;

	UniformObject ubo = {};
	{
		ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f),
			_swapchain_extent.width / _swapchain_extent.height * 1.0f, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
	}

	void* data;
	{
		vkMapMemory(_device.get(), _uniform_buffer_memory.get(), 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(_device.get(), _uniform_buffer_memory.get());
	}
}

void Vulkan::load_model()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	bool res = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
		MODEL_PATH.c_str());
	assert(res == true);

	std::unordered_map<Vertex, uint32_t> unique_vertices = {};

	for (const auto& shape : shapes)
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.tex_coord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (unique_vertices.count(vertex) == 0)
			{
				unique_vertices[vertex] = static_cast<uint32_t>(_vertices.size());
				_vertices.push_back(vertex);
			}
			_indices.push_back(unique_vertices[vertex]);
		}
}

void Vulkan::draw_frame()
{
	uint32_t image_index;
	VkResult res_acq = vkAcquireNextImageKHR(_device.get(), _swapchain.get(),
		std::numeric_limits<uint64_t>::max(),
		_image_available_semaphore.get(), VK_NULL_HANDLE,
		&image_index);

	if (res_acq == VK_ERROR_OUT_OF_DATE_KHR)
		recreate_swapchain();
	else if (res_acq != VK_SUCCESS && res_acq != VK_SUBOPTIMAL_KHR)
		assert(false);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { _image_available_semaphore.get() };
	VkSemaphore signal_semaphores[] = { _render_finished_semaphore.get() };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pSignalSemaphores = signal_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &_command_buffers[image_index];

	VkResult res = vkQueueSubmit(_graphics_queue.get(), 1, &submit_info, VK_NULL_HANDLE);
	assert(res == VK_SUCCESS);

	VkPresentInfoKHR present_info = {};
	VkSwapchainKHR swapchains[] = { _swapchain.get() };
	{
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;
		present_info.pImageIndices = &image_index;
	}

	VkResult res_que = vkQueuePresentKHR(_present_queue.get(), &present_info);

	if (res_que == VK_ERROR_OUT_OF_DATE_KHR || res_que == VK_SUBOPTIMAL_KHR)
		recreate_swapchain();
	else if (res_que != VK_SUCCESS)
		assert(false);

	vkQueueWaitIdle(_present_queue.get());
}

//////////////////////////////////// destroy functions /////////////////////////////////

void Vulkan::clear_swapchain()
{
	_depth_image_view.reset();
	_depth_image.reset();
	_depth_image_memory.reset();
	_swapchain_framebuffers.clear();
	vkFreeCommandBuffers(_device.get(), _command_pool.get(),
		static_cast<uint32_t>(_command_buffers.size()), _command_buffers.data());
	_graphics_pipeline.reset();
	_pipeline_layout.reset();
	_renderpass.reset();
	_image_views.clear();
	_swapchain.reset();
}