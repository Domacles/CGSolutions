#ifndef INCLUDE_VULKANAPPLICATION_H
#define INCLUDE_VULKANAPPLICATION_H

#ifndef VULKAN_NAMESPACE_BEGIN
#define VULKAN_NAMESPACE_BEGIN(vname) namespace vname {
#define VULKAN_NAMESPACE_END(vname) }
#endif

#include <array>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

VULKAN_NAMESPACE_BEGIN(VulkanApp)

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 tex_coord;

	static VkVertexInputBindingDescription binding_description();
	static std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions();

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
	}
};

struct UniformObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// const std::vector<Vertex> vertices = {
// 	{ { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
// 	{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
// 	{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
// 	{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } },
//
// 	{ { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
// 	{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
// 	{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
// 	{ { -0.5f, 0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } }
// };
//
// const std::vector<uint16_t> indices = {
// 	0, 1, 2, 2, 3, 0,
// 	4, 5, 6, 6, 7, 4
// };

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::string MODEL_PATH = "chalet.obj";
const std::string TEXTURE_PATH = "chalet.jpg";

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

	static void on_window_resized(GLFWwindow* window, int width, int height);

protected:
	// window property
	int _window_width;
	int _window_height;

	// glfw object or pointer
	HWND _hwnd;
	HINSTANCE _hinstance;
	std::shared_ptr<GLFWwindow> _window;

	// vulkan object
	VkExtent2D _swapchain_extent;
	VkFormat _swapchain_image_format;
	uint32_t _present_queue_family_index;
	uint32_t _graphics_queue_family_index;
	VkSurfaceCapabilitiesKHR _vksurface_capabilities;
	VkPipelineShaderStageCreateInfo _shader_stages[2];

	// vulkan object pointer

	std::shared_ptr<VkInstance_T> _instance;

	std::shared_ptr<VkSurfaceKHR_T> _surface;

	std::shared_ptr<VkPhysicalDevice_T> _physical_device;

	std::shared_ptr<VkDevice_T> _device;

	std::shared_ptr<VkSwapchainKHR_T> _swapchain;

	std::shared_ptr<VkCommandPool_T> _command_pool;

	std::shared_ptr<VkDescriptorSet_T> _descriptor_set;
	std::shared_ptr<VkDescriptorPool_T> _descriptor_pool;
	std::shared_ptr<VkDescriptorSetLayout_T> _descriptor_set_layout;

	std::shared_ptr<VkImageView_T> _depth_image_view;
	std::shared_ptr<VkImageView_T> _texture_image_view;

	std::shared_ptr<VkPipeline_T> _graphics_pipeline;
	std::shared_ptr<VkPipelineLayout_T> _pipeline_layout;

	std::shared_ptr<VkRenderPass_T> _renderpass;

	std::shared_ptr<VkSampler_T> _texture_sampler;

	std::shared_ptr<VkSemaphore_T> _image_available_semaphore;
	std::shared_ptr<VkSemaphore_T> _render_finished_semaphore;

	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;

	std::shared_ptr<VkImage_T> _depth_image;
	std::shared_ptr<VkImage_T> _texture_image;
	std::shared_ptr<VkQueue_T> _present_queue;
	std::shared_ptr<VkQueue_T> _graphics_queue;
	std::shared_ptr<VkBuffer_T> _vertex_buffer;
	std::shared_ptr<VkBuffer_T> _indices_buffer;
	std::shared_ptr<VkBuffer_T> _uniform_buffer;
	std::shared_ptr<VkDeviceMemory_T> _depth_image_memory;
	std::shared_ptr<VkDeviceMemory_T> _texture_image_memory;
	std::shared_ptr<VkDeviceMemory_T> _vertex_buffer_memory;
	std::shared_ptr<VkDeviceMemory_T> _indices_buffer_memory;
	std::shared_ptr<VkDeviceMemory_T> _uniform_buffer_memory;

	// vulkan vectors for structs
	std::vector<VkSurfaceFormatKHR> _formats;
	std::vector<VkPresentModeKHR> _present_modes;
	std::vector<VkQueueFamilyProperties> _queue_families;
	std::vector<VkExtensionProperties> _instance_extensions;

	// vulkan vectors for pointers
	std::vector<char> _vshader_spir_v_bytes;
	std::vector<char> _fshader_spir_v_bytes;
	std::vector<VkCommandBuffer> _command_buffers;
	std::vector<const char*> _instance_extension_names;
	std::vector<const char*> _logical_device_extension_names;
	std::vector<std::shared_ptr<VkImageView_T>> _image_views;
	std::vector<std::shared_ptr<VkImage_T>> _swapchain_images;
	std::vector<std::shared_ptr<VkPhysicalDevice_T>> _physical_devices;
	std::vector<std::shared_ptr<VkFramebuffer_T>> _swapchain_framebuffers;

	// Vulkan
	void init_surface();
	void init_instance();
	void init_extensions();
	void init_logical_device();
	void init_physical_device();

	void create_swapchain();
	void create_semaphores();
	void create_renderpass();
	void create_image_views();
	void create_command_pool();
	void create_frame_buffers();
	void create_descriptor_set();
	void create_descriptor_pool();
	void create_command_buffers();
	void create_graphics_pipeline();
	void create_descriptor_set_layout();

	void load_model();
	void draw_frame();
	void clear_swapchain();
	void recreate_swapchain();
	void update_uniform_buffer();

	void create_depth_image();
	void create_texture_image();
	void create_vertex_buffer();
	void create_indice_buffer();
	void create_uniform_buffer();
	void create_texture_sampler();
	void create_depth_image_view();
	void create_texture_image_view();

	// main function
	ExecutionStatus destroy();
	ExecutionStatus main_loop();
	ExecutionStatus init_vulkan();
	ExecutionStatus init_window();
};

template<typename Collection, typename FunTypeBefore, typename FunType, typename FunTypeAfter>
void debug_each(Collection& collection, FunTypeBefore before_func, FunType each_func,
	FunTypeAfter after_func)
{
#ifdef _DEBUG
	before_func(collection);
	for (auto& e : collection)
		each_func(e);
	after_func(collection);
#endif
}

VULKAN_NAMESPACE_END(VulkanApp)

#endif // INCLUDE_VULKANAPPLICATION_H
